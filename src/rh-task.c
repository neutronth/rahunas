/**
 * RahuNAS task implementation
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-09-07
 */

#include <syslog.h>
#include "rh-task.h"
#include "rh-task-iptables.h"
#include "rh-task-memset.h"
#include "rh-task-ipset.h"
#include "rh-task-dbset.h"
#include "rh-task-bandwidth.h"

void task_register(struct main_server *ms, struct task *task)
{
  GList *chk  = NULL;
  GList *node = NULL;
  struct task *ltask = NULL;

  if (task == NULL)
     return;

  DP("Registering Task: %s", task->taskname);

  chk = g_list_find(ms->task_list, task);

  /* Already register */
  if (chk != NULL) {
    DP("Already registered");
    return;
  }

  if (ms->task_list == NULL) {
    ms->task_list = g_list_append(ms->task_list, task);
    return;
  }

  for (node = g_list_first(ms->task_list); node != NULL; node = g_list_next(node)) {
    ltask = (struct task *)node->data;
    if (task->taskprio >= ltask->taskprio)
      break;  
  }
  
  ms->task_list = g_list_insert_before(ms->task_list, node, task);
}

void rh_task_register(struct main_server *ms)
{
  static int task_registered = 0;

  if (task_registered == 0) {
    /* Register all tasks */
    rh_task_iptables_reg(ms);
    rh_task_memset_reg(ms);
    rh_task_ipset_reg(ms);

    if (ms->main_config->serviceclass)
      rh_task_serviceclass_reg(ms);

    if (ms->main_config->bandwidth_shape)
      rh_task_bandwidth_reg(ms);

    rh_task_dbset_reg(ms);
    task_registered = 1;
  }
}

void rh_task_unregister(struct main_server *ms) {
  g_list_free(ms->task_list);
}

int  rh_task_startservice(struct main_server *ms)
{
  GList *runner = g_list_first(ms->task_list);
  struct task *ltask = NULL;

  DP("Start service");

  while (runner != NULL) {
    ltask = (struct task *)runner->data;
    (*ltask->startservice)();
    runner = g_list_next(runner);
  }

  logmsg(RH_LOG_NORMAL, "Service started");
  return 0;
}

int rh_task_stopservice(struct main_server *ms)
{  
  GList *runner = g_list_last(ms->task_list);
  struct task *ltask = NULL;

  DP("Stop service");

  while (runner != NULL) {
    ltask = (struct task *)runner->data;
    (*ltask->stopservice)();
    runner = g_list_previous(runner);
  }

  logmsg(RH_LOG_NORMAL, "Service stopped");
  return 0;
}

void rh_task_init (struct main_server *ms, struct vserver *vs)
{
  GList *runner = g_list_first(ms->task_list);
  struct task *ltask = NULL;

  DP("Initialize...");

  while (runner != NULL) {
    ltask = (struct task *)runner->data;
    (*ltask->init)(vs);
    runner = g_list_next(runner);
  }
}

void rh_task_cleanup(struct main_server *ms, struct vserver *vs)
{
  GList *runner = g_list_last(ms->task_list);
  struct task *ltask = NULL;

  DP("Task Cleanup");

  while (runner != NULL) {
    ltask = (struct task *)runner->data;
    (*ltask->cleanup)(vs);
    runner = g_list_previous(runner);
  }  
}

int  rh_task_startsess(struct vserver *vs, struct task_req *req)
{
  struct main_server *ms = &rh_main_server_instance;
  GList *runner = g_list_first(ms->task_list);
  struct task *ltask = NULL;

  DP("Start session called");

  while (runner != NULL) {
    ltask = (struct task *)runner->data;
    (*ltask->startsess)(vs, req);
    runner = g_list_next(runner);
  }  

  return 0;
}

int  rh_task_stopsess(struct vserver *vs, struct task_req *req)
{
  struct main_server *ms = &rh_main_server_instance;
  GList *runner = g_list_last(ms->task_list);
  struct task *ltask = NULL;

  DP("Stop session called");
 
  while (runner != NULL) {
    ltask = (struct task *)runner->data;
    (*ltask->stopsess)(vs, req);
    runner = g_list_previous(runner);
  }  

  return 0;
}

int  rh_task_commitstartsess(struct vserver *vs, struct task_req *req)
{
}

int  rh_task_commitstopsess(struct vserver *vs, struct task_req *req)
{
}

int  rh_task_rollbackstartsess(struct vserver *vs, struct task_req *req)
{
}

int  rh_task_rollbackstopsess(struct vserver *vs, struct task_req *req)
{
}
