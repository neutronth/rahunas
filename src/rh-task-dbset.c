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

struct dbset_row {
  gchar *session_id;
  gchar *username;
  gchar *ip;
  gchar *mac;
  time_t session_start;
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
      value = gda_data_model_get_value_at (dm, column_id, row_id);
      str = gda_value_stringify(value);
               
      if (strncmp("session_id", title, 10) == 0) {
        row->session_id = g_strdup(str);
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
      } 
    }
  }
  
  return TRUE;
}

GList *execute_sql_command(GdaConnection *connection, const gchar *buffer)
{
  GdaCommand *command;
  GList *data_list = NULL;
  GList *list;
  GList *node;
  GdaDataModel *dm;
  gboolean errors = FALSE;


  command = gda_command_new (buffer, GDA_COMMAND_TYPE_SQL,
                             GDA_COMMAND_OPTION_STOP_ON_ERRORS);

  list = gda_connection_execute_command (connection, command, NULL, NULL);
  if (list != NULL) {
    for (node = list; node != NULL; node = g_list_next(node)) {
      if (GDA_IS_DATA_MODEL(node->data)) {
        dm = (GdaDataModel *) node->data;
        parse_dm_to_struct(&data_list, dm);
        g_object_unref(dm);
      } else {
        get_errors (connection);
        errors = TRUE;  
      }
    }
  }

  gda_command_free(command);
  
  return errors == TRUE ? NULL : data_list;
}

void execute_sql(GdaConnection *connection, const gchar *buffer)
{
  GdaCommand *command;
  
  command = gda_command_new (buffer, GDA_COMMAND_TYPE_SQL,
                             GDA_COMMAND_OPTION_STOP_ON_ERRORS);
  gda_connection_execute_select_command (connection, command, NULL, NULL);

  gda_command_free (command);
}

void list_datasource (void)
{
  GList *ds_list;
  GList *node;
  GdaDataSourceInfo *info;

  ds_list = gda_config_get_data_source_list();

  for (node = g_list_first(ds_list); node != NULL; node = g_list_next(node)) {
    info = (GdaDataSourceInfo *) node->data;

    if (strncmp(info->name, PROGRAM, strlen(PROGRAM)) == 0) {
      logmsg(RH_LOG_NORMAL, "Datasource: NAME: %s PROVIDER: %s",
             info->name, info->provider);
      logmsg(RH_LOG_NORMAL, "  CNC: %s", info->cnc_string);
      logmsg(RH_LOG_NORMAL, "  DESC: %s", info->description);
    }
  }

  gda_config_free_data_source_list(ds_list);
}

GdaConnection *get_connection(GdaClient *client)
{
  return gda_client_open_connection (client, PROGRAM, NULL, NULL,
                                     GDA_CONNECTION_OPTIONS_NONE, NULL);
}

void free_data_list(GList *data_list)
{
  GList *node;
  struct dbset_row *row;

  for (node = g_list_first(data_list); node != NULL; 
         node = g_list_next (node)) {
    row = (struct dbset_row *) node->data;
    g_free(row->session_id);
    g_free(row->username);
    g_free(row->ip);
    g_free(row->mac);
  }
  
  g_list_free (data_list);  
}

gboolean restore_set(GList **data_list, struct rahunas_map *map)
{
  GList *node;
  struct rahunas_member *members = NULL;
  struct dbset_row *row = NULL;
  uint32_t id;

  if (!map)
    return FALSE;

  members = map->members;

  node = g_list_first(*data_list);

  if (node == NULL)
    return TRUE;

  for (node; node != NULL; node = g_list_next(node)) {
    row = (struct dbset_row *) node->data;

    id = iptoid(map, row->ip);
    
    if (id < 0)
      continue;

    // MEMSET
    members[id].session_id = g_strdup(row->session_id);
    members[id].username   = g_strdup(row->username);
    parse_mac(row->mac, &members[id].mac_address); 
    memcpy(&row->session_start, &members[id].session_start, sizeof(time_t));

    // IPSET
    set_adtip(rahunas_set, row->ip, row->mac, IP_SET_OP_ADD_IP);
  }
  return TRUE;
}

/* Initialize */
static void init (void)
{
  char ds_name[] = PROGRAM;
  char ds_provider[] = "SQLite";
  char ds_cnc_string[] = "DB_DIR=" RAHUNAS_CONF_DIR ";DB_NAME=" DB_NAME; 
  char ds_desc[] = "RahuNAS DB Set";

  logmsg(RH_LOG_NORMAL, "Task DBSET init..");  

  gda_init(PROGRAM, RAHUNAS_VERSION, NULL, NULL);

  gda_config_save_data_source(ds_name, ds_provider, ds_cnc_string, ds_desc,
                              NULL, NULL, FALSE);
 
  list_datasource();
}

/* Cleanup */
static void cleanup (void)
{
  char ds_name[] = PROGRAM;

  logmsg(RH_LOG_NORMAL, "Task DBSET cleanup..");  
  
  gda_config_remove_data_source (ds_name);
}


/* Start service task */
static int startservice (struct rahunas_map *map)
{
  GdaClient *client;
  GdaConnection *connection;
  GList *data_list;
  GList *node;
  struct dbset_row *row;

  logmsg(RH_LOG_NORMAL, "Task DBSET start..");  

  client = gda_client_new ();
  connection = gda_client_open_connection (client, PROGRAM, NULL, NULL,
                 GDA_CONNECTION_OPTIONS_READ_ONLY, NULL);

  data_list = execute_sql_command(connection, "SELECT * FROM dbset");

  restore_set(&data_list, map);

  free_data_list(data_list);

  gda_client_close_all_connections (client);

  g_object_unref(G_OBJECT(client));

  return 0;
}

/* Stop service task */
static int stopservice  (struct rahunas_map *map)
{
  /* Do nothing or need to implement */
  logmsg(RH_LOG_NORMAL, "Task DBSET stop..");  
}

/* Start session task */
static int startsess (struct rahunas_map *map, struct task_req *req)
{
  GdaClient *client;
  GdaConnection *connection;
  gint res;
  char startsess_cmd[256];
  char time_str[32];

  client = gda_client_new ();
  connection = gda_client_open_connection (client, PROGRAM, NULL, NULL,
                 GDA_CONNECTION_OPTIONS_NONE, NULL);
  
  strftime(&time_str, sizeof time_str, "%s", localtime(&req->session_start));
  sprintf(startsess_cmd, "INSERT INTO dbset"
         "(session_id,username,ip,mac,session_start) "
         "VALUES('%s','%s','%s','%s',%s)",
         req->session_id, req->username, idtoip(map, req->id), 
         mac_tostring(req->mac_address), time_str);

  DP("SQL: %s", startsess_cmd);

  execute_sql(connection, startsess_cmd);

  gda_client_close_all_connections (client);

  g_object_unref(G_OBJECT(client)); 

  return 0;
}

/* Stop session task */
static int stopsess (struct rahunas_map *map, struct task_req *req)
{
  GdaClient *client;
  GdaConnection *connection;
  gint res;
  char stopsess_cmd[256];

  client = gda_client_new ();
  connection = gda_client_open_connection (client, PROGRAM, NULL, NULL,
                 GDA_CONNECTION_OPTIONS_NONE, NULL);

  DP("Username  : %s", map->members[req->id].username);
  DP("SessionID : %s", map->members[req->id].session_id);

  sprintf(stopsess_cmd, "DELETE FROM dbset WHERE "
         "session_id='%s' AND username='%s'",
         map->members[req->id].session_id, 
         map->members[req->id].username);

  DP("SQL: %s", stopsess_cmd);

  execute_sql(connection, stopsess_cmd);

  gda_client_close_all_connections (client);

  g_object_unref(G_OBJECT(client)); 

  return 0;
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

static struct task task_dbset = {
  .taskname = "DBSET",
  .taskprio = 1,
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

void rh_task_dbset_reg(void) {
  task_register(&task_dbset);
}
