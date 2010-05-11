/**
 * RahuNAS task vipmap implementation 
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2010-05-11
 */

#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <libgda/libgda.h>
#include "rahunasd.h"
#include "rh-task.h"
#include "rh-task-vipmap.h"
#include "rh-task-memset.h"
#include "rh-ipset.h"
#include "rh-utils.h"

GList *vipmap = NULL;

int vipmap_exec(struct vserver *vs, char *const args[])
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
    execv(RAHUNAS_VIPMAP_WRAPPER, args);
  } else if (pid < 0) {
    // Fork error
    logmsg(RH_LOG_ERROR, "Error: vfork()"); 
    ret = -1;
  } else {
    // Parent
    ws = waitpid(pid, &status, 0);

    DP("VIP map: Return (%d)", WEXITSTATUS(status));

    // Return message log
    DP("Read message");
    read(exec_pipe[0], buffer, sizeof(buffer));

    if (buffer != NULL) {
      DP("Got message: %s", buffer);
      endline = strstr(buffer, "\n");
      if (endline != NULL) 
        *endline = '\0';

      if (vs != NULL) {
        logmsg(RH_LOG_NORMAL, "[%s] VIP map: %s", 
          vs->vserver_config->vserver_name, buffer);
      } else {
        logmsg(RH_LOG_NORMAL, "[main server] VIP map: %s", buffer);
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

int vipmap_add(struct vserver *vs, struct vipmap_req *req)
{
  char *args[6];

  DP("VIP map: add");

  args[0] = RAHUNAS_VIPMAP_WRAPPER;
  args[1] = "add";
  args[2] = vs->vserver_config->vserver_name; 
  args[3] = req->ip;
  args[4] = req->vip_ip;
  args[5] = (char *) 0;

  return vipmap_exec (vs, args);
}

int vipmap_del(struct vserver *vs, struct vipmap_req *req)
{
  char *args[6];

  DP("VIP map: add");

  args[0] = RAHUNAS_VIPMAP_WRAPPER;
  args[1] = "del";
  args[2] = vs->vserver_config->vserver_name; 
  args[3] = req->ip;
  args[4] = req->vip_ip;
  args[5] = (char *) 0;

  return vipmap_exec (vs, args);
}


/* Start service task */
static int startservice ()
{
  /* Do nothing */
  return 0;
}

/* Stop service task */
static int stopservice  ()
{
  /* Do nothing */
  return 0;
}

/* Initialize */
static void init (struct vserver *vs)
{
  /* Do nothing */
}

/* Cleanup */
static void cleanup (struct vserver *vs)
{
  /* Do nothing */
}

/* Start session task */
static int startsess (struct vserver *vs, struct task_req *req)
{
  struct vipmap_req vip_req;
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;

  member_node = member_get_node_by_id(vs, req->id);
  if (member_node == NULL)
    return -1;

  member = (struct rahunas_member *) member_node->data;

  snprintf(vip_req.ip, sizeof (vip_req.ip), "%s", idtoip(vs->v_map, req->id));
  snprintf(vip_req.vip_ip, sizeof (vip_req.vip_ip), "%s",
           idtoip(vs->v_map, req->id));

  vipmap_add(vs, &vip_req);
  return 0;
}

/* Stop session task */
static int stopsess (struct vserver *vs, struct task_req *req)
{
  struct vipmap_req vip_req;
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;

  member_node = member_get_node_by_id(vs, req->id);
  if (member_node == NULL)
    return -1;

  member = (struct rahunas_member *) member_node->data;

  snprintf(vip_req.ip, sizeof (vip_req.ip), "%s", idtoip(vs->v_map, req->id));
  snprintf(vip_req.vip_ip, sizeof (vip_req.vip_ip), "%s",
           idtoip(vs->v_map, req->id));

  vipmap_del(vs, &vip_req);

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

static struct task task_vipmap = {
  .taskname = "VIPMAP",
  .taskprio = 15,
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

void rh_task_vipmap_reg(struct main_server *ms) {
  task_register(ms, &task_vipmap);
}
