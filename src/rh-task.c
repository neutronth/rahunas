/**
 * RahuNAS task implementation
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-09-07
 */

#include <syslog.h>
#include "rh-task.h"
#include "rh-task-memset.h"
#include "rh-task-ipset.h"

static struct task dummy;

static struct task *task_find(const char *taskname)
{
  struct task *runner = get_task_list(); 
  while(runner != NULL) {
    if (strncmp(runner->taskname, taskname, RH_TASK_MAXNAMELEN) == 0)
      return runner;
    
    runner = runner->next;
  }

  return NULL;
}

struct task *get_task_list ()
{
  if (task_list == NULL)
    return NULL;
  else
    return task_list->next; /* start after dummy */
}

void task_register(struct task *task)
{
  struct task *chk, *prev = NULL;
  struct task *runner = task_list;

  if (task == NULL)
     return;

  DP("Registering Task: %s", task->taskname);

  chk = task_find(task->taskname);

  /* Already register */
  if (chk != NULL) {
    DP("Already registered");
    return;
  }
 
  while (runner->next != NULL && (task->taskprio < runner->next->taskprio)) {
    runner = runner->next;
  }

  task->next = runner->next;
  runner->next = task;
}

static void rh_task_call_init (void)
{
  struct task *runner = get_task_list();
  while(runner != NULL) {
    (*runner->init)();
    runner = runner->next;
  }
}

void rh_task_init(void)
{
  strncpy(dummy.taskname, "DUMMY", 6);
  dummy.taskprio = 999; /* ensure it always be head */
  dummy.next = NULL;

  task_list = &dummy;

  /* Register all tasks */
  rh_task_ipset_reg();
  rh_task_memset_reg();

  /* Call each init */
  rh_task_call_init();
}

void rh_task_cleanup(void)
{
  struct task *runner = get_task_list();

  DP("Task Cleanup");

  if (runner == NULL)
    return;

  while (runner != NULL) {
    (*runner->cleanup)();
    runner = runner->next;
  }
}

int  rh_task_startservice(struct rahunas_map *map)
{
  struct task *runner = get_task_list();

  DP("Start service");

  if (runner == NULL)
    return 0;

  while (runner != NULL) {
    (*runner->startservice)(map);
    runner = runner->next;
  }
  
  logmsg(RH_LOG_NORMAL, "Service start");
  return 0;
}

int  rh_task_stopservice(struct rahunas_map *map)
{  
  struct task *runner = get_task_list();

  DP("Stop service");

  if (runner == NULL)
    return 0;

  while (runner != NULL) {
    (*runner->stopservice)(map);
    runner = runner->next;
  }
}

int  rh_task_startsess(struct rahunas_map *map, struct task_req *req)
{
  struct task *runner = get_task_list();

  DP("Start session called");

  if (runner == NULL)
    return 0;

  while (runner != NULL) {
    if ((*runner->startsess)(map, req) < 0) {
      DP("Failed, rollback");
    }
    runner = runner->next;
  }
  
  return 0;
}

int  rh_task_stopsess(struct rahunas_map *map, struct task_req *req)
{
  struct task *runner = get_task_list();

  DP("Stop session called");

  if (runner == NULL)
    return 0;

  while (runner != NULL) {
    if ((*runner->stopsess)(map, req) < 0) {
      DP("Failed, rollback");
    }
    runner = runner->next;
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
