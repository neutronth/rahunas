/**
 * RahuNASd
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-08-07
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>

#include "rahunasd.h"
#include "rh-xmlrpc-server.h"
#include "rh-xmlrpc-cmd.h"
#include "rh-ipset.h"
#include "rh-utils.h"
#include "rh-task.h"

/* Abstract functions */
int logmsg(int priority, const char *msg, ...); 
int getline(int fd, char *buf, size_t size);

size_t expired_check(void *data);

/* Declaration */
struct rahunas_config rh_config;
struct rahunas_map *map = NULL;
struct set *rahunas_set = NULL;

struct set **set_list = NULL;
ip_set_id_t max_sets = 0;

const char *termstring = '\0';
pid_t pid, sid;

uint32_t iptoid(struct rahunas_map *map, const char *ip) {
  uint32_t ret;
  struct in_addr req_ip;

  if (!map || !ip)
	  return (-1);

  if (!(inet_aton(ip, &req_ip))) {
    DP("Could not convert IP: %s", ip);
    return (-1);  
  }

  DP("Request IP: %s", ip);
	
  ret = ntohl(req_ip.s_addr) - ntohl(map->first_ip);
	if (ret < 0 || ret > (map->size - 1))
	  ret = (-1);

  DP("Request Index: %lu", ret);
  return ret; 
}

char *idtoip(struct rahunas_map *map, uint32_t id) {
  struct in_addr sess_addr;

  if (!map)
    return termstring;

  if (id < 0)
    return termstring;

  sess_addr.s_addr = htonl((ntohl(map->first_ip) + id));

  return inet_ntoa(sess_addr);
}

void rh_free_member (struct rahunas_member *member)
{
  if (member->username && member->username != termstring)
    free(member->username);

  if (member->session_id && member->session_id != termstring)
    free(member->session_id);
  
  memset(member, 0, sizeof(struct rahunas_member));
  member->username = termstring;
  member->session_id = termstring;
}

int rh_openlog(const char *filename)
{
  return open(filename, O_WRONLY | O_APPEND | O_CREAT);
}

int rh_closelog(int fd)
{
  if (close(fd) == 0)
	  return 1;
	else
	  return 0;
}

int logmsg(int priority, const char *msg, ...) 
{
  int n, size = 256;
	va_list ap;
	char *time_fmt = "%b %e %T";
	char *p = NULL;
  char *np = NULL;

	if (priority < RH_LOG_LEVEL)
	  return 0;

	if ((p = rh_malloc(size)) == NULL) {
		return (-1);
	}

	while (1) {
    va_start(ap, msg);
		n = vsnprintf(p, size, msg, ap);
		va_end(ap);

		if (n > -1 && n < size)
		  break;
 
    if (n > -1)
		  size = n+1;
		else
		  size *= 2;

		if ((np = realloc(p, size)) == NULL) {
      free(p);
      p = NULL;
			break;
		} else {
      p = np;
		}
	}

	if (!p)
	  return (-1);

	fprintf(stderr, "%s : %s\n", timemsg(), p);

	rh_free(&p);
	rh_free(&np);
}

void rh_sighandler(int sig)
{
  switch (sig) {
    case SIGINT:
    case SIGTERM:
    case SIGKILL:
      if (pid == 0) {
        rh_exit();
        exit(EXIT_SUCCESS);
      }

      if (pid != 0) {
        syslog(LOG_NOTICE, "Kill Child PID %d", pid);
        kill(pid, SIGTERM);
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

gboolean polling(gpointer data) {
  struct rahunas_map *map = (struct rahunas_map *)data;
	DP("%s", "Start polling!");
  walk_through_set(&expired_check);
  return TRUE;
}

size_t expired_check(void *data)
{
  struct ip_set_list *setlist = (struct ip_set_list *) data;
  struct set *set = set_list[setlist->index];
  size_t offset;
  struct ip_set_rahunas *table = NULL;
  struct rahunas_member *members = map->members;
  struct task_req req;
  unsigned int i;
  char *ip = NULL;
  int res  = 0;

  offset = sizeof(struct ip_set_list) + setlist->header_size;
  table = (struct ip_set_rahunas *)(data + offset);

  DP("Map size %d", map->size);
 
  for (i = 0; i < map->size; i++) {
    if (test_bit(IPSET_RAHUNAS_ISSET, (void *)&table[i].flags)) {
      if ((time(NULL) - table[i].timestamp) > rh_config.idle_threshold) {
        // Idle Timeout
        DP("Found IP: %s idle timeout", idtoip(map, i));
        req.id = i;
        memcpy(req.mac_address, &table[i].ethernet, ETH_ALEN);
        req.req_opt = RH_RADIUS_TERM_IDLE_TIMEOUT;
			  send_xmlrpc_stopacct(map, i, RH_RADIUS_TERM_IDLE_TIMEOUT);
        res = rh_task_stopsess(map, &req);
      } else if (members[i].session_timeout != 0 && 
                   time(NULL) > members[i].session_timeout) {
        // Session Timeout (Expired)
        DP("Found IP: %s session timeout", idtoip(map, i));
        req.id = i;
        memcpy(req.mac_address, &table[i].ethernet, ETH_ALEN);
        req.req_opt = RH_RADIUS_TERM_SESSION_TIMEOUT;
			  send_xmlrpc_stopacct(map, i, RH_RADIUS_TERM_SESSION_TIMEOUT);
        res = rh_task_stopsess(map, &req);
      }
    }
  }
}

void rh_exit()
{
  syslog(LOG_ALERT, "Child Exiting ..");
  rh_task_stopservice(map);
  rh_task_cleanup();
  rh_closelog(rh_config.log_file);
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
  int pidfd;
  
	if (*(argv[0]) == '(')
	  return;


  pid = fork();	
	if (pid < 0) {
	  syslog(LOG_ALERT, "fork failed");
		exit(EXIT_FAILURE);
	} else if (pid > 0) {
	  /* parent */
    pidfd = open(DEFAULT_PID, O_WRONLY | O_TRUNC | O_CREAT);
    if (pidfd) {
      dup2(pidfd, STDOUT_FILENO);
      fprintf(stdout, "%d\n", pid);
      close(pidfd);
    }
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

	  if ((pid = fork()) == 0) {
      /* child */
      prog = strdup(argv[0]);
	    argv[0] = strdup("(rahunasd)");
		  execvp(prog, argv);
		  syslog(LOG_ALERT, "execvp failed");
		}
  
    syslog(LOG_NOTICE, "RahuNASd Parent: child process %d started", pid);   

    time(&start);
	  /* parent */
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
  
  
  	if (WIFEXITED(status))
  	  if (WEXITSTATUS(status) == 0)
  		  exit(EXIT_SUCCESS);
  	
  	sleep(3);
  }
}

int main(int argc, char **argv) 
{
	gchar* addr = "localhost";
	int port    = 8123;
  int fd_log;

	char line[256];
	char version[256];

	GNetXmlRpcServer *server = NULL;
	GMainLoop* main_loop     = NULL;

   

  signal(SIGTERM, rh_sighandler);
  signal(SIGKILL, rh_sighandler);

	watch_child(argv);

  /* Get configuration from config file */
  if (config_init(&rh_config) < 0) {
	  syslog(LOG_ERR, "Could not open config file %s", CONFIG_FILE);
    exit(EXIT_FAILURE);
  }

  /* Open log file */
 	if ((fd_log = rh_openlog(rh_config.log_file)) < 0) {
    syslog(LOG_ERR, "Could not open log file %s", rh_config.log_file);
    exit(EXIT_FAILURE);
  }

  dup2(fd_log, STDERR_FILENO);

  sprintf(version, "Starting %s - Version %s", PROGRAM, RAHUNAS_VERSION);
	logmsg(RH_LOG_NORMAL, version);
  syslog(LOG_INFO, version);

  rh_task_init();

  gnet_init();
  main_loop = g_main_loop_new (NULL, FALSE);

  /* XML RPC Server */
	server = gnet_xmlrpc_server_new (addr, port);

	if (!server) {
    syslog(LOG_ERR, "Could not start XML-RPC server!");
    rh_task_stopservice(map);
	  exit (EXIT_FAILURE);
	}

	gnet_xmlrpc_server_register_command (server, 
	                                     "startsession", 
	   																   do_startsession, 
			  															 map);

	gnet_xmlrpc_server_register_command (server, 
	                                     "stopsession", 
	   																   do_stopsession, 
			  															 map);

  gnet_xmlrpc_server_register_command (server, 
	                                     "getsessioninfo", 
	   																   do_getsessioninfo, 
			  															 map);

  g_timeout_add_seconds (rh_config.polling_interval, polling, map);

  rh_task_startservice(map);

	g_main_loop_run(main_loop);

	exit(EXIT_SUCCESS);
}
