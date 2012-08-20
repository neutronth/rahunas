/**
 * RahuNAS task conntrack implementation
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2012-06-17
 */

#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "rahunasd.h"
#include "rh-task.h"
#include "rh-utils.h"

int conntrack_exec(RHVServer *vs, char *const args[])
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
    execv(RAHUNAS_CONNTRACK_WRAPPER, args);
  } else if (pid < 0) {
    // Fork error
    logmsg(RH_LOG_ERROR, "Error: vfork()");
    ret = -1;
  } else {
    // Parent
    ws = waitpid(pid, &status, 0);

    DP("Connection Tracking: Return (%d)", WEXITSTATUS(status));

    // Return message log
    DP("Read message");
    read(exec_pipe[0], buffer, sizeof(buffer));

    if (buffer != NULL) {
      DP("Got message: %s", buffer);
      endline = strstr(buffer, "\n");
      if (endline != NULL)
        *endline = '\0';

      if (vs != NULL) {
        logmsg(RH_LOG_NORMAL, "[%s] Connection Tracking: %s",
          vs->vserver_config->vserver_name, buffer);
      }
    }

    if (WIFEXITED(status)) {
      ret = WEXITSTATUS(status);
    } else {
      ret = -1;
    }
  }

  if ((buffer != NULL) && (strncmp (buffer, "NOT COMPLETED", 13) == 0))
    ret = -2;  // Not complete need to retry

  close(exec_pipe[0]);
  close(exec_pipe[1]);
  return ret;
}

int conntrack_flush(RHVServer *vs, struct task_req *req)
{
  char *args[4];
  char ip[16];

  snprintf (ip, sizeof (ip), "%s", idtoip (vs->v_map, req->id));

  DP("Connection Tracking: flush");

  args[0] = RAHUNAS_CONNTRACK_WRAPPER;
  args[1] = "cut";
  args[2] = ip;
  args[3] = (char *) 0;

  return conntrack_exec(vs, args);
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
  /* Do nothing */
}

/* Cleanup */
static void cleanup (RHVServer *vs)
{
  /* Do nothing */
}

/* Start session task */
static int startsess (RHVServer *vs, struct task_req *req)
{
  return conntrack_flush (vs, req);
}

/* Stop session task */
static int stopsess  (RHVServer *vs, struct task_req *req)
{
  return conntrack_flush (vs, req);
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

static struct task taskconntrack = {
  .taskname = "CONNTRACK",
  .taskprio = 21,
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

void rh_task_conntrack_reg(RHMainServer *ms) {
  task_register(ms, &taskconntrack);
}
