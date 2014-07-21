/**
 * RahuNAS task dbset implementation 
 * Author: Suriya  Soutmun <darksolar@gmail.com>
 * Date:   2008-09-11
 */

#include <stdlib.h>
#include <syslog.h>
#include <time.h>
#include <string.h>
#include <sqlite3.h>
#include "rahunasd.h"
#include "rh-task.h"
#include "rh-ipset.h"
#include "rh-utils.h"
#include "rh-task-memset.h"


struct dbset_row {
  gchar *session_id;
  gchar *vserver_id;
  gchar *username;
  gchar *ip;
  gchar *mac;
  time_t session_start;
  time_t session_timeout;
  unsigned short bandwidth_slot_id; 
  long bandwidth_max_down;
  long bandwidth_max_up;
  gchar *service_class;
  uint32_t service_class_slot_id;
};

static void
free_dbset_row (struct dbset_row *row)
{
  if (!row)
    return;

  g_free (row->session_id);
  g_free (row->vserver_id);
  g_free (row->username);
  g_free (row->ip);
  g_free (row->mac);
  g_free (row->service_class);
}

static sqlite3 *
openconn (void)
{
  sqlite3 *connection = NULL;
  int  rc;

  rc = sqlite3_open (RAHUNAS_DB, &connection);
  if (rc) {
    logmsg (RH_LOG_ERROR, "Task DBSET: could not open database, %s",
            sqlite3_errmsg (connection));
    sqlite3_close (connection);
    connection = NULL;
  }

  return connection;
}

static void
closeconn (sqlite3 *connection)
{
  if (connection)
    sqlite3_close (connection);
}

static int
sql_execute_cb (void *NotUsed, int argc, char **argv, char **azColName)
{
  /* Do nothing */
  return 0;
}

static int
sql_execute (const char *sql)
{
  sqlite3 *connection = NULL;
  char *zErrMsg       = NULL;
  int rc;
  int retry = 5;

  if (!sql)
    return -1;

retry:
  connection = openconn ();
  if (!connection)
    return -1;

  rc = sqlite3_exec (connection, sql, sql_execute_cb, 0, &zErrMsg);
  if (rc != SQLITE_OK) {
    if (rc == SQLITE_BUSY && --retry > 0) {
      closeconn (connection);
      goto retry;
    }

    logmsg (RH_LOG_ERROR, "Task DBSET: could not execute sql (%s)", zErrMsg);
    sqlite3_free (zErrMsg);
  }

  closeconn (connection);
  return rc;
}


static int
restore_set (RHVServer *vs)
{
  sqlite3 *connection = NULL;
  char sql[256];
  int  rc;
  char **azResult = NULL;
  char *zErrMsg   = NULL;
  char *colname   = NULL;
  char *value     = NULL;
  int  nRow       = 0;
  int  nColumn    = 0;
  int  rowOffset  = 1;
  int  i          = 0;

  if (!vs)
    return -1;

  connection = openconn ();

  if (!connection)
    return -1;
 
  DP("Get data from DB if exist");
  snprintf(sql, sizeof (sql) - 1,
           "SELECT * FROM dbset WHERE vserver_id='%d'",
           vs->vserver_config->vserver_id);
  sql[sizeof (sql) - 1] = '\0';

  DP("SQL: %s", sql);

  rc = sqlite3_get_table (connection, sql, &azResult, &nRow, &nColumn,
                          &zErrMsg);
  if (rc != SQLITE_OK) {
    logmsg (RH_LOG_ERROR, "[%s] Task DBSET: could not get data (%s)",
                          vs->vserver_config->vserver_name, zErrMsg);
    sqlite3_free (zErrMsg);
    closeconn (connection);
    return -1;
  }

  while (rowOffset < (nRow + 1)) {
    struct dbset_row row;
    uint32_t id;
    struct task_req req;
    unsigned char ethernet[ETH_ALEN] = {0,0,0,0,0,0};
    struct tm tm;
    time_t time;

    /* Fetch row data */
    for (i = 0; i < nColumn; i++) {
      colname = azResult[i];
      value = azResult[(rowOffset * nColumn) + i];

      DP ("Row: %s = %s", colname, value);

      if (COLNAME_MATCH ("session_id", colname)) {
        row.session_id = g_strdup (value);
      } else if (COLNAME_MATCH ("vserver_id", colname)) {
        row.vserver_id = g_strdup (value);
      } else if (COLNAME_MATCH ("username", colname)) {
        row.username = g_strdup (value);
      } else if (COLNAME_MATCH ("ip", colname)) {
        row.ip = g_strdup (value);
      } else if (COLNAME_MATCH ("mac", colname)) {
        row.mac = g_strdup (value);
      } else if (COLNAME_MATCH ("session_start", colname)) {
        strptime (value, "%s", &tm);
        time = mktime (&tm);
        memcpy (&row.session_start, &time, sizeof (time_t));
      } else if (COLNAME_MATCH ("session_timeout", colname)) {
        strptime (value, "%s", &tm);
        time = mktime (&tm);
        memcpy (&row.session_timeout, &time, sizeof (time_t));
      } else if (COLNAME_MATCH ("bandwidth_slot_id", colname)) {
        row.bandwidth_slot_id = atoi (value);
      } else if (COLNAME_MATCH ("bandwidth_max_down", colname)) {
        row.bandwidth_max_down = atol (value);
      } else if (COLNAME_MATCH ("bandwidth_max_up", colname)) {
        row.bandwidth_max_up = atol (value);
      } else if (COLNAME_MATCH ("service_class_slot_id", colname)) {
          row.service_class_slot_id = atol (value);
      } else if (COLNAME_MATCH ("service_class", colname)) {
          row.service_class = g_strdup (value);
      }
    }

    /* Process row data */
    if (atoi(row.vserver_id) != vs->vserver_config->vserver_id)
      goto skip;

    id = iptoid (vs->v_map, row.ip);

    if (id < 0)
      goto skip;

    req.id = id;
    req.vserver_id = atoi (row.vserver_id);
    req.username = row.username;
    req.session_id = row.session_id;
    parse_mac(row.mac, ethernet);
    memcpy(req.mac_address, ethernet, ETH_ALEN);

    req.session_start = row.session_start;
    req.session_timeout = row.session_timeout;

    req.bandwidth_slot_id = row.bandwidth_slot_id;
    req.bandwidth_max_down = row.bandwidth_max_down;
    req.bandwidth_max_up = row.bandwidth_max_up;

    req.serviceclass_name = row.service_class;
    req.serviceclass_slot_id = row.service_class_slot_id;

    rh_task_startsess(vs, &req);

skip:
    free_dbset_row (&row);
    rowOffset++;
  }

  sqlite3_free_table (azResult);
  closeconn (connection);
  return 0;
}

/* Start service task */
static int startservice ()
{
  sqlite3 *connection = NULL;

  logmsg(RH_LOG_NORMAL, "Task DBSET start..");

  /* Test database connection */
  connection = openconn();
  if (!connection) {
    return -1;
  }

  /* Connection successfully connected, just close */
  closeconn (connection);

  return 0;
}

/* Stop service task */
static int stopservice  ()
{
  /* Clear all data in the database */
  return sql_execute ("DELETE FROM dbset;");
}

/* Initialize */
static void init (RHVServer *vs)
{
  if (vs->vserver_config->init_flag == VS_RELOAD)
    return;

  logmsg(RH_LOG_NORMAL, "[%s] Task DBSET initialize..",
         vs->vserver_config->vserver_name);  

  restore_set(vs);
}

/* Cleanup */
static void cleanup (RHVServer *vs)
{
  /* Do nothing */
}

/* Start session task */
static int startsess (RHVServer *vs, struct task_req *req)
{
  char startsess_cmd[512];
  char time_str[32];
  char time_str2[32];
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;

  strftime(time_str, sizeof time_str, "%s", localtime(&req->session_start));
  strftime(time_str2, sizeof time_str2, "%s",
    localtime(&req->session_timeout));

  member_node = member_get_node_by_id(vs, req->id);
  if (member_node == NULL)
    return (-1);

  member = (struct rahunas_member *) member_node->data;

  snprintf(startsess_cmd, sizeof (startsess_cmd) - 1, "INSERT INTO dbset"
         "(session_id,vserver_id,username,ip,mac,session_start,"
         "session_timeout,bandwidth_slot_id,bandwidth_max_down,"
         "bandwidth_max_up,service_class,service_class_slot_id) "
         "VALUES('%s','%d','%s','%s','%s',%s,%s,%u,%lu,%lu,'%s',%u)",
         req->session_id, 
         vs->vserver_config->vserver_id, 
         req->username, 
         idtoip(vs->v_map, req->id), 
         mac_tostring(req->mac_address), 
         time_str, 
         time_str2,
         member->bandwidth_slot_id, 
         req->bandwidth_max_down,
         req->bandwidth_max_up,
         member->serviceclass_name,
         member->serviceclass_slot_id);
  startsess_cmd[sizeof (startsess_cmd) - 1] = '\0';

  DP("SQL: %s", startsess_cmd);

  return sql_execute (startsess_cmd);
}

/* Stop session task */
static int stopsess (RHVServer *vs, struct task_req *req)
{
  char stopsess_cmd[256];
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;

  member_node = member_get_node_by_id(vs, req->id);
  if (member_node == NULL)
    return (-1);

  member = (struct rahunas_member *) member_node->data;

  DP("Username  : %s", member->username);
  DP("SessionID : %s", member->session_id);

  snprintf(stopsess_cmd, sizeof (stopsess_cmd) - 1, "DELETE FROM dbset WHERE "
         "session_id='%s' AND username='%s' AND vserver_id='%d'",
         member->session_id, 
         member->username,
         vs->vserver_config->vserver_id);
  stopsess_cmd[sizeof (stopsess_cmd) - 1] = '\0';

  DP("SQL: %s", stopsess_cmd);

  return sql_execute (stopsess_cmd);
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

static struct task task_dbset = {
  .taskname = "DBSET",
  .taskprio = 10,
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

void rh_task_dbset_reg(RHMainServer *ms) {
  task_register(ms, &task_dbset);
}
