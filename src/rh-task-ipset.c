/**
 * RahuNAS task memset implementation 
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 *         Suriya Soutmun <darksolar@gmail.com>
 * Date:   2008-09-07
 */

#include <string.h>
#include <syslog.h>

#include "rahunasd.h"
#include "rh-ipset.h"
#include "rh-task.h"
#include "rh-xmlrpc-cmd.h"

static int
set_cleanup(void *data)
{
  struct processing_set *process = (struct processing_set *) data;
  struct ip_set_list *setlist = (struct ip_set_list *) process->list;
  size_t offset;
  struct ip_set_rahu *table = NULL;
  struct task_req req;
  unsigned int id;
  char *ip = NULL;
  int res  = 0;
  GList *runner = g_list_first(process->vs->v_map->members);
  struct rahunas_member *member = NULL;

  if (process == NULL)
    return (-1);

  offset = sizeof(struct ip_set_list) + setlist->header_size;
  table = (struct ip_set_rahunas *)(process->list + offset);

  while (runner != NULL) {
    member = (struct rahunas_member *) runner->data;
    runner = g_list_next(runner);

    id = member->id;

    DP("Found IP: %s in set, try logout", idtoip(process->vs->v_map, id));
    req.id = id;
    memcpy(req.mac_address, table[id].ethernet, ETH_ALEN);
    req.req_opt = RH_RADIUS_TERM_NAS_REBOOT;
    send_xmlrpc_stopacct(process->vs, id, RH_RADIUS_TERM_NAS_REBOOT);
    rh_task_stopsess(process->vs, &req);
  }
}

/* Start service task */
static int startservice ()
{
  return 0;
}

/* Stop service task */
static int stopservice  ()
{
  return 0;
}

/* Initialize */
static void init (RHVServer *vs)
{
  if (vs->vserver_config->init_flag == VS_RELOAD)
    return;

  vs->v_set = set_adt_get(vs->vserver_config->vserver_name);

  logmsg(RH_LOG_NORMAL, "[%s] Task IPSET initialize..",
         vs->vserver_config->vserver_name);  

  DP("getsetname: %s", vs->v_set->name);
  DP("getsetid: %d", vs->v_set->id);
  DP("getsetindex: %d", vs->v_set->index);

  /* Ensure the set is empty */
  set_flush(vs->vserver_config->vserver_name);
}

/* Cleanup */
static void cleanup (RHVServer *vs)
{
  if (vs->vserver_config->init_flag == VS_RELOAD)
    return;

  logmsg(RH_LOG_NORMAL, "[%s] Task IPSET cleanup..",
         vs->vserver_config->vserver_name);  

  walk_through_set(&set_cleanup, vs);

  set_flush(vs->vserver_config->vserver_name);
  rh_free((void **) &vs->v_set);
}

/* Start session task */
static int startsess (RHVServer *vs, struct task_req *req)
{
  int res = 0;
  ip_set_ip_t ip;
  parse_ip(idtoip(vs->v_map, req->id), &ip);

  res = set_adtip_nb(vs->v_set, &ip, req->mac_address, IP_SET_OP_ADD_IP);

  return res;
}

/* Stop session task */
static int stopsess  (RHVServer *vs, struct task_req *req)
{
  int res = 0;
  ip_set_ip_t ip;
  parse_ip(idtoip(vs->v_map, req->id), &ip);

  res = set_adtip_nb(vs->v_set, &ip, req->mac_address, IP_SET_OP_DEL_IP);

  return res;
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

static struct task task_ipset = {
  .taskname = "IPSET",
  .taskprio = 30,
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

void rh_task_ipset_reg(RHMainServer *ms) {
  task_register(ms, &task_ipset);
}
