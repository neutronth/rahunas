/**
 * RahuNAS task bandwidth implementation 
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-11-20
 */

#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>

#include "rahunasd.h"
#include "rh-task.h"
#include "rh-task-bandwidth.h"
#include "rh-task-memset.h"
#include "rh-utils.h"

static uint64_t slot_flags[MAX_SLOT_PAGE];
static uint64_t slot_count = 0;

static uint16_t _get_slot_id()
{
  uint16_t slot_id    = 0;
  uint16_t page       = 0;
  uint8_t  id_on_page = 0;
  time_t random_time;

  // Slot is full 
  if (slot_count >= MAX_SLOT_ID)
    return 0;

  // Do a random slot_id 
  time(&(random_time));
  srandom(random_time);
  srandom(random());

  while (slot_id == 0) {
    slot_id = random () % (MAX_SLOT_ID + 1);
   
    // Check validity
    page = slot_id / PAGE_SIZE;
    id_on_page = slot_id % PAGE_SIZE;
   
    if (!(slot_flags[page] & (1 << id_on_page))) {
      // Slot available
      break;
    }

    // Second try, probe other slot in current page
    uint16_t id    = 0;
    for (id = 0; id < PAGE_SIZE; id++) {
      if (!(slot_flags[page] & (1 << id))) {
        slot_id = (page * PAGE_SIZE) + id;
        goto done;
      }
    }

    // Slot not available, retry
    srandom(slot_id);
    slot_id = 0;
  }

done:
  return slot_id;
}

void mark_reserved_slot_id(uint16_t slot_id)
{
  uint16_t page       = 0;
  uint8_t  id_on_page = 0;

  page = slot_id / PAGE_SIZE;
  id_on_page = slot_id % PAGE_SIZE;

  slot_count++;
  slot_flags[page] |= 1 << id_on_page;
}

void unmark_reserved_slot_id(uint16_t slot_id)
{
  uint16_t page       = 0;
  uint8_t  id_on_page = 0;
  uint16_t id_flag    = 0;

  page = slot_id / PAGE_SIZE;
  id_on_page = slot_id % PAGE_SIZE;
  id_flag = 1 << id_on_page;

  if (slot_count > 0)
    slot_count--;

  slot_flags[page] &= ~(id_flag);
}

int bandwidth_exec(RHVServer *vs, char *const args[])
{
  char buffer[150];
  int  ret = 0;

  ret = rh_cmd_exec(RAHUNAS_BANDWIDTH_WRAPPER, args, NULL, buffer,
                    sizeof (buffer));

  if (ret >= 0) {
    if (strncmp (buffer, "NOT COMPLETED", 13) == 0) {
      ret = -2;
    }

    logmsg(RH_LOG_NORMAL, "[%s] Bandwidth: %s",
           vs->vserver_config->vserver_name, buffer);
  } else {
    logmsg(RH_LOG_NORMAL, "[%s] Bandwidth: error",
           vs->vserver_config->vserver_name);
  }

  return ret;
}

int bandwidth_start(RHVServer *vs)
{
  char *args[5];
  struct interfaces *iface = vs->vserver_config->iface;
  int  ret;

  DP("Bandwidth: start");

  args[0] = RAHUNAS_BANDWIDTH_WRAPPER;
  args[1] = "start";
  args[2] = iface->dev_internal;
  args[3] = iface->dev_ifb;
  args[4] = (char *) 0;

  ret = bandwidth_exec(vs, args);
  return ret; 
}

int bandwidth_stop(RHVServer *vs)
{
  char *args[5];
  struct interfaces *iface = vs->vserver_config->iface;
  int  ret;

  DP("Bandwidth: stop");

  args[0] = RAHUNAS_BANDWIDTH_WRAPPER;
  args[1] = "stop";
  args[2] = iface->dev_internal;
  args[3] = iface->dev_ifb;
  args[4] = (char *) 0;

  ret = bandwidth_exec(vs, args);
  return ret; 
}

int bandwidth_add(RHVServer *vs, struct bandwidth_req *bw_req)
{
  char *args[9];
  struct interfaces *iface = vs->vserver_config->iface;

  DP("Bandwidth: request %s %s %s %s", bw_req->slot_id, 
     bw_req->ip, bw_req->bandwidth_max_down, bw_req->bandwidth_max_up);

  args[0] = RAHUNAS_BANDWIDTH_WRAPPER;
  args[1] = "add";
  args[2] = bw_req->slot_id;
  args[3] = bw_req->ip;
  args[4] = bw_req->bandwidth_max_down;
  args[5] = bw_req->bandwidth_max_up;
  args[6] = iface->dev_internal;
  args[7] = iface->dev_ifb;
  args[8] = (char *) 0;

  return bandwidth_exec(vs, args);
}

int bandwidth_del(RHVServer *vs, struct bandwidth_req *bw_req)
{
  char *args[6];
  struct interfaces *iface = vs->vserver_config->iface;

  DP("Bandwidth: request %s", bw_req->slot_id);

  args[0] = RAHUNAS_BANDWIDTH_WRAPPER;
  args[1] = "del";
  args[2] = bw_req->slot_id;
  args[3] = iface->dev_internal;
  args[4] = iface->dev_ifb;
  args[5] = (char *) 0;

  return bandwidth_exec(vs, args);
}

/* Start service task */
static int startservice (void)
{
  /* Do nothing */
}

/* Stop service task */
static int stopservice (void)
{
  /* Do nothing */
}

/* Initialize */
static void init (RHVServer *vs)
{
  struct interfaces *iface = NULL;
  if (!vs)
    return;

  if (vs->vserver_config->init_flag <= VS_INIT) {
    memset (slot_flags, 0, sizeof (uint64_t) * MAX_SLOT_PAGE);
  }

  if (vs->vserver_config->init_flag == VS_RELOAD)
    goto initial;

  interfaces_list = append_interface (interfaces_list, 
                                      vs->vserver_config->dev_internal);

initial:
  vs->vserver_config->iface = get_interface (interfaces_list, 
                                             vs->vserver_config->dev_internal);
  iface = vs->vserver_config->iface;
  if (!iface->init)
    if (bandwidth_start(vs) >= 0)
      iface->init = 1;
}

/* Cleanup */
static void cleanup (RHVServer *vs)
{
  struct interfaces *iface = NULL;
  if (!vs)
    return;

  if (vs->vserver_config->init_flag == VS_RELOAD)
    return;

  iface = vs->vserver_config->iface;
  DP ("Bandwidth Cleanup: init=%d, hit=%d", iface->init, iface->hit);
  if (iface->init && iface->hit <= 1)
    if (bandwidth_stop(vs) >= 0)
      iface->init = 0;

  interfaces_list = remove_interface (interfaces_list, 
                                      vs->vserver_config->dev_internal);
}

/* Start session task */
static int startsess (RHVServer *vs, struct task_req *req)
{
  struct bandwidth_req bw_req;
  uint16_t slot_id;
  unsigned char max_try = 3;
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;

  member_node = member_get_node_by_id(vs, req->id);
  if (member_node == NULL)
    return (-1);

  member = (struct rahunas_member *) member_node->data;

  if (member->bandwidth_max_down == 0 && member->bandwidth_max_up == 0)
    return 0;

  if (member->bandwidth_slot_id > 0)
    return 0;

  memset (bw_req.ip, 0, sizeof (bw_req.ip));
  memset (bw_req.bandwidth_max_down, 0, sizeof (bw_req.bandwidth_max_down));
  memset (bw_req.bandwidth_max_up, 0, sizeof (bw_req.bandwidth_max_up));
  
  // Formating the bandwidth request
  snprintf(bw_req.ip, sizeof (bw_req.ip), "%s", idtoip(vs->v_map, req->id));
  snprintf(bw_req.bandwidth_max_down, sizeof (bw_req.bandwidth_max_down), 
           "%lu", member->bandwidth_max_down);
  snprintf(bw_req.bandwidth_max_up, sizeof (bw_req.bandwidth_max_up), "%lu", 
           member->bandwidth_max_up);
  
  while (max_try > 0) { 
    slot_id = req->bandwidth_slot_id > 0 ? req->bandwidth_slot_id :
                _get_slot_id();
    snprintf(bw_req.slot_id, sizeof (bw_req.slot_id), "%" PRIu16, slot_id);
    if (slot_id > 0 && bandwidth_add(vs, &bw_req) == 0)
      break;
    else {
      max_try--;
      sleep (1); 
    }
  }


  if (max_try == 0) {
    logmsg(RH_LOG_ERROR, "[%s] Bandwidth: Maximum trying, failed!",
           vs->vserver_config->vserver_name);
    return -1;
  }

  mark_reserved_slot_id(slot_id);
  member->bandwidth_slot_id = slot_id;
 
  return 0;
}

/* Stop session task */
static int stopsess  (RHVServer *vs, struct task_req *req)
{
  struct bandwidth_req bw_req;
  uint16_t slot_id = 0;
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;
  
  member_node = member_get_node_by_id(vs, req->id);
  if (member_node == NULL)
    return (-1); 
  
  member = (struct rahunas_member *) member_node->data;
  slot_id = member->bandwidth_slot_id;

  if (slot_id < 1)
    return 0;

  snprintf(bw_req.slot_id, sizeof (bw_req.slot_id), "%" PRIu16, slot_id);

  if (bandwidth_del(vs, &bw_req) == 0) {
    unmark_reserved_slot_id (member->bandwidth_slot_id);
    member->bandwidth_slot_id = 0;

    return 0; 
  } else {
    return -1;
  }
}

/* Commit start session task */
static int commitstartsess (RHVServer *vs, struct task_req *req)
{
  /* Do nothing or need to implement */
}

/* Commit stop session task */
static int commitstopsess  (RHVServer *vs, struct task_req *req)
{
  /* Do nothing or need to implement */
}

/* Rollback start session task */
static int rollbackstartsess (RHVServer *vs, struct task_req *req)
{
  /* Do nothing or need to implement */
}

/* Rollback stop session task */
static int rollbackstopsess  (RHVServer *vs, struct task_req *req)
{
  /* Do nothing or need to implement */
}

static struct task taskbandwidth = {
  .taskname = "BANDWIDTH",
  .taskprio = 20,
  .init = &init,
  .cleanup = &cleanup,
  .startservice = &startservice,
  .stopservice = &stopservice,
  .startsess = &startsess,
  .stopsess = &stopsess,
  .commitstartsess = &commitstartsess,
  .commitstopsess = &commitstopsess,
  .rollbackstartsess = &rollbackstartsess,
  .rollbackstopsess = &rollbackstopsess,
};

void rh_task_bandwidth_reg(RHMainServer *ms) {
  task_register(ms, &taskbandwidth);
}
