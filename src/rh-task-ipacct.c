/**
 * RahuNAS task IP accounting implementation
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2014-07-19
 */

#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include "rahunasd.h"
#include "rh-task.h"
#include "rh-task-ipacct.h"
#include "rh-utils.h"

static int ipacct_exec(RHVServer *vs, char *const args[])
{
  int  ret = 0;
  char *env[6];
  int env_size = (sizeof (env) / sizeof (char *));
  int  i = 0;

  env[0]  = g_strdup("ENV_OVERRIDE=yes");
  env[1]  = g_strdup_printf("SETNAME=%s", vs->vserver_config->vserver_name);
  env[2]  = g_strdup_printf("VSERVER_ID=%d", vs->vserver_config->vserver_id);
  env[3]  = g_strdup_printf("DEV_INTERNAL=%s",vs->vserver_config->dev_internal);
  env[4]  = g_strdup_printf("CLIENTS=%s", vs->vserver_config->clients);
  env[5] = (char *) 0;

  for (i = 0; i < env_size; i++) {
    if (env[i] != NULL)
      DP("%s", env[i]);
  }

  ret =  rh_cmd_exec (RAHUNAS_IPACCT_WRAPPER, args, env, NULL, 0);

  for (i = 0; i < env_size; i++) {
    g_free(env[i]);
  }

  return ret;
}

static int ipacct_start(RHVServer *vs)
{
  char *args[3];

  DP("IP Accounting: start");

  args[0] = RAHUNAS_IPACCT_WRAPPER;
  args[1] = "start-config";
  args[2] = (char *) 0;

  return ipacct_exec(vs, args);
}

static int ipacct_stop(RHVServer *vs)
{
  char *args[3];

  DP("IP Accounting: stop");

  args[0] = RAHUNAS_IPACCT_WRAPPER;
  args[1] = "stop-config";
  args[2] = (char *) 0;

  return ipacct_exec(vs, args);
}

static int
sql_execute_cb (void *NotUsed, int argc, char **argv, char **azColName)
{
  /* Do nothing */
  return 0;
}

static int
reset_accounting (const char *ip) {
  char sql[512];
  sqlite3 *connection = NULL;
  char *zErrMsg       = NULL;
  int rc = 0;

  if (ip) {
    snprintf (sql, sizeof (sql) - 1, "DELETE FROM acct WHERE ip_src='%s' OR "
              "ip_dst='%s'", ip, ip);
  } else {
    snprintf (sql, sizeof (sql) - 1, "DELETE FROM acct");
  }

  rc = sqlite3_open (RAHUNAS_DB, &connection);
  if (rc) {
    logmsg (RH_LOG_ERROR, "Task DBSET: could not open database, %s",
            sqlite3_errmsg (connection));
    sqlite3_close (connection);
    return -1;
  }

  sqlite3_busy_timeout (connection, RH_SQLITE_BUSY_TIMEOUT_DEFAULT);

  rc = sqlite3_exec (connection, sql, sql_execute_cb, 0, &zErrMsg);
  if (rc != SQLITE_OK) {
    logmsg (RH_LOG_ERROR, "Task DBSET: could not execute sql (%s)", zErrMsg);
    sqlite3_free (zErrMsg);
  }

  sqlite3_close (connection);
  return rc;
}

/* Start service task */
static int startservice (void)
{
  /* Do nothing */
}

/* Stop service task */
static int stopservice (void)
{
  reset_accounting (NULL);
}

/* Initialize */
static void init (RHVServer *vs)
{
  logmsg(RH_LOG_NORMAL, "[%s] Task IP Accounting initialize..",
         vs->vserver_config->vserver_name);

  ipacct_start(vs);
}

/* Cleanup */
static void cleanup (RHVServer *vs)
{
  logmsg(RH_LOG_NORMAL, "[%s] Task IP Accounting cleanup..",
         vs->vserver_config->vserver_name);

  ipacct_stop(vs);
}

/* Start session task */
static int startsess (RHVServer *vs, struct task_req *req)
{
  reset_accounting (idtoip (vs->v_map, req->id));
  return 0;
}

/* Stop session task */
static int stopsess  (RHVServer *vs, struct task_req *req)
{
  reset_accounting (idtoip (vs->v_map, req->id));
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

static struct task taskipacct = {
  .taskname = "IPACCT",
  .taskprio = 22,
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

void rh_task_ipacct_reg(RHMainServer *ms) {
  task_register(ms, &taskipacct);
}
