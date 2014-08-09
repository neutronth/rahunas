/**
 * RahuNAS task serviceclass implementation
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2010-07-29
 */

#include <string.h>
#include <syslog.h>
#include <sqlite3.h>

#include "rahunasd.h"
#include "rh-serviceclass.h"
#include "rh-ipset.h"
#include "rh-task.h"
#include "rh-task-memset.h"

static struct set *rh_serviceclass_set = NULL;

static uint32_t _sc_get_slot_id(struct rahunas_serviceclass_config *sc_config)
{
  uint32_t slot_id = 0;
  time_t random_time;
  int  retry = 30;
  char select_cmd[256];
  sqlite3 *connection = NULL;
  int  rc;

  if (sc_config == NULL)
    return 0;

  rc = sqlite3_open (RAHUNAS_DB, &connection);
  if (rc) {
    logmsg (RH_LOG_ERROR, "Task SERVICECLASS: could not open database, %s",
            sqlite3_errmsg (connection));
    sqlite3_close (connection);
    return -1;
  }

  while (slot_id == 0 && (--retry > 0)) {
    int  rc;
    char **azResult = NULL;
    char *zErrMsg   = NULL;
    char *colname   = NULL;
    char *value     = NULL;
    int  nRow       = 0;
    int  nColumn    = 0;

    time(&random_time);
    srandom(random_time);
    slot_id = random() % sc_config->network_size;

    if (slot_id != 0) {
      snprintf(select_cmd, sizeof (select_cmd) - 1,
               "SELECT service_class_slot_id FROM dbset WHERE "
               "service_class = '%s' AND service_class_slot_id = %u",
               sc_config->serviceclass_name, slot_id);
      select_cmd[sizeof (select_cmd) - 1] = '\0';

      DP("SQL: %s", select_cmd);
      rc = sqlite3_get_table (connection, select_cmd, &azResult, &nRow,
                              &nColumn, &zErrMsg);
      if (rc != SQLITE_OK) {
        logmsg (RH_LOG_ERROR, "Task SERVICECLASS: could not get slot id form db"
                              " (%s)", zErrMsg);
        sqlite3_free (zErrMsg);
        sqlite3_free_table (azResult);
        sqlite3_close (connection);
        return 0;
      }

      sqlite3_free_table (azResult);

      if (nRow > 0) {
        slot_id = 0;
        continue;
      } else {
        break;
      }
    }
  }

  sqlite3_close (connection);

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
    rh_free((void **) &rh_serviceclass_set);
  }

  return 0;
}

/* Initialize */
static void init (RHVServer *vs)
{
}

/* Cleanup */
static void cleanup (RHVServer *vs)
{
}

/* Start session task */
static int startsess (RHVServer *vs, struct task_req *req)
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
    member->serviceclass_description = sc_config->description;
    member->serviceclass_slot_id = req->serviceclass_slot_id;

    if (member->mapping_ip)
      {
        free (member->mapping_ip);
        member->mapping_ip = NULL;
      }

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
  if ((req->serviceclass_name != NULL) && (strlen(req->serviceclass_name) == 0))
    goto out;

  logmsg (RH_LOG_NORMAL, "[%s] Service class for User: %s, IP: %s "
                         "- Service Class: %s, Failed!",
                         vs->vserver_config->vserver_name,
                         req->username,
                         idtoip(vs->v_map, req->id),
                         req->serviceclass_name);

out:
  if (member->serviceclass_name != NULL &&
        member->serviceclass_name != termstring) {
    free (member->serviceclass_name);
    member->serviceclass_name    = NULL;
    member->serviceclass_slot_id = 0;
  }

  return 0;
}

/* Stop session task */
static int stopsess  (RHVServer *vs, struct task_req *req)
{
  struct serviceclass *sc = NULL;
  struct rahunas_serviceclass_config *sc_config = NULL;
  ip_set_ip_t ip;
  ip_set_ip_t ip1;
  int res = 0;
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;

  member_node = member_get_node_by_id(vs, req->id);
  if (member_node == NULL)
    return (-1);

  member = (struct rahunas_member *) member_node->data;

  if (member->serviceclass_name == NULL)
    return 0;

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

void rh_task_serviceclass_reg(RHMainServer *ms) {
  task_register(ms, &task_serviceclass);
}
