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

#include "rahunasd.h"
#include "rh-server.h"
#include "rh-xmlrpc-server.h"
#include "rh-xmlrpc-cmd.h"
#include "rh-ipset.h"
#include "rh-utils.h"
#include "rh-task.h"

const char *termstring = '\0';
pid_t pid, sid;

struct main_server rh_main_server_instance = {
  .vserver_list = NULL,
  .task_list = NULL,
};

struct main_server *rh_main_server = &rh_main_server_instance;

int getline(int fd, char *buf, size_t size);
size_t expired_check(void *data);

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
  }
}

int getline(int fd, char *buf, size_t size)
{
  char cbuf;
  char *current;

  if (!buf || fd < 0)
    return 0;

  current = buf;

  while (read(fd, &cbuf, 1) > 0) {
    *current = cbuf;
    if (cbuf == '\n') {
      *current = '\0';
      break;
    } else if ((current - buf) < (size - 1)) {
      current++;
    }
  }

  return (current - buf);
}

size_t expired_check(void *data)
{
  struct processing_set *process = (struct processing_set *) data;
  struct ip_set_list *setlist = (struct ip_set_list *) process->list;
  size_t offset;
  struct ip_set_rahunas *table = NULL;
  struct task_req req;
  unsigned int id;
  char *ip = NULL;
  int res  = 0;
  GList *runner = g_list_first(process->vs->v_map->members);
  struct rahunas_member *member = NULL;

  if (process == NULL)
    return (-1);

  if (process->list == NULL)
    return (-1); 

  offset = sizeof(struct ip_set_list) + setlist->header_size;
  table = (struct ip_set_rahunas *)(process->list + offset);

  while (runner != NULL) {
    member = (struct rahunas_member *)runner->data;
    runner = g_list_next(runner);

    id = member->id;

    DP("Processing id = %d", id);

    DP("Time diff = %d, idle_timeout=%d", (time(NULL) - table[id].timestamp),
         process->vs->vserver_config->idle_timeout);

    if ((time(NULL) - table[id].timestamp) > 
         process->vs->vserver_config->idle_timeout) {
      // Idle Timeout
      DP("Found IP: %s idle timeout", idtoip(process->vs->v_map, id));
      req.id = id;
      memcpy(req.mac_address, &table[id].ethernet, ETH_ALEN);
      req.req_opt = RH_RADIUS_TERM_IDLE_TIMEOUT;
      send_xmlrpc_stopacct(process->vs, id, 
                           RH_RADIUS_TERM_IDLE_TIMEOUT);
      res = rh_task_stopsess(process->vs, &req);
    } else if (member->session_timeout != 0 && 
               time(NULL) > member->session_timeout) {
      // Session Timeout (Expired)
      DP("Found IP: %s session timeout", idtoip(process->vs->v_map, id));
      req.id = id;
      memcpy(req.mac_address, &table[id].ethernet, ETH_ALEN);
      req.req_opt = RH_RADIUS_TERM_SESSION_TIMEOUT;
      send_xmlrpc_stopacct(process->vs, id, 
                           RH_RADIUS_TERM_SESSION_TIMEOUT);
      res = rh_task_stopsess(process->vs, &req);
    }
  }
}

gboolean polling_expired_check(struct main_server *ms, struct vserver *vs) {
  walk_through_set(&expired_check, vs);
  return TRUE;
}

gboolean polling(gpointer data) {
  struct main_server *ms = (struct main_server *)data;
  struct vserver *vs = NULL;
  DP("%s", "Start polling!");
  walk_through_vserver(&polling_expired_check, ms);
  return TRUE;
}

void rh_exit()
{
  walk_through_vserver(&rh_task_cleanup, rh_main_server);
  rh_task_stopservice(rh_main_server);
  rh_task_unregister(rh_main_server);
  rh_closelog(rh_main_server->main_config->log_file);
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
}

int main(int argc, char **argv) 
{
  gchar* addr = "localhost";
  int port    = 8123;
  int fd_log;

  char version[256];

  char line[256];

  union rahunas_config rh_main_config = {
    .rh_main.conf_dir = NULL,
    .rh_main.log_file = NULL,
    .rh_main.dhcp = NULL,
    .rh_main.polling_interval = POLLING,
    .rh_main.bandwidth_shape = BANDWIDTH_SHAPE,
  };

  GNetXmlRpcServer *server = NULL;
  GMainLoop* main_loop     = NULL;

  signal(SIGTERM, rh_sighandler);

  watch_child(argv);

  /* Get main server config */
  get_config(CONFIG_FILE, &rh_main_config);
  rh_main_server->main_config = (struct rahunas_main_config *) &rh_main_config;

  /* Open and select main log file */
  if (rh_main_server->main_config->log_file != NULL) {
    syslog(LOG_INFO, "Open log file: %s", 
           rh_main_server->main_config->log_file);
    rh_main_server->log_fd = rh_openlog(rh_main_server->main_config->log_file);

    if (!rh_main_server->log_fd) {
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

  sprintf(version, "Starting %s - Version %s", PROGRAM, RAHUNAS_VERSION);
  logmsg(RH_LOG_NORMAL, version);

  rh_task_register(rh_main_server);
  rh_task_startservice(rh_main_server);

  walk_through_vserver(&rh_task_init, rh_main_server);

  gnet_init();
  main_loop = g_main_loop_new (NULL, FALSE);

  /* XML RPC Server */
  server = gnet_xmlrpc_server_new (addr, port);

  if (!server) {
    syslog(LOG_ERR, "Could not start XML-RPC server!");
    walk_through_vserver(&rh_task_stopservice, rh_main_server);
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

  DP("Polling interval = %d", rh_main_server->main_config->polling_interval);

  g_timeout_add_seconds (rh_main_server->main_config->polling_interval, 
                         polling, rh_main_server);


  logmsg(RH_LOG_NORMAL, "Ready to serve...");
  g_main_loop_run(main_loop);

  exit(EXIT_SUCCESS);
}
