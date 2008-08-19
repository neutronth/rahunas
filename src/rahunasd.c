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
#include <time.h>
#include <signal.h>
#include <syslog.h>

#include "rahunasd.h"
#include "rh-xmlrpc-server.h"
#include "ipset-control.h"

/* Abstract functions */
int logmsg(int priority, const char *msg, ...); 
int getline(int fd, char *buf, size_t size);
int finish();
int ipset_flush();
int chk_set(struct rahunas_map *map);
int update_set(struct rahunas_map *map);

struct rahunas_map* rh_init_map();
int rh_init_members(struct rahunas_map *map);

int parse_set_header(const char *in, struct rahunas_map *map);
int parse_set_list(const char *in, struct rahunas_map *map);

int send_xmlrpc_stopacct(struct rahunas_map *map, uint32_t id);

/* Declaration */
struct rahunas_map *map = NULL;

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
	  return NULL;

  sess_addr.s_addr = htonl((ntohl(map->first_ip) + id));

  return inet_ntoa(sess_addr);
}

void *rh_malloc(size_t size)
{
  void *p;
  
  if (size == 0)
    return NULL;

  if ((p = malloc(size)) == NULL) {
    syslog(LOG_ERR, "RahuNASd: not enough memory");
    exit(EXIT_FAILURE);
  } 

  return p;
}

void rh_free(void **data)
{
  if (*data == NULL)
    return;

  free(*data);
  *data = NULL;
}

int rh_openlog(const char *filename)
{
  return open(filename, O_WRONLY | O_APPEND);
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

void rh_shutdown(int sig)
{
  ipset_flush();
  finish();
  rh_closelog(DEFAULT_LOG);
}

int ipset_flush()
{
  logmsg(RH_LOG_NORMAL, "Flushing IPSET...");
  return ctrl_flush();
}

int finish()
{
  char *exitmsg = "Exit Gracefully";
	struct rahunas_member *members = NULL;
	int i;
  int end;

  if (map) {
    if (map->members) {
      members = map->members;
      end = map->size;
    } else {
      end = 0;
    }  

	  for (i=0; i < end; i++) {
			  rh_free(&(members[i].username));
			  rh_free(&(members[i].session_id));
		}

		rh_free(&(map->members));
		rh_free(&map);
	}

  logmsg(RH_LOG_NORMAL, exitmsg);
	syslog(LOG_INFO, exitmsg);
  return 0;
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


int parse_set_header(const char *in, struct rahunas_map *map)
{
 	in_addr_t first_ip;
	in_addr_t last_ip;
	char *ip_from_start = NULL;
  char *ip_from_end = NULL;
	char *ip_to_start = NULL;
  char *ip_to_end = NULL;
	char *ip_from = NULL;
  char *ip_to = NULL;

	if (!map || !in)
	  return (-1);

  ip_from_start = strstr(in, "from:");
	if (ip_from_start == NULL)
	  return (-1);

	ip_to_start = strstr(in, "to:");
	if (ip_to_start == NULL)
	  return (-1);

  ip_from_start += 6;
	ip_from_end = ip_to_start - 1;
	ip_to_start += 4;
	ip_to_end = in + strlen(in);

	ip_from = strndup(ip_from_start, ip_from_end - ip_from_start);
	ip_to   = strndup(ip_to_start, ip_to_end - ip_to_start);
	logmsg(RH_LOG_NORMAL, "First IP: %s", ip_from);
	logmsg(RH_LOG_NORMAL, "Last IP: %s", ip_to);

	first_ip = inet_addr(ip_from);
  memcpy(&map->first_ip, &first_ip, sizeof(in_addr_t));

	last_ip = inet_addr(ip_to);
  memcpy(&map->last_ip, &last_ip, sizeof(in_addr_t));

	map->size = ntohl(map->last_ip) - ntohl(map->first_ip) + 1;

	logmsg(RH_LOG_NORMAL, "Set Size: %lu", map->size);
	free(ip_from);
	free(ip_to);

	return 0;
}

int parse_set_list(const char *in, struct rahunas_map *map)
{
	char *sep = NULL;
  char *sess_ip = NULL;
	char *sess_idle_time = NULL;
	uint32_t  id;

	struct rahunas_member *members;

	if (!map || !in)
	  return (-1);

  if (!map->members)
    return (-1);
	
	members = map->members;

  // Found members
  DP("Parse from list: %s", in);
	sep = strstr(in, ":");
	sess_ip = strndup(in, sep - in);
  if (sess_ip == NULL)
    return (-1);
	id = iptoid(map, sess_ip);
 
	sess_idle_time = strndup(sep + 1, strstr(in, "seconds") - sep - 1);
  if (sess_idle_time == NULL)
    return (-1);

  members[id].flags = 1;
  members[id].expired = atoi(sess_idle_time) < IDLE_THRESHOLD ? 0 : 1;

  free(sess_ip);
  free(sess_idle_time);
	return id;
}

int chk_set(struct rahunas_map *map) 
{
  struct rahunas_member *members = NULL;
	unsigned short set_start = 0;
	char  line[256];
  int   pipefd[2];
	pid_t cpid;
	uint32_t  id;

	members = map->members;

	if (pipe(pipefd) == -1) {
    syslog(LOG_ERR, "Could not create pipe");
		return -1;
	}

	cpid = fork();
	if (cpid == 0) {
	  close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		execlp("ipset", "ipset", "-nL", SET_NAME, NULL);
		return (-1);
		close(pipefd[1]);
	}

  wait(NULL);

  close(pipefd[1]);

	while (getline(pipefd[0], line, sizeof line) > 0) {

    if (strstr(line, "Bindings:") != NULL)
		  set_start = 0;

		if (set_start)
		  parse_set_list(line, map);

		// Capture header
    if (strstr(line, "Header:") != NULL)
		  if (map != NULL && map->size == 0) {
        parse_set_header(line, map);
				break;
			}

		if (strstr(line, "Members:") != NULL)
		  set_start = 1;
	}
	close(pipefd[0]); 

	return 0;
}


int update_set(struct rahunas_map *map) 
{
  struct rahunas_member *members = NULL;
  unsigned int i;
  unsigned short no_detail = 0;
 
  if (!map)
	  return (-1);
	
	members = map->members;

	for (i=0; i < map->size; i++) {
    no_detail = 0;
    if (members[i].flags) {
      if (!members[i].username || !members[i].session_id)
        no_detail = 1;
 
    

		  if (members[i].expired) {
    	  if (!no_detail) {
    		    DP("IP %s, Client: Username %s, "
    		       "Session-ID %s, Session-Start %d, "
               "Expired %d",
               idtoip(map, i),
    			     members[i].username, 
    			     members[i].session_id,
    			     members[i].session_start,
               members[i].expired);

			      send_xmlrpc_stopacct(map, i);
          }

        if (ctrl_del_from_set(map, i) == 0) {
          if (!no_detail) {
            logmsg(RH_LOG_NORMAL, "Session Stop, User: %s, IP: %s, "
                            "Session ID: %s",
                            members[i].username, 
                            idtoip(map, i), 
                            members[i].session_id); 
          }

            rh_free(&(members[i].username));
            rh_free(&(members[i].session_id));

			    memset(&members[i], 0, sizeof(struct rahunas_member));
			    logmsg(RH_LOG_NORMAL, "Client IP %s was removed!", idtoip(map,i));
			  }
			}
		}
	}
  return 0;
}

int send_xmlrpc_stopacct(struct rahunas_map *map, uint32_t id) {
  struct rahunas_member *members = NULL;
  GNetXmlRpcClient *client = NULL;
  gchar *reply  = NULL;
	gchar *params = NULL;

	if (!map)
	  return (-1);

  if (!map->members)
    return (-1);

  if (id < 0 || id > (map->size - 1))
    return (-1);
	
	members = map->members;

  client = gnet_xmlrpc_client_new(XMLSERVICE_HOST, XMLSERVICE_URL, 
	                                XMLSERVICE_PORT);

  if (!client) {
    logmsg(RH_LOG_ERROR, "Could not connect to XML-RPC service");
    return (-1);
  }
	
	params = g_strdup_printf("%s|%s|%d", 
	                         members[id].username,
													 members[id].session_id,
													 members[id].session_start);

  if (params == NULL)
    return (-1);
  
  if (gnet_xmlrpc_client_call(client, "stopacct", params, &reply) == 0)
    {
      DP("stopacct reply = %s", reply);
      g_free(reply);
    }
  else
    DP("%s", "Failed executing stopacct!");
	
	g_free(params);

  return 0;
}


struct rahunas_map* rh_init_map() {
  struct rahunas_map *map = NULL;

	map = (struct rahunas_map*)(rh_malloc(sizeof(struct rahunas_map)));

  map->members = NULL;
	map->size = 0;

	return map;
}

int rh_init_members (struct rahunas_map* map)
{
	struct rahunas_member *members = NULL;
	int size;

  if (!map)
	  return (-1);
	
	size = map->size == 0 ? MAX_MEMBERS : map->size;

	members = 
    (struct rahunas_member*)(rh_malloc(sizeof(struct rahunas_member)*size));

	memset(members, 0, sizeof(struct rahunas_member)*size);

	map->members = members;

	return 0;
}

gboolean polling(gpointer data) {
  struct rahunas_map *map = (struct rahunas_map *)data;
	DP("%s", "Start polling!");
	chk_set (map);
	update_set (map);
  return TRUE;
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

  pid_t pid, sid;

	if (*(argv[0]) == '(')
	  return;

  signal(SIGINT, rh_shutdown);
  signal(SIGTERM, rh_shutdown);
  signal(SIGKILL, rh_shutdown);

  pid = fork();	
	if (pid < 0) {
	  syslog(LOG_ALERT, "fork failed");
		exit(EXIT_FAILURE);
	} else if (pid > 0) {
	  /* parent */
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
  	pid = wait3(&status, 0, NULL);
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
  	
  	if (WIFSIGNALED(status)) {
      switch (WTERMSIG(status)) {
  		  case SIGKILL:
  			  exit(0);
  				break;
  			
  			case SIGINT:
  			case SIGTERM:
  			  syslog(LOG_ALERT, "Exiting due to unexpected forced shutdown");
  				exit(EXIT_FAILURE);
  				break;
  			
  			default:
  			  break;
  		}
  	}
  	
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

	watch_child(argv);
  
  gnet_init();
  main_loop = g_main_loop_new (NULL, FALSE);

  sprintf(version, "Starting %s - Version %s", PROGRAM, VERSION);

  /* Open log file */
 	if ((fd_log = rh_openlog(DEFAULT_LOG)) == (-1)) {
    syslog(LOG_ERR, "Could not open log file %s", DEFAULT_LOG);
    exit(EXIT_FAILURE);
  }

  dup2(fd_log, STDERR_FILENO);

	logmsg(RH_LOG_NORMAL, version);
  syslog(LOG_INFO, version);

  map = rh_init_map();
  chk_set(map);
  rh_init_members(map);

  /* XML RPC Server */
	server = gnet_xmlrpc_server_new (addr, port);

	if (!server) {
    syslog(LOG_ERR, "Could not start XML-RPC server!");
    ipset_flush();
    finish(); 
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

  g_timeout_add_seconds (POLLING, polling, map);

	g_main_loop_run(main_loop);

	exit(EXIT_SUCCESS);
	return 0;
}
