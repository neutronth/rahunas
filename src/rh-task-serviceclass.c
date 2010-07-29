/**
 * RahuNAS task serviceclass implementation 
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2010-07-29
 */

#include <syslog.h>
#include <libgda/libgda.h>

#include "rahunasd.h"
#include "rh-serviceclass.h"
#include "rh-ipset.h"
#include "rh-task.h"

static struct set *rh_serviceclass_set = NULL;

static uint32_t _sc_get_slot_id(struct rahunas_serviceclass_config *sc_config)
{
  uint32_t slot_id = 0;
  time_t random_time; 
  int  retry = 30;
  char select_cmd[256];
  GdaConnection *connection = NULL;
  GdaSqlParser  *parser = NULL;
  GList *data_list = NULL;

  if (sc_config == NULL)
    return 0;

  connection = gda_connection_open_from_dsn (PROGRAM, NULL,
                 GDA_CONNECTION_OPTIONS_READ_ONLY, NULL);
  parser = gda_connection_create_parser (connection);
  if (!parser) {
    parser = gda_sql_parser_new ();
  }

  g_object_set_data_full (G_OBJECT (connection), "parser", parser, g_object_unref);

  while (slot_id == 0 && (--retry > 0)) {
    time(&random_time);
    srandom(random_time);
    slot_id = random() % sc_config->network_size;

    if (slot_id != 0) {
      memset (&select_cmd, '\0', sizeof (select_cmd));
      snprintf(select_cmd, sizeof (select_cmd) - 1,
               "SELECT * FROM dbset WHERE service_class = '%s' "
               "AND service_class_slot_id = %u",
               sc_config->serviceclass_name, slot_id);

      DP("SQL: %s", select_cmd);
      data_list = execute_sql_command(connection, select_cmd);

      if (g_list_first (data_list) == NULL) {
        // Available
        free_data_list(data_list);
        break;
      } else { 
        // Not available, retry
        free_data_list(data_list);
        slot_id = 0;
      }
    }
  }

  if (connection)
    g_object_unref(G_OBJECT(connection));

  return slot_id;
}

/* Start service task */
static int startservice ()
{
  rh_serviceclass_set = set_adt_get(SERVICECLASS_SET_NAME);

  logmsg(RH_LOG_NORMAL, "Service Class: Flushing set ...");
  set_flush (SERVICECLASS_SET_NAME);
  return 0;
}

/* Stop service task */
static int stopservice  ()
{
  if (rh_serviceclass_set != NULL) {
    logmsg(RH_LOG_NORMAL, "Service Class: Flushing set ...");
    set_flush (SERVICECLASS_SET_NAME);
    rh_free(&rh_serviceclass_set); 
  }

  return 0;
}

/* Initialize */
static void init (struct vserver *vs)
{
}

/* Cleanup */
static void cleanup (struct vserver *vs)
{
}

/* Start session task */
static int startsess (struct vserver *vs, struct task_req *req)
{
  struct serviceclass *sc = NULL;
  struct rahunas_serviceclass_config *sc_config = NULL;
  ip_set_ip_t ip;
  ip_set_ip_t ip1;
  int res = 0;
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;

  if (req->serviceclass_name == NULL)
    return 0;

  member_node = member_get_node_by_id(vs, req->id);
  if (member_node == NULL)
    return (-1);

  member = (struct rahunas_member *) member_node->data;


  sc = serviceclass_exists(vs->main_server->serviceclass_list, -1,
                           req->serviceclass_name);

  if (sc == NULL) {
    goto failed;
  }

  sc_config = sc->serviceclass_config;

  if (req->serviceclass_slot_id == 0) {
    req->serviceclass_slot_id = _sc_get_slot_id (sc_config);

    if (req->serviceclass_slot_id == 0)
      goto failed;
  }

  if (req->serviceclass_slot_id <= sc_config->network_size) {
    parse_ip(idtoip(vs->v_map, req->id), &ip);
    ip1 = ntohl (sc_config->start_addr.s_addr) + req->serviceclass_slot_id;

    res = set_ipiphash_adtip_nb(rh_serviceclass_set, &ip, &ip1, IP_SET_OP_ADD_IP);

    if (res != 0)
      goto failed;

    res = set_ipiphash_adtip_nb(rh_serviceclass_set, &ip1, &ip, IP_SET_OP_ADD_IP);

    if (res != 0) {
      // Revert
      res = set_ipiphash_adtip_nb(rh_serviceclass_set, &ip, &ip1, IP_SET_OP_DEL_IP);
      goto failed;
    }


    member->serviceclass_name = strdup (req->serviceclass_name);
    member->serviceclass_slot_id = req->serviceclass_slot_id;

    member->mapping_ip = strdup (ip_tostring(ip1));

    logmsg (RH_LOG_NORMAL, "[%s] Service class for User: %s, IP: %s "
                           "- Service Class: %s, Mapping: %s",
                           vs->vserver_config->vserver_name,
                           req->username,
                           idtoip(vs->v_map, req->id),
                           req->serviceclass_name,
                           member->mapping_ip);
  } else {
    goto failed;
  }

  return 0;

failed:
  logmsg (RH_LOG_NORMAL, "[%s] Service class for User: %s, IP: %s "
                         "- Service Class: %s, Failed!",
                         vs->vserver_config->vserver_name,
                         req->username,
                         idtoip(vs->v_map, req->id),
                         req->serviceclass_name);

  return 0;
}

/* Stop session task */
static int stopsess  (struct vserver *vs, struct task_req *req)
{
  struct serviceclass *sc = NULL;
  struct rahunas_serviceclass_config *sc_config = NULL;
  ip_set_ip_t ip;
  ip_set_ip_t ip1;
  int res = 0;
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;

  if (req->serviceclass_name == NULL)
    return 0;

  member_node = member_get_node_by_id(vs, req->id);
  if (member_node == NULL)
    return (-1);

  member = (struct rahunas_member *) member_node->data;

  sc = serviceclass_exists(vs->main_server->serviceclass_list, -1,
                           member->serviceclass_name);

  if (sc == NULL) {
    goto failed;
  }

  sc_config = sc->serviceclass_config;

  if (member->serviceclass_slot_id == 0) {
    goto failed;
  }

  if (member->serviceclass_slot_id <= sc_config->network_size) {
    parse_ip(idtoip(vs->v_map, req->id), &ip);
    ip1 = ntohl (sc_config->start_addr.s_addr) + member->serviceclass_slot_id;

    res = set_ipiphash_adtip_nb(rh_serviceclass_set, &ip, &ip1, IP_SET_OP_DEL_IP);

    if (res != 0)
      goto failed;

    res = set_ipiphash_adtip_nb(rh_serviceclass_set, &ip1, &ip, IP_SET_OP_DEL_IP);
    if (res != 0)
      goto failed;

    logmsg (RH_LOG_NORMAL, "[%s] Service class for User: %s, IP: %s "
                           "- Service Class: %s, Mapping: %s (Removed)",
                           vs->vserver_config->vserver_name,
                           member->username,
                           idtoip(vs->v_map, req->id),
                           member->serviceclass_name,
                           member->mapping_ip);
  } else {
    goto failed;
  }

  return 0;

failed:
  logmsg (RH_LOG_NORMAL, "[%s] Service class for User: %s, IP: %s "
                         "- Service Class: %s, Mapping: %s (Removed Failed!)",
                         vs->vserver_config->vserver_name,
                         member->username,
                         idtoip(vs->v_map, req->id),
                         member->serviceclass_name,
                         member->mapping_ip);

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

static struct task task_serviceclass = {
  .taskname = "SERVICECLASS",
  .taskprio = 25,
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

void rh_task_serviceclass_reg(struct main_server *ms) {
  task_register(ms, &task_serviceclass);
}
