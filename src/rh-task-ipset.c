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

  int retry = 3;

  while (send_xmlrpc_offacct (vs) != 0 && --retry > 0) {
    sleep (5);
  }

  if (retry < 0) {
    /* TODO: Accounting-Off failed, should be checked on next service start,
     *       stop-delay should be added.
     */
  }

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

/* Update session task */
static int updatesess (RHVServer *vs, struct task_req *req)
{
  /* Do nothing */
  return 0;
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
  .updatesess = &updatesess,
  .commitstartsess = &commitstartsess,
  .commitstopsess = &commitstopsess,
  .rollbackstartsess = &rollbackstartsess,
  .rollbackstopsess = &rollbackstopsess,
};

void rh_task_ipset_reg(RHMainServer *ms) {
  task_register(ms, &task_ipset);
}
