/**
 * utility functions header
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-08-31
 */

#include <string.h>
#include <syslog.h>
#include <glib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "rahunasd.h"
#include "rh-utils.h"

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

gchar *rh_string_get_sep(const char *haystack, const char *sep,
                         unsigned short idx)
{
  gchar *result = NULL;
  const gchar *pStart = NULL;
  const gchar *pEnd   = NULL;
  const gchar *pLast  = NULL;
  unsigned short current_idx = 0;
  unsigned short isFound = 0;

  if (!haystack) {
    return NULL;
  }

  pEnd = haystack;
  pLast = haystack + strlen(haystack);
  while (!isFound) {
    pStart = pEnd;
    if (pStart == pLast) {
      // Finding the separator fail
      return NULL;
    }

    pEnd = g_strstr_len(pStart, strlen(pStart), sep);  
    pEnd = pEnd == NULL ? pLast : pEnd;

    current_idx++;

    if (current_idx == idx) {
      result = g_strndup(pStart, pEnd - pStart);
      result = g_strchug (result);
      result = g_strchomp (result);
      return result;
    } else {
      pEnd++;
    }
  }
}

int rh_openlog(const char *filename)
{
  return open(filename, O_WRONLY | O_APPEND | O_CREAT,
              S_IRWXU | S_IRGRP | S_IROTH);
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

  rh_free((void **) &p);
  rh_free((void **) &np);
}


int rh_writepid(const char *pidfile, int pid)
{
  int pidfd;

  pidfd = open(DEFAULT_PID, O_WRONLY | O_TRUNC | O_CREAT,
               S_IRWXU | S_IRGRP | S_IROTH);
  if (pidfd) {
    dup2(pidfd, STDOUT_FILENO);
    fprintf(stdout, "%d\n", pid);
    close(pidfd);
  }
}

int rh_logselect(int fd)
{
  dup2(fd, STDERR_FILENO);
  return 0;
}

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

const char *idtoip(struct rahunas_map *map, uint32_t id) {
  struct in_addr sess_addr;

  if (!map)
    return termstring;

  if (id < 0)
    return termstring;

  sess_addr.s_addr = htonl((ntohl(map->first_ip) + id));

  return inet_ntoa(sess_addr);
}


