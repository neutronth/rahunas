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

GList *member_get_node_by_id(struct vserver *vs, uint32_t id) 
{
  GList *runner = g_list_first(vs->v_map->members);
  struct rahunas_member *member = NULL;

  while (runner != NULL) {
    member = (struct rahunas_member *) runner->data;

    if (member->id == id)
      return runner;

    runner = g_list_next(runner);
  }

  return NULL;
}

gint idcmp(struct rahunas_member *a, struct rahunas_member *b)
{
  if (a == NULL || b == NULL)
    return 0;

  if (a != NULL && b != NULL)
    return (a->id - b->id);
}

/* Initialize */
static void init (struct vserver *vs)
{
  int size;

  logmsg(RH_LOG_NORMAL, "[%s] Task MEMSET init..",
         vs->vserver_config->vserver_name);  

  vs->v_map = (struct rahunas_map *)(rh_malloc(sizeof(struct rahunas_map)));
  if (vs->v_map == NULL) {
    logmsg(RH_LOG_ERROR, "[%s] Could not allocate memory...",
           vs->vserver_config->vserver_name); 
    exit(EXIT_FAILURE);
  }

  memset(vs->v_map, 0, sizeof(struct rahunas_map));

  if (get_header_from_set(vs) < 0) {
    syslog(LOG_ERR, "Could not fetch IPSET header");
    exit(EXIT_FAILURE);
  }
}

/* Cleanup */
static void cleanup (struct vserver *vs)
{
  GList *runner = NULL;
  struct rahunas_member *member = NULL;

  logmsg(RH_LOG_NORMAL, "[%s] Task MEMSET cleanup..",
         vs->vserver_config->vserver_name);  

  if (vs->v_map != NULL) {

    runner = g_list_first(vs->v_map->members);

    while (runner != NULL) {
      member = (struct rahunas_member *) runner->data; 

      rh_free_member(member);

      runner = g_list_delete_link(runner, runner);
    }

    g_list_free(vs->v_map->members);

    rh_free(&(vs->v_map->members));
    rh_free(&(vs->v_map));
  }

  return 0;
}


/* Start service task */
static int startservice (struct vserver *vs)
{
  /* Do nothing or need to implement */
}

/* Stop service task */
static int stopservice  (struct vserver *vs)
{
  /* Do nothing or need to implement */
}

/* Start session task */
static int startsess (struct vserver *vs, struct task_req *req)
{
  uint32_t id = req->id;
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;

  member_node = member_get_node_by_id(vs, id);

  if (member_node == NULL) {
    DP("Create new member");
    member = (struct rahunas_member *)rh_malloc(sizeof(struct rahunas_member));
    if (member == NULL)
      return (-1);

    memset(member, 0, sizeof(struct rahunas_member));

    vs->v_map->members = 
      g_list_insert_sorted(vs->v_map->members, member, idcmp);
  } else {
    DP("Member already exists");
    member = (struct rahunas_member *)member_node->data;
  }

  member->id = id; 

  if (member->username && member->username != termstring)
    free(member->username);

  if (member->session_id && member->username != termstring)
    free(member->session_id);

  member->username   = strdup(req->username);
  if (!member->username)
    member->username = termstring;

  member->session_id = strdup(req->session_id);
  if (!member->session_id)
    member->session_id = termstring;

  time(&(member->session_start));
  memcpy(&req->session_start, &member->session_start, 
         sizeof(time_t));

  memcpy(&member->mac_address, &req->mac_address, ETH_ALEN);

  memcpy(&member->session_timeout, &req->session_timeout, 
         sizeof(time_t));
  member->bandwidth_max_down = req->bandwidth_max_down;
  member->bandwidth_max_up = req->bandwidth_max_up;
 
  DP("Session-Timeout %d", req->session_timeout);
  DP("Bandwidth - Down: %lu, Up: %lu", req->bandwidth_max_down, 
       req->bandwidth_max_up);

  logmsg(RH_LOG_NORMAL, "[%s] Session Start, User: %s, IP: %s, "
                        "Session ID: %s, MAC: %s",
                        vs->vserver_config->vserver_name,
                        member->username, 
                        idtoip(vs->v_map, id), 
                        member->session_id,
                        mac_tostring(member->mac_address)); 
  return 0;
}

/* Stop session task */
static int stopsess  (struct vserver *vs, struct task_req *req)
{
  uint32_t id = req->id;
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;
  char cause[16] = "";

  member_node = member_get_node_by_id(vs, id);
  if (member_node == NULL)
    return (-1);

  member = (struct rahunas_member *)member_node->data;

  switch (req->req_opt) {
    case RH_RADIUS_TERM_IDLE_TIMEOUT :
      strcpy(cause, "idle timeout");
      break;
    case RH_RADIUS_TERM_SESSION_TIMEOUT :
      strcpy(cause, "session timeout");
      break;
    case RH_RADIUS_TERM_USER_REQUEST :
      strcpy(cause, "user request");
      break;
    case RH_RADIUS_TERM_NAS_REBOOT :
      strcpy(cause, "nas reboot");
      break;
    case RH_RADIUS_TERM_ADMIN_RESET :
      strcpy(cause, "admin reset");
      break;
  }
  if (!member->username)
    member->username = termstring;

  if (!member->session_id)
    member->session_id = termstring;

  logmsg(RH_LOG_NORMAL, "[%s] Session Stop (%s), User: %s, IP: %s, "
                        "Session ID: %s, MAC: %s",
                        vs->vserver_config->vserver_name,
                        cause,
                        member->username, 
                        idtoip(vs->v_map, id), 
                        member->session_id,
                        mac_tostring(member->mac_address)); 

  rh_free_member(member);   

  vs->v_map->members = g_list_delete_link(vs->v_map->members, member_node);

  return 0;
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

void rh_task_memset_reg(struct main_server *ms) {
  task_register(ms, &task_memset);
}
