/**
 * RahuNAS task memset implementation 
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-09-07
 */

#include <stdlib.h>
#include <syslog.h>
#include <time.h>
#include "rahunasd.h"
#include "rh-radius.h"
#include "rh-task.h"
#include "rh-ipset.h"

/* Initialize */
static void init (void)
{
  struct rahunas_member *members = NULL;
  int size;

  logmsg(RH_LOG_NORMAL, "Task MEMSET init..");  
  map = (struct rahunas_map*)(rh_malloc(sizeof(struct rahunas_map)));

  map->members = NULL;
  map->size = 0;

  if (get_header_from_set(map) < 0) {
    syslog(LOG_ERR, "Could not fetch IPSET header");
    exit(EXIT_FAILURE);
  }

  size = map->size == 0 ? MAX_MEMBERS : map->size;

  members = 
    (struct rahunas_member*)(rh_malloc(sizeof(struct rahunas_member)*size));

  memset(members, 0, sizeof(struct rahunas_member)*size);

  map->members = members;
}

/* Cleanup */
static void cleanup (void)
{
  struct rahunas_member *members = NULL;
  int i;
  int end;

  logmsg(RH_LOG_NORMAL, "Task MEMSET cleanup..");  

  if (map) {
    if (map->members) {
      members = map->members;
      end = map->size;
    } else {
      end = 0;
    }  

    for (i=0; i < end; i++) {
        rh_free_member(&members[i]);
    }

    rh_free(&(map->members));
    rh_free(&map);
  }

  return 0;
}


/* Start service task */
static int startservice (struct rahunas_map *map)
{
  /* Do nothing or need to implement */
}

/* Stop service task */
static int stopservice  (struct rahunas_map *map)
{
  /* Do nothing or need to implement */
}

/* Start session task */
static int startsess (struct rahunas_map *map, struct task_req *req)
{
  struct rahunas_member *members = map->members;
  uint32_t id = req->id;

  members[id].flags = 1;
  if (members[id].username && members[id].username != termstring)
    free(members[id].username);

  if (members[id].session_id && members[id].username != termstring)
    free(members[id].session_id);

  members[id].username   = strdup(req->username);
  if (!members[id].username)
    members[id].username = termstring;

  members[id].session_id = strdup(req->session_id);
  if (!members[id].session_id)
    members[id].session_id = termstring;

  time(&(members[id].session_start));
  memcpy(&req->session_start, &members[id].session_start, sizeof(time_t));

  memcpy(&members[id].mac_address, &(req->mac_address), ETH_ALEN);

  memcpy(&members[id].session_timeout, &req->session_timeout, sizeof(time_t));
  members[id].bandwidth_max_down = req->bandwidth_max_down;
  members[id].bandwidth_max_up = req->bandwidth_max_up;
 
  DP("Session-Timeout %d", req->session_timeout);
  DP("Bandwidth - Down: %lu, Up: %lu", req->bandwidth_max_down, 
       req->bandwidth_max_up);

  logmsg(RH_LOG_NORMAL, "Session Start, User: %s, IP: %s, "
                        "Session ID: %s, MAC: %s",
                        members[id].username, 
                        idtoip(map, id), 
                        members[id].session_id,
                        mac_tostring(members[id].mac_address)); 
  return 0;
}

/* Stop session task */
static int stopsess  (struct rahunas_map *map, struct task_req *req)
{
  struct rahunas_member *members = map->members;
  uint32_t id = req->id;
  char cause[16] = "";
  switch (req->req_opt) {
    case RH_RADIUS_TERM_IDLE_TIMEOUT :
      strcpy(cause, "idle timeout");
      break;
    case RH_RADIUS_TERM_USER_REQUEST :
      strcpy(cause, "user request");
      break;
    case RH_RADIUS_TERM_NAS_REBOOT :
      strcpy(cause, "nas reboot");
      break;
  }
  if (!members[id].username)
    members[id].username = termstring;

  if (!members[id].session_id)
    members[id].session_id = termstring;

  logmsg(RH_LOG_NORMAL, "Session Stop (%s), User: %s, IP: %s, "
                        "Session ID: %s, MAC: %s",
                        cause,
                        members[id].username, 
                        idtoip(map, id), 
                        members[id].session_id,
                        mac_tostring(members[id].mac_address)); 

  rh_free_member(&members[id]);   

  return 0;
 
}

/* Commit start session task */
static int commitstartsess (struct rahunas_map *map, struct task_req *req)
{
  /* Do nothing or need to implement */
}

/* Commit stop session task */
static int commitstopsess  (struct rahunas_map *map, struct task_req *req)
{
  /* Do nothing or need to implement */
}

/* Rollback start session task */
static int rollbackstartsess (struct rahunas_map *map, struct task_req *req)
{
  /* Do nothing or need to implement */
}

/* Rollback stop session task */
static int rollbackstopsess  (struct rahunas_map *map, struct task_req *req)
{
  /* Do nothing or need to implement */
}

static struct task task_memset = {
  .taskname = "MEMSET",
  .taskprio = 40,
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

void rh_task_memset_reg(void) {
  task_register(&task_memset);
}
