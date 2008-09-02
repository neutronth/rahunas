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
#include "rh-xmlrpc-cmd.h"
#include "rh-ipset.h"
#include "rh-utils.h"

/* Abstract functions */
int logmsg(int priority, const char *msg, ...); 
int getline(int fd, char *buf, size_t size);
int finish();
int ipset_flush();

struct rahunas_map* rh_init_map();
int rh_init_members(struct rahunas_map *map);

int walk_through_set (int (*callback)(void *));
size_t expired_check(void *data);
size_t nas_reboot(void *data);

/* Declaration */
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
        walk_through_set(&nas_reboot);
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

int ipset_flush()
{
  syslog(LOG_NOTICE, "Flushing IPSET...");
  set_flush(SET_NAME);
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
			  rh_free_member(&members[i]);
		}

		rh_free(&(map->members));
		rh_free(&map);
	}

  rh_free(&rahunas_set);
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
  unsigned int i;
  char *ip = NULL;
  ip_set_ip_t current_ip;
  int res  = 0;

  offset = sizeof(struct ip_set_list) + setlist->header_size;
  table = (struct ip_set_rahunas *)(data + offset);

  DP("Map size %d", map->size);
 
  for (i = 0; i < map->size; i++) {
    if (test_bit(IPSET_RAHUNAS_ISSET,
          (void *)&table[i].flags)) {
      if ((time(NULL) - table[i].timestamp) > IDLE_THRESHOLD) {
        DP("Found IP: %s expired", idtoip(map, i));
        current_ip = ntohl(map->first_ip) + i;
        res = set_adtip_nb(rahunas_set, &current_ip, &table[i].ethernet,
                           IP_SET_OP_DEL_IP);  
        DP("set_adtip_nb() res=%d errno=%d", res, errno);

        if (res == 0) {
			    send_xmlrpc_stopacct(map, i, RH_RADIUS_TERM_IDLE_TIMEOUT);

          if (!members[i].username)
            members[i].username = termstring;

          if (!members[i].session_id)
            members[i].session_id = termstring;

          logmsg(RH_LOG_NORMAL, "Session Idle-Timeout, User: %s, IP: %s, "
                                "Session ID: %s, MAC: %s",
                                members[i].username, 
                                idtoip(map, i), 
                                members[i].session_id,
                                mac_tostring(members[i].mac_address)); 

          rh_free_member(&members[i]);
        }
      } 
    }
  }
}

size_t nas_reboot(void *data)
{
  struct ip_set_list *setlist = (struct ip_set_list *) data;
  struct set *set = set_list[setlist->index];
  size_t offset;
  struct ip_set_rahunas *table = NULL;
  struct rahunas_member *members = map->members;
  unsigned int i;
  char *ip = NULL;
  ip_set_ip_t current_ip;
  int res  = 0;

  offset = sizeof(struct ip_set_list) + setlist->header_size;
  table = (struct ip_set_rahunas *)(data + offset);

  DP("Map size %d", map->size);
 
  for (i = 0; i < map->size; i++) {
    if (test_bit(IPSET_RAHUNAS_ISSET, (void *)&table[i].flags)) {
      DP("Found IP: %s in set, try logout", idtoip(map, i));
      current_ip = ntohl(map->first_ip) + i;
      res = set_adtip_nb(rahunas_set, &current_ip, &table[i].ethernet,
                         IP_SET_OP_DEL_IP);  
      DP("set_adtip_nb() res=%d errno=%d", res, errno);

      if (res == 0) {
			    send_xmlrpc_stopacct(map, i, RH_RADIUS_TERM_NAS_REBOOT);

        if (!members[i].username)
          members[i].username = termstring;

        if (!members[i].session_id)
          members[i].session_id = termstring;

        logmsg(RH_LOG_NORMAL, "Session Stop (NAS Reboot), User: %s, IP: %s, "
                              "Session ID: %s, MAC: %s",
                              members[i].username, 
                              idtoip(map, i), 
                              members[i].session_id,
                              mac_tostring(members[i].mac_address)); 

        rh_free_member(&members[i]);
      }
    }
  }
}

int get_header_from_set ()
{
  struct ip_set_req_rahunas_create *header = NULL;
  void *data = NULL;
  ip_set_id_t idx;
  socklen_t size, req_size;
  size_t offset;
  int res = 0;
 	in_addr_t first_ip;
	in_addr_t last_ip;

  size = req_size = load_set_list(SET_NAME, &idx, 
                                  IP_SET_OP_LIST_SIZE, CMD_LIST); 

  DP("Get Set Size: %d", size);
  
  if (size) {
    data = rh_malloc(size);
    ((struct ip_set_req_list *) data)->op = IP_SET_OP_LIST;
    ((struct ip_set_req_list *) data)->index = idx;
    res = kernel_getfrom_handleerrno(data, &size);
    DP("get_lists getsockopt() res=%d errno=%d", res, errno);

    if (res != 0 || size != req_size) {
      free(data);
      return -EAGAIN;
    }
    size = 0;
  }

  offset = sizeof(struct ip_set_list);
  header = (struct ip_set_req_rahunas_create *) (data + offset);

  first_ip = htonl(header->from); 
  last_ip = htonl(header->to); 
  
  memcpy(&map->first_ip, &first_ip, sizeof(in_addr_t));
  memcpy(&map->last_ip, &last_ip, sizeof(in_addr_t));
	map->size = ntohl(map->last_ip) - ntohl(map->first_ip) + 1;

	logmsg(RH_LOG_NORMAL, "First IP: %s", ip_tostring(ntohl(map->first_ip)));
	logmsg(RH_LOG_NORMAL, "Last  IP: %s", ip_tostring(ntohl(map->last_ip)));
	logmsg(RH_LOG_NORMAL, "Set Size: %lu", map->size);

  rh_free(&data);
  return res;
}

int walk_through_set (int (*callback)(void *))
{
  void *data = NULL;
  ip_set_id_t idx;
  socklen_t size, req_size;
  int res = 0;

  size = req_size = load_set_list(SET_NAME, &idx, 
                                  IP_SET_OP_LIST_SIZE, CMD_LIST); 

  DP("Get Set Size: %d", size);
  
  if (size) {
    data = rh_malloc(size);
    ((struct ip_set_req_list *) data)->op = IP_SET_OP_LIST;
    ((struct ip_set_req_list *) data)->index = idx;
    res = kernel_getfrom_handleerrno(data, &size);
    DP("get_lists getsockopt() res=%d errno=%d", res, errno);

    if (res != 0 || size != req_size) {
      free(data);
      return -EAGAIN;
    }
    size = 0;
  }
  
  if (data != NULL)
    (*callback)(data);

  rh_free(&data);
  return res;
}

void rh_exit()
{
  syslog(LOG_ALERT, "Child Exiting ..");
  ipset_flush();
  finish();
  rh_closelog(DEFAULT_LOG);
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

  /* Open log file */
 	if ((fd_log = rh_openlog(DEFAULT_LOG)) == (-1)) {
    syslog(LOG_ERR, "Could not open log file %s", DEFAULT_LOG);
    exit(EXIT_FAILURE);
  }

  dup2(fd_log, STDERR_FILENO);

  gnet_init();
  main_loop = g_main_loop_new (NULL, FALSE);

  sprintf(version, "Starting %s - Version %s", PROGRAM, VERSION);

  
	logmsg(RH_LOG_NORMAL, version);
  syslog(LOG_INFO, version);

  rahunas_set = set_adt_get(SET_NAME);
  DP("getsetname: %s", rahunas_set->name);
  DP("getsetid: %d", rahunas_set->id);
  DP("getsetindex: %d", rahunas_set->index);

  map = rh_init_map();
  get_header_from_set();
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
}
