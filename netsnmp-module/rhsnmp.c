/* RahuNAS SNMP Agent
 *
 * License: BSD
 * Copyright (C) 2011 Neutron Soutmun <neutron@rahunas.org>
 */

#include <sys/inotify.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sqlite3.h>
#include "rhsnmp.h"

pthread_mutex_t rh_mtx = PTHREAD_MUTEX_INITIALIZER;
netsnmp_table_data_set *table_set = NULL;
oid rh_reg_oid[]       = { 1, 3, 6, 1, 4, 1, 38668, 1, 1 };
int stop_module;

/* Override table_data */
static int                   rh_netsnmp_register_table_data (
                               netsnmp_handler_registration *reginfo,
                               netsnmp_table_data *table,
                               netsnmp_table_registration_info *table_info);

static netsnmp_mib_handler * rh_netsnmp_get_table_data_handler (
                               netsnmp_table_data *table);

static int                   rh_netsnmp_table_data_helper_handler (
                               netsnmp_mib_handler *handler,
                               netsnmp_handler_registration *reginfo,
                               netsnmp_agent_request_info *reqinfo,
                               netsnmp_request_info *requests);

/* Override table_dataset */
static int                   rh_netsnmp_register_table_data_set (
                               netsnmp_handler_registration *reginfo,
                               netsnmp_table_data_set *data_set,
                               netsnmp_table_registration_info *table_info);


/* RahuNAS Data Update*/
static void * rh_update_data (void *ptr);
static void   rh_remove_old_data_set (netsnmp_table_data_set *table_set);
static void   rh_add_new_data_set (netsnmp_table_data_set *table_set);

/* Override table_data implementation */
static int
rh_netsnmp_register_table_data (
  netsnmp_handler_registration *reginfo,
  netsnmp_table_data *table,
  netsnmp_table_registration_info *table_info)
{
  netsnmp_inject_handler (reginfo, rh_netsnmp_get_table_data_handler (table));
  return netsnmp_register_table (reginfo, table_info);
}

static netsnmp_mib_handler *
rh_netsnmp_get_table_data_handler (netsnmp_table_data *table)
{
  netsnmp_mib_handler *ret = NULL;

  if (!table) {
      snmp_log(LOG_INFO,
               "netsnmp_get_table_data_handler(NULL) called\n");
      return NULL;
  }

  ret =
      netsnmp_create_handler(TABLE_DATA_NAME,
                             rh_netsnmp_table_data_helper_handler);
  if (ret) {
      ret->flags |= MIB_HANDLER_AUTO_NEXT;
      ret->myvoid = (void *) table;
  }

  return ret;
}

static int
rh_netsnmp_table_data_helper_handler (
  netsnmp_mib_handler *handler,
  netsnmp_handler_registration *reginfo,
  netsnmp_agent_request_info *reqinfo,
  netsnmp_request_info *requests)
{
  int ret = 0;

  pthread_mutex_lock (&rh_mtx);
  ret = netsnmp_table_data_helper_handler (handler, reginfo, reqinfo, requests);
  pthread_mutex_unlock (&rh_mtx);

  return ret;
}


/* Override table_dataset implementation */
static int
rh_netsnmp_register_table_data_set (
  netsnmp_handler_registration *reginfo,
  netsnmp_table_data_set *data_set,
  netsnmp_table_registration_info *table_info)
{
    if (NULL == table_info) {
        /* allocate the table if one wasn't allocated */
        table_info = SNMP_MALLOC_TYPEDEF (netsnmp_table_registration_info);
    }

    if (NULL == table_info->indexes && data_set->table->indexes_template) {
        /* copy the indexes in */
        table_info->indexes =
            snmp_clone_varbind (data_set->table->indexes_template);
    }

    if ((!table_info->min_column || !table_info->max_column) &&
        (data_set->default_row)) {
        /* determine min/max columns */
        unsigned int    mincol = 0xffffffff, maxcol = 0;
        netsnmp_table_data_set_storage *row;

        for (row = data_set->default_row; row; row = row->next) {
            mincol = SNMP_MIN (mincol, row->column);
            maxcol = SNMP_MAX (maxcol, row->column);
        }
        if (!table_info->min_column)
            table_info->min_column = mincol;
        if (!table_info->max_column)
            table_info->max_column = maxcol;
    }

    netsnmp_inject_handler (reginfo,
                            netsnmp_get_table_data_set_handler (data_set));
    return rh_netsnmp_register_table_data (reginfo, data_set->table,
                                           table_info);
}

/* Agent Initializing */
void
init_rahunas (void)
{
  pthread_t upd_thread;

  stop_module = 0;

  DEBUGMSGTL (("RahuNAS SNMP Agent", "Initializing.\n"));

  table_set = netsnmp_create_table_data_set ("rahunasAuthenLoginTable");
  table_set->allow_creation = 1;

  netsnmp_table_dataset_add_index (table_set, ASN_OCTET_STR);
  netsnmp_table_set_multi_add_default_row (table_set,
                                           2, ASN_OCTET_STR, 0, NULL, 0,
                                           3, ASN_OCTET_STR, 0, NULL, 0,
                                           4, ASN_INTEGER,   0, NULL, 0,
                                           0);

  rh_netsnmp_register_table_data_set(netsnmp_create_handler_registration
                                  ("rahunasAuthenLoginTable", NULL,
                                   rh_reg_oid,
                                   OID_LENGTH(rh_reg_oid),
                                   HANDLER_CAN_RWRITE), table_set, NULL);

  if (pthread_create (&upd_thread, NULL, rh_update_data,
        (void *) table_set) != 0)
    {
      DEBUGMSGTL(("RahuNAS SNMP Agent", "Start update data thread fail!"));
      exit (EXIT_FAILURE);
    }
  else
    {
      DEBUGMSGTL(("RahuNAS SNMP Agent", "Start update data thread success!"));
      pthread_detach (upd_thread);
    }

  DEBUGMSGTL (("RahuNAS SNMP Agent", "Done initializing.\n"));
}

void
deinit_rahunas (void)
{
  stop_module = 1;
  unregister_mib (rh_reg_oid, OID_LENGTH(rh_reg_oid));
}

/* Update data thread worker */
static void *
rh_update_data (void *ptr)
{
  netsnmp_table_data_set *table_set = (netsnmp_table_data_set *) ptr;
  int  inotify_fd     = -1;
  int  inotify_wfd    = -1;
  char inotify_buffer[32 * (sizeof (struct inotify_event) + 16)];
  int  inotify_enable = 0;
  fd_set rfds;

  inotify_fd = inotify_init ();
  if (inotify_fd >= 0)
    {
      if (fcntl (inotify_fd, F_SETFL, O_NONBLOCK) == 0)
        {
          inotify_wfd = inotify_add_watch (inotify_fd, RAHUNAS_DB,
                                           IN_CLOSE_WRITE);
          if (inotify_wfd >= 0)
            {
              inotify_enable = 1;
              FD_ZERO (&rfds);
              FD_SET  (inotify_fd, &rfds);
            }
        }
    }

  while (!stop_module)
    {
      struct timeval timeout;
      int  retval;
      ssize_t r;

      pthread_mutex_lock (&rh_mtx);
      rh_remove_old_data_set (table_set);
      rh_add_new_data_set (table_set);
      pthread_mutex_unlock (&rh_mtx);

      if (inotify_enable)
        {
          timeout.tv_sec  = 30;
          timeout.tv_usec = 0;

          retval = select (1, &rfds, NULL, NULL, &timeout);

          if (retval == -1)
            {
              /* Fallback to sleep */
              sleep (30);
            }
          else if (retval)
            {
              while ((r = read (inotify_fd, inotify_buffer,
                     sizeof (inotify_buffer))))
                {
                  /* Read until no data, r == -1 expected: errno == EAGAIN */
                  if (r == -1)
                    break;
                }
            }
          else
            {
              /* Timeout, fall-through */
            }

        }
      else
        {
          sleep (30);
        }
    }

  if (inotify_enable)
    {
      inotify_rm_watch (inotify_fd, inotify_wfd);
      close (inotify_fd);
    }

  pthread_exit (NULL);
}

static void
rh_remove_old_data_set (netsnmp_table_data_set *table_set)
{
  netsnmp_table_row *row = NULL;

  while ((row = netsnmp_table_data_set_get_first_row (table_set)))
    {
      netsnmp_table_dataset_remove_and_delete_row (table_set, row);
    }
}


static void
rh_add_new_data_set (netsnmp_table_data_set *table_set)
{
  sqlite3 *db     = NULL;
  char *zErrMsg   = NULL;
  char **azResult = NULL;
  char *colname   = NULL;
  char *value     = NULL;
  int nRow;
  int nColumn;
  int row_offset = 1;
  int rc;
  char sql[] = "SELECT * FROM dbset";

  rc = sqlite3_open (RAHUNAS_DB, &db);
  if (rc)
    {
      sqlite3_close (db);
      DEBUGMSGTL (("RahuNAS SNMP Agent", "Could not open database.\n"));
      exit (EXIT_FAILURE);
    }

  rc = sqlite3_get_table (db, sql, &azResult, &nRow, &nColumn, &zErrMsg);
  if (rc != SQLITE_OK)
    {
      DEBUGMSGTL (("RahuNAS SNMP Agent", zErrMsg));
      goto out;
    }

  while (row_offset < (nRow + 1))
    {
      int i = 0;
      netsnmp_table_row *row = NULL;

      row = netsnmp_create_table_data_row ();

      for (i = 0; i < nColumn; i++)
        {
          colname = azResult[i];
          value   = azResult[(row_offset * nColumn) + i];

          if (strcmp (colname, "session_id") == 0)
            {
              netsnmp_table_row_add_index (row, ASN_OCTET_STR, value,
                                           strlen (value));
            }
          else if (strcmp (colname, "username") == 0)
            {
              netsnmp_set_row_column      (row, 2, ASN_OCTET_STR, value,
                                           strlen (value));
            }
          else if (strcmp (colname, "ip") == 0)
            {
              netsnmp_set_row_column      (row, 3, ASN_OCTET_STR, value,
                                           strlen (value));
            }
          else if (strcmp (colname, "session_start") == 0)
            {
              u_long tmpul;
              char *endptr = NULL;

              endptr = strchr (value, '.');
              if (endptr)
                *endptr = '\0';

              endptr = NULL;
              tmpul = strtol (value, &endptr, 10);

              if (*endptr != '\0')
                tmpul = 0;

              netsnmp_set_row_column      (row, 4, ASN_INTEGER, (char *) &tmpul,
                                           sizeof (tmpul));
            }
        }

      netsnmp_table_dataset_add_row (table_set, row);

      row_offset++;
    }

  sqlite3_free_table (azResult);

out:
  sqlite3_close (db);
}
