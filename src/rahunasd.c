/**
 * RahuNASd
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-08-07
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sqlite3.h>
#include <inttypes.h>
#include <execinfo.h>

#include "rahunasd.h"
#include "rh-server.h"
#include "rh-xmlrpc-server.h"
#include "rh-xmlrpc-cmd.h"
#include "rh-ipset.h"
#include "rh-utils.h"
#include "rh-task.h"
#include "rh-macauthen.h"

const char *termstring = "";
pid_t pid, sid;

pthread_mutex_t RHMtxLock        = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t RHPollingMtxLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  RHPollingCond    = PTHREAD_COND_INITIALIZER;

static void rh_exit               (void);
static void rh_reload             (void);
static int  expired_check         (void *data);
static int  polling_expired_check (RHMainServer *ms, RHVServer *vs);
static int  do_task_init          (RHMainServer *ms, RHVServer *vs);
static int  do_task_cleanup       (RHMainServer *ms, RHVServer *vs);
static int  do_task_stopservice   (RHMainServer *ms, RHVServer *vs);
static int  do_vserver_reload     (RHMainServer *ms, RHVServer *vs);
static int  do_vserver_init_done  (RHMainServer *ms, RHVServer *vs);
static int  do_serviceclass_init  (RHMainServer *ms, RHSvClass *sc);
static int  do_serviceclass_reload(RHMainServer *ms, RHSvClass *sc);
static void *polling_service      (void *data);

RHMainServer rh_main_server_instance = {
  .vserver_list = NULL,
  .serviceclass_list = NULL,
  .task_list = NULL,
};
RHMainServer *rh_main_server = &rh_main_server_instance;

void rh_sighandler(int sig)
{
  switch (sig) {
    case SIGTERM:
      if (pid == 0) {
        rh_exit();
        exit(EXIT_SUCCESS);
      } else if (pid > 0) {
        syslog(LOG_NOTICE, "Kill Child PID %d", pid);
        kill(pid, SIGTERM);
      } else {
        syslog(LOG_ERR, "Invalid PID");
        exit(EXIT_FAILURE);
      }
      break;
    case SIGHUP:
      if (pid == 0) {
        rh_reload();
      } else if (pid > 0) {
        syslog(LOG_NOTICE, "Reloading config files");
        kill(pid, SIGHUP);
      } else {
        syslog(LOG_ERR, "Invalid PID");
      }
      break;
    case SIGSEGV:
      {
        void *array[10];
        size_t size;

        size = backtrace (array, 10);

        fprintf (stderr, "Error: signal %d:\n", sig);
        backtrace_symbols_fd (array, size, STDERR_FILENO);

        exit (1);
      }
      break;
  }
  return;
}

void
rh_data_sync (int vserver_id, struct rahunas_member *member)
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
  int  i          = 0;
  static int dl_pos = 0;
  static int ul_pos = 0;

  snprintf(sql, sizeof (sql) - 1,
           "SELECT * FROM dbset WHERE vserver_id='%d' AND session_id='%s' LIMIT 1",
           vserver_id, member->session_id);
  DP("SQL: %s", sql);

  rc = sqlite3_open (RAHUNAS_DB, &connection);
  if (rc) {
    logmsg (RH_LOG_ERROR, "Task DBSET: could not open database, %s",
            sqlite3_errmsg (connection));
    sqlite3_close (connection);
    return;
  }

  sqlite3_busy_timeout (connection, RH_SQLITE_BUSY_TIMEOUT_DEFAULT);

  rc = sqlite3_get_table (connection, sql, &azResult, &nRow, &nColumn,
                          &zErrMsg);
  if (rc == SQLITE_OK && nRow > 0) {
    /* Fetch row data */
    if (dl_pos == 0 || ul_pos == 0) {
      for (i = 0; i < nColumn; i++) {
        colname = azResult[i];
        value = azResult[nColumn + i];

        DP ("Row: %s = %s", colname, value);
        if (COLNAME_MATCH ("download_bytes", colname)) {
          dl_pos = i;
          sscanf (value, "%" SCNd64, &member->download_bytes);
        } else if (COLNAME_MATCH ("upload_bytes", colname)) {
          ul_pos = i;
          sscanf (value, "%" SCNd64, &member->upload_bytes);
        }
      }
    }

    sscanf (azResult[nColumn + dl_pos], "%" SCNd64, &member->download_bytes);
    sscanf (azResult[nColumn + ul_pos], "%" SCNd64, &member->upload_bytes);

    sqlite3_free_table (azResult);
  }

  DP ("Download/Upload bytes: %" PRId64 "/%" PRId64, member->download_bytes,
      member->upload_bytes);

  sqlite3_free (zErrMsg);
  sqlite3_close (connection);
}

static int
expired_check(void *data)
{
  struct processing_set *process = (struct processing_set *) data;
  struct ip_set_req_rahunas *d = NULL;
  struct task_req req;
  unsigned int id;
  char *ip = NULL;
  int res  = 0;
  GList *runner = NULL;
  struct rahunas_member *member = NULL;

  if (process == NULL)
    return (-1);

  if (process->list == NULL)
    return (-1); 

  runner = g_list_first(process->vs->v_map->members);

  while (runner != NULL) {
    pthread_mutex_lock (&RHMtxLock);
    time_t time_now = time (NULL);

    member = (struct rahunas_member *)runner->data;
    runner = g_list_next(runner);

    rh_data_sync (process->vs->vserver_config->vserver_id, member);

    id = member->id;

    if (time_now - member->last_interimupdate >
          process->vs->vserver_config->interim_interval) {
      if (send_xmlrpc_interimupdate (process->vs, id) == 0) {
        member->last_interimupdate = time_now;
      }
    }

    d = get_data_from_set (process->list, id, process->vs->v_map);
    if (d == NULL) {
      pthread_mutex_unlock (&RHMtxLock);
      continue;
    }

    DP("Processing id = %d", id);

    DP("Time diff = %d, idle_timeout=%d", (time_now - d->timestamp),
         process->vs->vserver_config->idle_timeout);

    if (time_now - d->timestamp >
         process->vs->vserver_config->idle_timeout) {
      // Idle Timeout
      DP("Found IP: %s idle timeout", idtoip(process->vs->v_map, id));
      req.id = id;
      memcpy(req.mac_address, &d->ethernet, ETH_ALEN);
      req.req_opt = RH_RADIUS_TERM_IDLE_TIMEOUT;

      if (send_xmlrpc_stopacct(process->vs, id,
            RH_RADIUS_TERM_IDLE_TIMEOUT) == 0) {
        res = rh_task_stopsess(process->vs, &req);
      }
    } else if (member->session_timeout != 0 && 
               time_now > member->session_timeout) {
      // Session Timeout (Expired)
      DP("Found IP: %s session timeout", idtoip(process->vs->v_map, id));
      req.id = id;
      memcpy(req.mac_address, &d->ethernet, ETH_ALEN);
      req.req_opt = RH_RADIUS_TERM_SESSION_TIMEOUT;

      if (send_xmlrpc_stopacct(process->vs, id,
            RH_RADIUS_TERM_SESSION_TIMEOUT) == 0) {
        res = rh_task_stopsess(process->vs, &req);
      }
    } else {
      /* Update session */
      req.id = id;
      memcpy(req.mac_address, &d->ethernet, ETH_ALEN);
      res = rh_task_updatesess (process->vs, &req);
    }

    pthread_mutex_unlock (&RHMtxLock);
  }
}

static int
polling_expired_check (RHMainServer *ms, RHVServer *vs) {
  walk_through_set(expired_check, vs);
  return 0;
}

gboolean polling(gpointer data) {
  if (pthread_mutex_trylock (&RHPollingMtxLock) == 0)
    {
      DP("%s", "Start polling!");
      pthread_cond_signal (&RHPollingCond);
      pthread_mutex_unlock (&RHPollingMtxLock);
    }
  return TRUE;
}

static void
rh_exit (void)
{
  walk_through_vserver(do_task_cleanup, rh_main_server);
  rh_task_stopservice(rh_main_server);
  rh_task_unregister(rh_main_server);
  unregister_vserver_all(rh_main_server);
  rh_closelog(rh_main_server->log_fd);
}

static void
rh_reload (void)
{
  logmsg(RH_LOG_NORMAL, "Reloading config files");
  /* Block polling */
  rh_main_server->polling_blocked = 1;

  if (rh_main_server->main_config->log_file != NULL) {
    syslog(LOG_INFO, "Open log file: %s", 
           rh_main_server->main_config->log_file);
    rh_main_server->log_fd = rh_openlog(rh_main_server->main_config->log_file);

    if (rh_main_server->log_fd == -1) {
      syslog(LOG_ERR, "Could not open log file %s\n", 
             rh_main_server->main_config->log_file);
      exit(EXIT_FAILURE);
    }

    rh_logselect(rh_main_server->log_fd);
  }

  /* Get vserver(s) config, again */
  if (rh_main_server->main_config->conf_dir != NULL) {
    get_vservers_config(rh_main_server->main_config->conf_dir, rh_main_server);
  } else {
    syslog(LOG_ERR, "The main configuration file is incompleted, lack of conf_dir\n");
    exit(EXIT_FAILURE);
  }

  /* Get serviceclass config, again */
  if (rh_main_server->main_config->serviceclass_conf_dir != NULL) {
    get_serviceclass_config(rh_main_server->main_config->serviceclass_conf_dir,
                            rh_main_server);
  } else {
    syslog(LOG_ERR, "The main configuration file is incompleted, lack of serviceclass_conf_dir\n");
    exit(EXIT_FAILURE);
  }

  walk_through_serviceclass(do_serviceclass_reload, rh_main_server);
  serviceclass_unused_cleanup(rh_main_server);

  walk_through_vserver(do_vserver_reload, rh_main_server);
  vserver_unused_cleanup(rh_main_server);
  
  /* Unblock polling */
  rh_main_server->polling_blocked = 0;
  DP("Config reload finished");
  return;
}

static void
watch_child(char *argv[])
{
  char *prog = NULL;
  int failcount = 0;
  time_t start;
  time_t stop;
  int status;
  int nullfd;
  
  if (*(argv[0]) == '(')
    return;

  pid = fork(); 
  if (pid < 0) {
    syslog(LOG_ALERT, "fork failed");
    exit(EXIT_FAILURE);
  } else if (pid > 0) {
    /* parent */
    rh_writepid(DEFAULT_PID, pid);
    exit(EXIT_SUCCESS);
  }

  /* Change the file mode mask */
  umask(0);

  if ((sid = setsid()) < 0)
    syslog(LOG_ALERT, "setsid failed");

  if ((chdir("/")) < 0) {
    exit(EXIT_FAILURE);
  }
    
  /* Close out the standard file descriptors */
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);


  while(1) {
    pid = fork();
    if (pid == 0) {
      /* child */
      prog = strdup(argv[0]);
      argv[0] = strdup("(rahunasd)");
      execvp(prog, argv);
      syslog(LOG_ALERT, "execvp failed");
    } else if (pid < 0) {
      syslog(LOG_ERR, "Could not fork the child process");   
      exit(EXIT_FAILURE);
    }
  
    /* parent */
    syslog(LOG_NOTICE, "RahuNASd Parent: child process %d started", pid);   

    time(&start);

    pid = waitpid(-1, &status, 0);
    time(&stop);

    if (WIFEXITED(status)) {
      syslog(LOG_NOTICE,
               "RahuNASd Parent: child process %d exited with status %d",
               pid, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
      syslog(LOG_NOTICE,
               "RahuNASd Parent: child process %d exited due to signal %d",
               pid, WTERMSIG(status));
    } else {
      syslog(LOG_NOTICE, "RahuNASd Parent: child process %d exited", pid);
    }
  
    if (stop - start < 10)
      failcount++;
    else
      failcount = 0;
  
    if (failcount == 5) {
      syslog(LOG_ALERT, "Exiting due to repeated, frequent failures");
      exit(EXIT_FAILURE);
    }
  
    if (WIFEXITED(status) && (WEXITSTATUS(status) == 0)) {
        syslog(LOG_NOTICE, "Exit Gracefully");
        exit(EXIT_SUCCESS);
    }
    
    sleep(3);
  }
}

void rh_free_member (struct rahunas_member *member)
{
  if (member->username && member->username != termstring)
    free(member->username);

  if (member->session_id && member->session_id != termstring)
    free(member->session_id);

  if (member->serviceclass_name && member->serviceclass_name != termstring)
    free(member->serviceclass_name);

  if (member->mapping_ip && member->mapping_ip != termstring)
    free(member->mapping_ip);
}


int main(int argc, char *argv[])
{
  gchar* addr = "localhost";
  int port    = 8123;
  int fd_log;

  char version[256];

  char line[256];

  union rahunas_config rh_main_config = {
    .rh_main.conf_dir = NULL,
    .rh_main.serviceclass_conf_dir = NULL,
    .rh_main.log_file = NULL,
    .rh_main.dhcp = NULL,
    .rh_main.polling_interval = POLLING,
    .rh_main.bandwidth_shape = BANDWIDTH_SHAPE,
    .rh_main.ip_accounting = IP_ACCOUNTING,
    .rh_main.serviceclass = 0,
  };

  GNetXmlRpcServer *server = NULL;
  GMainLoop* main_loop     = NULL;
  pthread_t  polling_tid;

  signal(SIGTERM, &rh_sighandler);
  signal(SIGHUP, &rh_sighandler);
  signal(SIGSEGV, &rh_sighandler);
  signal(SIGINT, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);

  watch_child(argv);

  /* Get main server config */
  get_config(CONFIG_FILE, &rh_main_config);
  rh_main_server->main_config = (struct rahunas_main_config *) &rh_main_config;

  /* Open and select main log file */
  if (rh_main_server->main_config->log_file != NULL) {
    syslog(LOG_INFO, "Open log file: %s", 
           rh_main_server->main_config->log_file);
    rh_main_server->log_fd = rh_openlog(rh_main_server->main_config->log_file);

    if (rh_main_server->log_fd == -1) {
      syslog(LOG_ERR, "Could not open log file %s\n", 
             rh_main_server->main_config->log_file);
      exit(EXIT_FAILURE);
    }

    rh_logselect(rh_main_server->log_fd);
  }

  syslog(LOG_INFO, "Config directory: %s", rh_main_server->main_config->conf_dir);

  /* Get vserver(s) config */
  if (rh_main_server->main_config->conf_dir != NULL) {
    get_vservers_config(rh_main_server->main_config->conf_dir, rh_main_server);
  } else {
    syslog(LOG_ERR, "The main configuration file is incompleted, lack of conf_dir\n");
    exit(EXIT_FAILURE);
  }

  if (rh_main_server->main_config->serviceclass) {
    /* Get serviceclass config */
    if (rh_main_server->main_config->serviceclass_conf_dir != NULL) {
      get_serviceclass_config(rh_main_server->main_config->serviceclass_conf_dir,
                              rh_main_server);
    } else {
      syslog(LOG_ERR, "The main configuration file is incompleted, lack of serviceclass_conf_dir\n");
      exit(EXIT_FAILURE);
    }
  }


  snprintf(version, sizeof (version), "Starting %s - Version %s", PROGRAM, 
           RAHUNAS_VERSION);
  logmsg(RH_LOG_NORMAL, version);

  rh_task_register(rh_main_server);
  rh_task_startservice(rh_main_server);

  if (rh_main_server->main_config->serviceclass)
    walk_through_serviceclass(do_serviceclass_init, rh_main_server);

  walk_through_vserver(do_task_init, rh_main_server);

  gnet_init();
  main_loop = g_main_loop_new (NULL, FALSE);

  /* XML RPC Server */
  server = gnet_xmlrpc_server_new (addr, port);

  if (!server) {
    syslog(LOG_ERR, "Could not start XML-RPC server!");
    walk_through_vserver(do_task_stopservice, rh_main_server);
    exit (EXIT_FAILURE);
  }

  gnet_xmlrpc_server_register_command (server, 
                                       "startsession", 
                                       do_startsession, 
                                       rh_main_server);

  gnet_xmlrpc_server_register_command (server, 
                                       "stopsession", 
                                       do_stopsession, 
                                       rh_main_server);

  gnet_xmlrpc_server_register_command (server, 
                                       "getsessioninfo", 
                                       do_getsessioninfo, 
                                       rh_main_server);

  gnet_xmlrpc_server_register_command (server,
                                       "roaming",
                                       do_roaming,
                                       rh_main_server);

  DP("Polling interval = %d", rh_main_server->main_config->polling_interval);

  g_timeout_add_seconds (rh_main_server->main_config->polling_interval, 
                         polling, NULL);
 
  walk_through_vserver(do_vserver_init_done, rh_main_server);

  if (pthread_create (&polling_tid, NULL, polling_service,
                      (void *) rh_main_server) != 0)
    {
      perror ("Create polling service");
    }
  else
    {
      pthread_detach (polling_tid);
    }

  macauthen_setup (rh_main_server);

  logmsg(RH_LOG_NORMAL, "Ready to serve...");
  g_main_loop_run(main_loop);

  exit(EXIT_SUCCESS);
}

static int
do_task_init (RHMainServer *ms, RHVServer *vs)
{
  rh_task_init (ms, vs);
  return 0;
}

static int
do_task_cleanup (RHMainServer *ms, RHVServer *vs)
{
  rh_task_cleanup (ms, vs);
  return 0;
}

static int
do_task_stopservice (RHMainServer *ms, RHVServer *vs)
{
  rh_task_stopservice (ms);
  return 0;
}

static int
do_vserver_reload (RHMainServer *ms, RHVServer *vs)
{
  vserver_reload (ms, vs);
  return 0;
}

static int
do_vserver_init_done (RHMainServer *ms, RHVServer *vs)
{
  vserver_init_done (ms, vs);
  return 0;
}

static int
do_serviceclass_init  (RHMainServer *ms, RHSvClass *sc)
{
  serviceclass_init (ms, sc);
  return 0;
}

static int
do_serviceclass_reload(RHMainServer *ms, RHSvClass *sc)
{
  serviceclass_reload (ms, sc);
  return 0;
}

static void *
polling_service (void *data)
{
  RHMainServer *ms = (RHMainServer *)data;

  for (;;)
    {
      pthread_mutex_lock (&RHPollingMtxLock);
      pthread_cond_wait (&RHPollingCond, &RHPollingMtxLock);

      walk_through_vserver(polling_expired_check, ms);

      pthread_mutex_unlock (&RHPollingMtxLock);
    }
}
