/**
 * RahuNAS task implementation
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-09-07
 */

#include <syslog.h>
#include "rh-task.h"
#include "rh-task-memset.h"
#include "rh-task-ipset.h"

void task_register(struct task *task)
{
  GList *chk  = NULL;
  GList *node = NULL;
  struct task *ltask = NULL;

  if (task == NULL)
     return;

  DP("Registering Task: %s", task->taskname);

  chk = g_list_find(task_list, task);

  /* Already register */
  if (chk != NULL) {
    DP("Already registered");
    return;
  }

  if (task_list == NULL) {
    task_list = g_list_append(task_list, task);
    return;
  }

  for (node = g_list_first(task_list); node != NULL; node = g_list_next(node)) {
    ltask = (struct task *)node->data;
    if (task->taskprio >= ltask->taskprio)
      break;  
  }
  
  task_list = g_list_insert_before(task_list, node, task);
}

static void rh_task_call_init (void)
{
  GList *runner = g_list_first(task_list);
  struct task *ltask = NULL;

  DP("Initialize...");

  while (runner != NULL) {
    ltask = (struct task *)runner->data;
    (*ltask->init)();
    runner = g_list_next(runner);
  }
}

void rh_task_init(void)
{
  /* Register all tasks */
  rh_task_ipset_reg();
  rh_task_memset_reg();
  rh_task_dbset_reg();

  /* Call each init */
  rh_task_call_init();
}

void rh_task_cleanup(void)
{
  GList *runner = g_list_last(task_list);
  struct task *ltask = NULL;

  DP("Task Cleanup");

  while (runner != NULL) {
    ltask = (struct task *)runner->data;
    (*ltask->cleanup)();
    runner = g_list_previous(runner);
  }  

  g_list_free(task_list);
}

int  rh_task_startservice(struct rahunas_map *map)
{
  GList *runner = g_list_first(task_list);
  struct task *ltask = NULL;

  DP("Start service");

  while (runner != NULL) {
    ltask = (struct task *)runner->data;
    (*ltask->startservice)(map);
    runner = g_list_next(runner);
  }  

  logmsg(RH_LOG_NORMAL, "Service started");
  return 0;
}

int  rh_task_stopservice(struct rahunas_map *map)
{  
  GList *runner = g_list_last(task_list);
  struct task *ltask = NULL;

  DP("Stop service");

  while (runner != NULL) {
    ltask = (struct task *)runner->data;
    (*ltask->stopservice)(map);
    runner = g_list_previous(runner);
  }  

  logmsg(RH_LOG_NORMAL, "Service stopped");
}

int  rh_task_startsess(struct rahunas_map *map, struct task_req *req)
{
  GList *runner = g_list_first(task_list);
  struct task *ltask = NULL;

  DP("Start session called");

  while (runner != NULL) {
    ltask = (struct task *)runner->data;
    (*ltask->startsess)(map, req);
    runner = g_list_next(runner);
  }  

  return 0;
}

int  rh_task_stopsess(struct rahunas_map *map, struct task_req *req)
{
  GList *runner = g_list_last(task_list);
  struct task *ltask = NULL;

  DP("Stop session called");

  while (runner != NULL) {
    ltask = (struct task *)runner->data;
    (*ltask->stopsess)(map, req);
    runner = g_list_previous(runner);
  }  

  return 0;
}

int  rh_task_commitstartsess(struct rahunas_map *map, struct task_req *req)
{
}

int  rh_task_commitstopsess(struct rahunas_map *map, struct task_req *req)
{
}

int  rh_task_rollbackstartsess(struct rahunas_map *map, struct task_req *req)
{
}

int  rh_task_rollbackstopsess(struct rahunas_map *map, struct task_req *req)
{
}
