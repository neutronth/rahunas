/**
 * ipset control implementation
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
#include <syslog.h>

#include "ipset-control.h"

int _issue_ipset_cmd(unsigned short cmd_type, const char *ip)
{
	char *cmd = NULL; 
	int   idx  = 0;
	char  line[256];
	int   pipefd[2];
  int   status;
	char  buf;
	pid_t cpid;
	
	if (cmd_type == RH_IPSET_CMD_ADD)
	  cmd = strdup("-A");
	else if (cmd_type == RH_IPSET_CMD_DEL)
	  cmd = strdup("-D");
	else if (cmd_type == RH_IPSET_CMD_FLS)
    cmd = strdup("-F");
	else
	  return (-1);

  if (pipe(pipefd) == (-1)) {
    syslog(LOG_ERR, "Could not create pipe");
		return (-1);
	}

	cpid = fork();
	if (cpid == 0) {
    close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
    
    if (ip == NULL)
		  logmsg(RH_LOG_DEBUG, "ipset %s %s", cmd, SET_NAME);
    else
		  logmsg(RH_LOG_DEBUG, "ipset %s %s %s", cmd, SET_NAME, ip);

		execlp("ipset", "ipset", cmd, SET_NAME, ip, NULL);
		close(pipefd[1]);
		return (-1);
	}

  cpid = wait(&status);

  close(pipefd[1]);
	while (getline(pipefd[0], line, sizeof line) > 0) {
    logmsg(RH_LOG_DEBUG, line);
	}
	close(pipefd[0]);

	if (cmd)
	  free(cmd);

  if (WIFEXITED(status) && (WEXITSTATUS(status) == 0))
    return 0;
  else
	  return (-1);
}

int ctrl_add_to_set(struct rahunas_map *map, uint32_t id)
{
	struct rahunas_member *members = NULL;
  struct in_addr sess_addr;
	int ret;

	if (!map)
	  return (-1);

  if (!map->members)
    return (-1);

  members = map->members;

	if (id > ((map->size) - 1) || id < 0)
	  return (-1);

	if (members[id].flags)
	  return 0;
		
  ret = _issue_ipset_cmd(RH_IPSET_CMD_ADD, idtoip(map, id));

	chk_set(map);

	if (ret == 0 && members[id].flags)
    return 0;
	else
	  return (-1);
}

int ctrl_del_from_set(struct rahunas_map *map, uint32_t id)
{
	struct rahunas_member *members = NULL;
  struct in_addr sess_addr;
	int ret;

	if (!map)
	  return (-1);

  if (!map->members)
    return (-1);

  members = map->members;

	if (id > ((map->size) - 1) || id < 0)
	  return (-1);

	if (!members[id].flags)
	  return 0;
	
  ret = _issue_ipset_cmd(RH_IPSET_CMD_DEL, idtoip(map, id));

	if (ret == 0)
	  members[id].flags = 0;
	
	return ret;
}

int ctrl_flush()
{
  return _issue_ipset_cmd(RH_IPSET_CMD_FLS, NULL);
}
