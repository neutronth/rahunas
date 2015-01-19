/**
 * RahuNAS task bandwidth implementation 
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-11-20
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

int iptables_exec(RHVServer *vs, char *const args[])
{
  int  ret = 0;
  char *env[22];
  int env_size = (sizeof (env) / sizeof (char *));
  int  i = 0;

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
  env[20] = g_strdup_printf("KEEP_SET=%s",
                            vs->vserver_config->init_flag == VS_RELOAD ?
                            "yes" : "no");
  env[21] = (char *) 0;

  for (i = 0; i < env_size; i++) {
    if (env[i] != NULL) 
      DP("%s", env[i]);
  }
  
  ret =  rh_cmd_exec (RAHUNAS_FIREWALL_WRAPPER, args, env, NULL, 0);
 
  for (i = 0; i < env_size; i++) {
    g_free(env[i]);
  } 
  
  return ret;
}

int iptables_start(RHVServer *vs)
{
  char *args[3];

  DP("IPTables: start");

  args[0] = RAHUNAS_FIREWALL_WRAPPER;
  args[1] = "start-config";
  args[2] = (char *) 0;

  return iptables_exec(vs, args);
}

int iptables_stop(RHVServer *vs)
{
  char *args[3];

  DP("IPTables: stop");

  args[0] = RAHUNAS_FIREWALL_WRAPPER;
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
static void init (RHVServer *vs)
{
  logmsg(RH_LOG_NORMAL, "[%s] Task IPTABLES initialize..", 
         vs->vserver_config->vserver_name);  

  iptables_start(vs);
}

/* Cleanup */
static void cleanup (RHVServer *vs)
{
  logmsg(RH_LOG_NORMAL, "[%s] Task IPTABLES cleanup..",
         vs->vserver_config->vserver_name);  

  iptables_stop(vs);
}

/* Start session task */
static int startsess (RHVServer *vs, struct task_req *req)
{
  /* Do nothing */
  return 0;
}

/* Stop session task */
static int stopsess  (RHVServer *vs, struct task_req *req)
{
  /* Do nothing */
  return 0; 
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

static struct task taskiptables = {
  .taskname = "IPTABLES",
  .taskprio = 50,
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

void rh_task_iptables_reg(RHMainServer *ms) {
  task_register(ms, &taskiptables);
}
