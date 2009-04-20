/**
 * RahuNAS task bandwidth implementation 
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-11-20
 */

#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "rahunasd.h"
#include "rh-task.h"
#include "rh-task-bandwidth.h"
#include "rh-task-memset.h"
#include "rh-utils.h"



#define BANDWIDTH_WRAPPER "/etc/rahunas/bandwidth.sh"

static unsigned short slot_flags[MAX_SLOT_PAGE] = {1};
static unsigned short slot_count = 0;

unsigned short _get_slot_id()
{
  unsigned short slot_id    = 0;
  unsigned short page       = 0;
  unsigned char  id_on_page = 0;
  time_t random_time;

  // Slot is full 
  if (slot_count >= MAX_SLOT_ID)
    return 0;

  // Do a random slot_id 
  while (slot_id == 0) {
    time(&(random_time));
    srandom(random_time);
    slot_id = random()/(int)(((unsigned int)RAND_MAX + 1) / (MAX_SLOT_ID + 1));
   
    // Check validity
    page = slot_id / PAGE_SIZE; 
    id_on_page = slot_id % PAGE_SIZE;
   
    if (!(slot_flags[page] & (1 << id_on_page))) {
      // Slot available
      break;
    }

    // Slot not available, retry
    slot_id = 0;
  }

  return slot_id;
}

void mark_reserved_slot_id(unsigned int slot_id)
{
  unsigned short page       = 0;
  unsigned char  id_on_page = 0;

  page = slot_id / PAGE_SIZE; 
  id_on_page = slot_id % PAGE_SIZE;

  slot_count++;
  slot_flags[page] |= 1 << id_on_page;
}

int bandwidth_exec(struct vserver *vs, char *const args[])
{
  pid_t ws;
  pid_t pid;
  int status;
  int exec_pipe[2];
  char buffer[150];
  char *endline = NULL;
  int ret = 0;
  int fd = 0;
  
  memset(buffer, '\0', sizeof(buffer));

  if (pipe(exec_pipe) == -1) {
    logmsg(RH_LOG_ERROR, "Error: pipe()");
    return -1;
  }
  DP("pipe0=%d,pipe1=%d", exec_pipe[0], exec_pipe[1]);

  pid = vfork();
  dup2(exec_pipe[1], STDOUT_FILENO);

  if (pid == 0) {
    // Child
    execv(BANDWIDTH_WRAPPER, args);
  } else if (pid < 0) {
    // Fork error
    logmsg(RH_LOG_ERROR, "Error: vfork()"); 
    ret = -1;
  } else {
    // Parent
    ws = waitpid(pid, &status, 0);

    DP("Bandwidth: Return (%d)", WEXITSTATUS(status));

    // Return message log
    DP("Read message");
    read(exec_pipe[0], buffer, sizeof(buffer));

    if (buffer != NULL) {
      DP("Got message: %s", buffer);
      endline = strstr(buffer, "\n");
      if (endline != NULL) 
        *endline = '\0';

      if (vs != NULL) {
        logmsg(RH_LOG_NORMAL, "[%s] Bandwidth: %s", 
          vs->vserver_config->vserver_name, buffer);
      } else {
        logmsg(RH_LOG_NORMAL, "[main server] Bandwidth: %s", buffer);
      }
    }

    if (WIFEXITED(status)) {
      ret = WEXITSTATUS(status);
    } else {
      ret = -1;
    } 
  }

  close(exec_pipe[0]);
  close(exec_pipe[1]);
  return ret;
}

int bandwidth_start(void)
{
  char *args[3];

  DP("Bandwidth: start");

  args[0] = BANDWIDTH_WRAPPER;
  args[1] = "start";
  args[2] = (char *) 0;

  return bandwidth_exec(NULL, args);
}

int bandwidth_stop(void)
{
  char *args[3];

  DP("Bandwidth: stop");

  args[0] = BANDWIDTH_WRAPPER;
  args[1] = "stop";
  args[2] = (char *) 0;

  return bandwidth_exec(NULL, args);
}

int bandwidth_add(struct vserver *vs, struct bandwidth_req *bw_req)
{
  char *args[7];

  DP("Bandwidth: request %s %s %s %s", bw_req->slot_id, 
     bw_req->ip, bw_req->bandwidth_max_down, bw_req->bandwidth_max_up);

  args[0] = BANDWIDTH_WRAPPER;
  args[1] = "add";
  args[2] = bw_req->slot_id;
  args[3] = bw_req->ip;
  args[4] = bw_req->bandwidth_max_down;
  args[5] = bw_req->bandwidth_max_up;
  args[6] = (char *) 0;

  return bandwidth_exec(vs, args);
}

int bandwidth_del(struct vserver *vs, struct bandwidth_req *bw_req)
{
  char *args[4];

  DP("Bandwidth: request %s", bw_req->slot_id);

  args[0] = BANDWIDTH_WRAPPER;
  args[1] = "del";
  args[2] = bw_req->slot_id;
  args[3] = (char *) 0;

  return bandwidth_exec(vs, args);
}

/* Start service task */
static int startservice (void)
{
  return bandwidth_start();
}

/* Stop service task */
static int stopservice (void)
{
  return bandwidth_stop();
}

/* Initialize */
static void init (struct vserver *vs)
{
  /* Do nothing */
}

/* Cleanup */
static int cleanup (struct vserver *vs)
{
  /* Do nothing */
}

/* Start session task */
static int startsess (struct vserver *vs, struct task_req *req)
{
  struct bandwidth_req bw_req;
  unsigned short slot_id;
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
  
  // Formating the bandwidth request
  sprintf(bw_req.ip, "%s", idtoip(vs->v_map, req->id));
  sprintf(bw_req.bandwidth_max_down, "%lu", 
    member->bandwidth_max_down);
  sprintf(bw_req.bandwidth_max_up, "%lu", 
    member->bandwidth_max_up);
  
  while (max_try > 0) { 
    slot_id = _get_slot_id();
    sprintf(bw_req.slot_id, "%d", slot_id);
    if (bandwidth_add(vs, &bw_req) == 0)
      break;
    else
      max_try--;
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
static int stopsess  (struct vserver *vs, struct task_req *req)
{
  struct bandwidth_req bw_req;
  unsigned short slot_id = 0;
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;
  
  member_node = member_get_node_by_id(vs, req->id);
  if (member_node == NULL)
    return (-1); 
  
  member = (struct rahunas_member *) member_node->data;
  slot_id = member->bandwidth_slot_id;

  if (slot_id < 1)
    return 0;

  sprintf(bw_req.slot_id, "%d", slot_id);

  if (bandwidth_del(vs, &bw_req) == 0) {
    member->bandwidth_slot_id = 0;

    if (slot_count > 0)
      slot_count--;
  
    return 0; 
  } else {
    return -1;
  }
}

/* Commit start session task */
static int commitstartsess (struct vserver *vs, struct task_req *req)
{
  /* Do nothing or need to implement */
}

/* Commit stop session task */
static int commitstopsess  (struct vserver *vs, struct task_req *req)
{
  /* Do nothing or need to implement */
}

/* Rollback start session task */
static int rollbackstartsess (struct vserver *vs, struct task_req *req)
{
  /* Do nothing or need to implement */
}

/* Rollback stop session task */
static int rollbackstopsess  (struct vserver *vs, struct task_req *req)
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

void rh_task_bandwidth_reg(struct main_server *ms) {
  task_register(ms, &taskbandwidth);
}
