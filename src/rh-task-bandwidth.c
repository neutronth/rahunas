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


/* MAX_SLOT_PAGE is calculated from the formula of
   MAX_SLOT_PAGE = ceil(MAX_SLOT_ID/PAGE_SIZE)

   where
     PAGE_SIZE = sizeof(short) * 8
*/

#define MAX_SLOT_ID    9998
#define PAGE_SIZE      16
#define MAX_SLOT_PAGE  625

#define BANDWIDTH_WRAPPER "/etc/rahunas/bandwidth.sh"

static unsigned short slot_flags[MAX_SLOT_PAGE] = {1};
static unsigned short slot_count = 0;

struct bandwidth_req {
  char slot_id[5];
  char ip[16];
  char bandwidth_max_down[15];
  char bandwidth_max_up[15];
};

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

void _mark_reserved_slot_id(unsigned int slot_id)
{
  unsigned short page       = 0;
  unsigned char  id_on_page = 0;

  page = slot_id / PAGE_SIZE; 
  id_on_page = slot_id % PAGE_SIZE;

  slot_count++;
  slot_flags[page] |= 1 << id_on_page;
}

int _bandwidth_exec(char *const args[])
{
  pid_t ws;
  pid_t pid;
  int status;
  int exec_pipe[2];
  char buffer[150];
  char *endline = NULL;
  int ret = 0;
  
  memset(buffer, '\0', sizeof(buffer));

  if (pipe(exec_pipe) == -1) {
    logmsg(RH_LOG_ERROR, "Error: pipe()");
    return -1;
  }

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
    read(exec_pipe[0], buffer, sizeof(buffer));
    if (buffer != NULL) {
      endline = strstr(buffer, "\n");
      if (endline != NULL) 
        *endline = '\0';

      logmsg(RH_LOG_NORMAL, "Bandwidth: %s", buffer);
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

int _bandwidth_start()
{
  char *args[3];

  DP("Bandwidth: start");

  args[0] = BANDWIDTH_WRAPPER;
  args[1] = "start";
  args[2] = (char *) 0;

  return _bandwidth_exec(args);
}

int _bandwidth_stop()
{
  char *args[3];

  DP("Bandwidth: stop");

  args[0] = BANDWIDTH_WRAPPER;
  args[1] = "stop";
  args[2] = (char *) 0;

  return _bandwidth_exec(args);
}

int _bandwidth_add(struct bandwidth_req *bw_req)
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

  return _bandwidth_exec(args);
}

int _bandwidth_del(struct bandwidth_req *bw_req)
{
  char *args[4];

  DP("Bandwidth: request %s", bw_req->slot_id);

  args[0] = BANDWIDTH_WRAPPER;
  args[1] = "del";
  args[2] = bw_req->slot_id;
  args[3] = (char *) 0;

  return _bandwidth_exec(args);
}

/* Initialize */
static void init (void)
{
  logmsg(RH_LOG_NORMAL, "Task BANDWIDTH init..");  
}

/* Cleanup */
static void cleanup (void)
{
  logmsg(RH_LOG_NORMAL, "Task BANDWIDTH cleanup..");  
}

/* Start service task */
static int startservice (struct rahunas_map *map)
{
  return _bandwidth_start();
}

/* Stop service task */
static int stopservice  (struct rahunas_map *map)
{
  return _bandwidth_stop();
}

/* Start session task */
static int startsess (struct rahunas_map *map, struct task_req *req)
{
  struct bandwidth_req bw_req;
  unsigned short slot_id;
  unsigned char max_try = 3;

  if (map->members[req->id].bandwidth_max_down == 0 && 
      map->members[req->id].bandwidth_max_up == 0)
    return 0;

  if (map->members[req->id].bandwidth_slot_id > 0)
    return 0;
  
  // Formating the bandwidth request
  sprintf(bw_req.ip, "%s", idtoip(map, req->id));
  sprintf(bw_req.bandwidth_max_down, "%lu", 
    map->members[req->id].bandwidth_max_down);
  sprintf(bw_req.bandwidth_max_up, "%lu", 
    map->members[req->id].bandwidth_max_up);
  
  while (max_try > 0) { 
    slot_id = _get_slot_id();
    sprintf(bw_req.slot_id, "%d", slot_id);

    if (_bandwidth_add(&bw_req) == 0)
      break;
    else
      max_try--;
  }

  if (max_try == 0) {
    logmsg(RH_LOG_ERROR, "Bandwidth: Maximum trying, failed!");
    return -1;
  }

  _mark_reserved_slot_id(slot_id);
  map->members[req->id].bandwidth_slot_id = slot_id;
 
  return 0;
}

/* Stop session task */
static int stopsess  (struct rahunas_map *map, struct task_req *req)
{
  struct bandwidth_req bw_req;
  unsigned short slot_id = map->members[req->id].bandwidth_slot_id;

  if (slot_id < 1)
    return 0;

  sprintf(bw_req.slot_id, "%d", slot_id);

  if (_bandwidth_del(&bw_req) == 0) {
    map->members[req->id].bandwidth_slot_id = 0;

    if (slot_count > 0)
      slot_count--;
  
    return 0; 
  } else {
    return -1;
  }
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

static struct task task_bandwidth = {
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

void rh_task_bandwidth_reg(void) {
  task_register(&task_bandwidth);
}
