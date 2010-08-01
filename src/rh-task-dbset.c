/**
 * RahuNAS task dbset implementation 
 * Author: Suriya  Soutmun <darksolar@gmail.com>
 * Date:   2008-09-11
 */

#include <stdlib.h>
#include <syslog.h>
#include <time.h>
#include <string.h>
#include <libgda/libgda.h>
#include "rahunasd.h"
#include "rh-task.h"
#include "rh-ipset.h"
#include "rh-utils.h"

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

gboolean get_errors (GdaConnection * connection)
{
  GList *list;
  GList *node;
  GdaConnectionEvent *error;

  list = (GList *) gda_connection_get_events(connection);

  for (node = g_list_first(list); node != NULL; node = g_list_next(node)) {
    error = (GdaConnectionEvent *) node->data;
    logmsg(RH_LOG_NORMAL, "DB Error no: %d", 
             gda_connection_event_get_code(error));
    logmsg(RH_LOG_NORMAL, "DB Desc: %s", 
             gda_connection_event_get_description(error));
    logmsg(RH_LOG_NORMAL, "DB Source: %s", 
             gda_connection_event_get_source(error));
    logmsg(RH_LOG_NORMAL, "DB SQL State: %s", 
             gda_connection_event_get_sqlstate(error));
  }
}

gboolean *parse_dm_to_struct(GList **data_list, GdaDataModel *dm) {
  gint  row_id;
  gint  column_id;
  GValue *value;
  gchar  *str;
  gchar  *title;
  GdaNumeric *num;
  struct dbset_row *row;
  struct tm tm;
  time_t time;

  char tmp[80];

  for (row_id = 0; row_id < gda_data_model_get_n_rows(dm); row_id++) {
    row = (struct dbset_row *)g_malloc(sizeof(struct dbset_row));
    if (row == NULL) {
      /* Do implement the row list cleanup */
    }

    *data_list = g_list_append(*data_list, row); 

    for (column_id = 0; column_id < gda_data_model_get_n_columns(dm); 
           column_id++) {

      title = gda_data_model_get_column_title(dm, column_id);
      value = gda_data_model_get_value_at (dm, column_id, row_id, NULL);
      str = gda_value_stringify(value);
               
      if (strncmp("session_id", title, 10) == 0) {
        row->session_id = g_strdup(str);
      } else if (strncmp("vserver_id", title, 10) == 0) {
        row->vserver_id = g_strdup(str);
      } else if (strncmp("username", title, 8) == 0) {
        row->username = g_strdup(str);
      } else if (strncmp("ip", title, 2) == 0) {
        row->ip = g_strdup(str);
      } else if (strncmp("mac", title, 3) == 0) {
        row->mac = g_strdup(str);
      } else if (strncmp("session_start", title, 13) == 0) {
        strptime(str, "%s", &tm);
        time = mktime(&tm);
        memcpy(&row->session_start, &time, sizeof(time_t));
      } else if (strncmp("session_timeout", title, 15) == 0) {
        strptime(str, "%s", &tm);
        time = mktime(&tm);
        memcpy(&row->session_timeout, &time, sizeof(time_t));
      } else if (strncmp("bandwidth_slot_id", title, 17) == 0) {
        row->bandwidth_slot_id = atoi(str);
      } else if (strncmp("bandwidth_max_down", title, 18) == 0) {
        row->bandwidth_max_down = atol(str);
      } else if (strncmp("bandwidth_max_up", title, 18) == 0) {
        row->bandwidth_max_up = atol(str);
      } else if (strncmp("service_class_slot_id", title,
                         strlen("service_class_slot_id")) == 0) {
          row->service_class_slot_id = atol(str);
      } else if (strncmp("service_class", title,
                         strlen("service_class")) == 0) {
          row->service_class = g_strdup(str);
      }
    }
  }
  
  return TRUE;
}

GList *execute_sql_command(GdaConnection *connection, const gchar *buffer)
{
  GdaSqlParser *parser = NULL;
  GdaStatement *stmt   = NULL;
  GdaSet       *params = NULL;
  GList *data_list = NULL;
  GList *list;
  GList *node;
  GdaDataModel *dm;
  gboolean errors = FALSE;

  parser = g_object_get_data (G_OBJECT (connection), "parser");

  stmt = gda_sql_parser_parse_string (parser, buffer, NULL, NULL);
  gda_statement_get_parameters (stmt, &params, NULL);

  dm = gda_connection_statement_execute_select (connection, stmt, params, NULL);
  if (dm) {
    parse_dm_to_struct (&data_list, dm);
    g_object_unref (dm);
  }

  if (params)
    g_object_unref (params);

  if (stmt)
    g_object_unref (stmt);
  
  return errors == TRUE ? NULL : data_list;
}

void execute_sql(GdaConnection *connection, const gchar *buffer)
{
  GdaSqlParser *parser = NULL;
  GdaStatement *stmt   = NULL;
  GdaSet       *params = NULL;
  gint         res     = 0;

  parser = g_object_get_data (G_OBJECT (connection), "parser");

  stmt = gda_sql_parser_parse_string (parser, buffer, NULL, NULL);
  gda_statement_get_parameters (stmt, &params, NULL);

  res = gda_connection_statement_execute_non_select (connection, stmt, params,
                                                     NULL, NULL);
  if (params)
    g_object_unref (params);

  if (stmt)
    g_object_unref (stmt);
}

void list_datasource (void)
{
  GList *ds_list;
  GList *node;
  GdaDsnInfo *info;

  info = gda_config_get_dsn_info (PROGRAM);

  logmsg(RH_LOG_NORMAL, "Datasource: NAME: %s PROVIDER: %s",
         info->name, info->provider);
  logmsg(RH_LOG_NORMAL, "  CNC: %s", info->cnc_string);
  logmsg(RH_LOG_NORMAL, "  DESC: %s", info->description);
}

void free_data_list(GList *data_list)
{
  GList *node;
  struct dbset_row *row;

  for (node = g_list_first(data_list); node != NULL; 
         node = g_list_next (node)) {
    row = (struct dbset_row *) node->data;
    g_free(row->session_id);
    g_free(row->vserver_id);
    g_free(row->username);
    g_free(row->ip);
    g_free(row->mac);
    g_free(row->service_class);
  }
  
  g_list_free (data_list);  
}

gboolean restore_set(GList **data_list, struct vserver *vs)
{
  GList *node = NULL;
  struct dbset_row *row = NULL;
  uint32_t id;
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;
  struct task_req req;
  unsigned char ethernet[ETH_ALEN] = {0,0,0,0,0,0};
  unsigned char max_try = 3;
 
  node = g_list_first(*data_list);

  if (node == NULL)
    return TRUE;

  DP("Get data from DB if exist");

  for (node; node != NULL; node = g_list_next(node)) {
    row = (struct dbset_row *) node->data;

    if (atoi(row->vserver_id) != vs->vserver_config->vserver_id)
      continue;

    id = iptoid(vs->v_map, row->ip);

    if (id < 0)
      continue;

    req.id = id;
    req.vserver_id = atoi(row->vserver_id);
    req.username = row->username;
    req.session_id = row->session_id;
    parse_mac(row->mac, &ethernet);
    memcpy(req.mac_address, &ethernet, ETH_ALEN);

    req.session_start = row->session_start;
    req.session_timeout = row->session_timeout;

    req.bandwidth_slot_id = row->bandwidth_slot_id;
    req.bandwidth_max_down = row->bandwidth_max_down; 
    req.bandwidth_max_up = row->bandwidth_max_up;

    req.serviceclass_name = row->service_class;
    req.serviceclass_slot_id = row->service_class_slot_id;

    rh_task_startsess(vs, &req);
  }
  return TRUE;
}


GdaConnection *openconn (GdaConnectionOptions options)
{
  GdaConnection *connection = NULL;
  GdaSqlParser  *parser = NULL;
  connection = gda_connection_open_from_dsn (PROGRAM, NULL, options, NULL);

  parser = gda_connection_create_parser (connection);
  if (!parser) {
    parser = gda_sql_parser_new ();
  }

  g_object_set_data_full (G_OBJECT (connection), "parser", parser, g_object_unref);

  return connection;
}

/* Start service task */
static int startservice ()
{
  GError *error = NULL;
  GdaDsnInfo dsn_info = {
    .name        = PROGRAM,
    .provider    = "SQLite",
    .cnc_string  = "DB_DIR=" RAHUNAS_DB_DIR ";DB_NAME=" DB_NAME,
    .description = "RahuNAS DB Set",
  };

  logmsg(RH_LOG_NORMAL, "Task DBSET start..");
   
  gda_init();
    
  if (!gda_config_define_dsn (&dsn_info, &error)) {
    logmsg (RH_LOG_ERROR, "Could not define DSN");
    g_error_free (error);
  }

  list_datasource();

  return 0;
}

/* Stop service task */
static int stopservice  ()
{
  gda_config_remove_dsn (PROGRAM, NULL);
  return 0;
}

/* Initialize */
static void init (struct vserver *vs)
{
  GdaConnection *connection;
  GList *data_list;
  GList *node;
  struct dbset_row *row;
  char select_cmd[256];

  if (vs->vserver_config->init_flag == VS_RELOAD)
    return;

  logmsg(RH_LOG_NORMAL, "[%s] Task DBSET initialize..",
         vs->vserver_config->vserver_name);  

  connection = openconn (GDA_CONNECTION_OPTIONS_READ_ONLY);

  snprintf(select_cmd, sizeof (select_cmd), 
           "SELECT * FROM dbset WHERE vserver_id='%d'",
           vs->vserver_config->vserver_id);

  DP("SQL: %s", select_cmd);

  data_list = execute_sql_command(connection, select_cmd); 

  restore_set(&data_list, vs);

  free_data_list(data_list);

  if (connection)
    g_object_unref(G_OBJECT(connection));
}

/* Cleanup */
static void cleanup (struct vserver *vs)
{
  /* Do nothing */
}

/* Start session task */
static int startsess (struct vserver *vs, struct task_req *req)
{
  GdaConnection *connection;
  gint res;
  char startsess_cmd[512];
  char time_str[32];
  char time_str2[32];
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;

  connection = openconn (GDA_CONNECTION_OPTIONS_NONE);

  strftime(&time_str, sizeof time_str, "%s", localtime(&req->session_start));
  strftime(&time_str2, sizeof time_str2, "%s", 
    localtime(&req->session_timeout));

  member_node = member_get_node_by_id(vs, req->id);
  if (member_node == NULL)
    return (-1);

  member = (struct rahunas_member *) member_node->data;

  snprintf(startsess_cmd, sizeof (startsess_cmd), "INSERT INTO dbset"
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

  DP("SQL: %s", startsess_cmd);

  execute_sql(connection, startsess_cmd);

  if (connection)
    g_object_unref(G_OBJECT(connection));

  return 0;
}

/* Stop session task */
static int stopsess (struct vserver *vs, struct task_req *req)
{
  GdaConnection *connection;
  gint res;
  char stopsess_cmd[256];
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;

  member_node = member_get_node_by_id(vs, req->id);
  if (member_node == NULL)
    return (-1);

  member = (struct rahunas_member *) member_node->data;

  connection = openconn (GDA_CONNECTION_OPTIONS_NONE);

  DP("Username  : %s", member->username);
  DP("SessionID : %s", member->session_id);

  snprintf(stopsess_cmd, sizeof (stopsess_cmd), "DELETE FROM dbset WHERE "
         "session_id='%s' AND username='%s' AND vserver_id='%d'",
         member->session_id, 
         member->username,
         vs->vserver_config->vserver_id);

  DP("SQL: %s", stopsess_cmd);

  execute_sql(connection, stopsess_cmd);

  if (connection)
    g_object_unref(G_OBJECT(connection));

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

void rh_task_dbset_reg(struct main_server *ms) {
  task_register(ms, &task_dbset);
}
