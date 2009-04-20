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
#include "rh-utils.h"

#define IPTABLES_WRAPPER "/etc/rahunas/firewall.sh"

int iptables_exec(struct vserver *vs, char *const args[])
{
  pid_t ws;
  pid_t pid;
  int status;
  int exec_pipe[2];
  char buffer[150];
  char *endline = NULL;
  int ret = 0;
  int fd = 0;
  int i = 0;
  char *env[21];

  env[0]  = g_strdup("ENV_OVERRIDE=yes");
  env[1]  = g_strdup_printf("SETNAME=%s", vs->vserver_config->vserver_name);
  env[2]  = g_strdup_printf("VSERVER_ID=%d", vs->vserver_config->vserver_id);
  env[3]  = g_strdup_printf("DEV_EXTERNAL=%s",vs->vserver_config->dev_external);
  env[4]  = g_strdup_printf("DEV_INTERNAL=%s",vs->vserver_config->dev_internal);
  env[5]  = g_strdup_printf("BRIDGE=%s", vs->vserver_config->bridge);
  env[6]  = g_strdup_printf("MASQUERADE=%s", vs->vserver_config->masquerade);
  env[7]  = g_strdup_printf("IGNORE_MAC=%s", vs->vserver_config->ignore_mac);
  env[8]  = g_strdup_printf("VSERVER_IP=%s", vs->vserver_config->vserver_ip);
  env[9]  = g_strdup_printf("CLIENTS=%s", vs->vserver_config->clients);
  env[10] = g_strdup_printf("EXCLUDED=%s", vs->vserver_config->excluded);
  env[11] = g_strdup_printf("DNS=%s", vs->vserver_config->dns);
  env[12] = g_strdup_printf("SSH=%s", vs->vserver_config->ssh);
  env[13] = g_strdup_printf("PROXY=%s", vs->vserver_config->proxy);
  env[14] = g_strdup_printf("PROXY_HOST=%s", vs->vserver_config->proxy_host);
  env[15] = g_strdup_printf("PROXY_PORT=%s", vs->vserver_config->proxy_port);
  env[16] = g_strdup_printf("BITTORRENT=%s", vs->vserver_config->bittorrent);
  env[17] = g_strdup_printf("BITTORRENT_ALLOW=%s", 
                            vs->vserver_config->bittorrent_allow);
  env[18] = g_strdup_printf("VSERVER_PORTS_ALLOW=%s", 
                            vs->vserver_config->vserver_ports_allow);
  env[19] = g_strdup_printf("VSERVER_PORTS_INTERCEPT=%s", 
                            vs->vserver_config->vserver_ports_intercept);
  env[20] = (char *) 0;

  for (i = 0; i < 21; i++) {
    if (env[i] != NULL) 
      DP("%s", env[i]);
  }
  
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
    execve(IPTABLES_WRAPPER, args, env);
  } else if (pid < 0) {
    // Fork error
    logmsg(RH_LOG_ERROR, "Error: vfork()"); 
    ret = -1;
  } else {
    // Parent
    ws = waitpid(pid, &status, 0);

    DP("IPTables: Return (%d)", WEXITSTATUS(status));

    if (WIFEXITED(status)) {
      ret = WEXITSTATUS(status);
    } else {
      ret = -1;
    } 
  }

  close(exec_pipe[0]);
  close(exec_pipe[1]);

 
  for (i = 0; i < 21; i++) {
    g_free(env[i]);
  } 
  
  return ret;
}

int iptables_start(struct vserver *vs)
{
  char *args[3];

  DP("IPTables: start");

  args[0] = IPTABLES_WRAPPER;
  args[1] = "start-config";
  args[2] = (char *) 0;

  return iptables_exec(vs, args);
}

int iptables_stop(struct vserver *vs)
{
  char *args[3];

  DP("IPTables: stop");

  args[0] = IPTABLES_WRAPPER;
  args[1] = "stop-config";
  args[2] = (char *) 0;

  return iptables_exec(vs, args);
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
static void init (struct vserver *vs)
{
  logmsg(RH_LOG_NORMAL, "[%s] Task IPTABLES initialize..", 
         vs->vserver_config->vserver_name);  

  iptables_start(vs);
}

/* Cleanup */
static int cleanup (struct vserver *vs)
{
  logmsg(RH_LOG_NORMAL, "[%s] Task IPTABLES cleanup..",
         vs->vserver_config->vserver_name);  
  iptables_stop(vs);
}

/* Start session task */
static int startsess (struct vserver *vs, struct task_req *req)
{
  /* Do nothing */
  return 0;
}

/* Stop session task */
static int stopsess  (struct vserver *vs, struct task_req *req)
{
  /* Do nothing */
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

static struct task taskiptables = {
  .taskname = "IPTABLES",
  .taskprio = 50,
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

void rh_task_iptables_reg(struct main_server *ms) {
  task_register(ms, &taskiptables);
}
