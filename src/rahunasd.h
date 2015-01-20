/**
 * RahuNASd header 
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-08-07
 */

#ifndef __RAHUNASD_H
#define __RAHUNASD_H

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <linux/if_ether.h>
#include <errno.h>

#include "rh-config.h"
#include "rh-utils.h"

#define MAX_MEMBERS 0x00FFFF

#define DEFAULT_MAC "00:00:00:00:00:00"
#define RH_SQLITE_BUSY_TIMEOUT_DEFAULT  10000  /* 10 seconds */

extern const char *termstring; 
extern RHMainServer rh_main_server_instance;
extern pthread_mutex_t RHMtxLock;

enum RH_LOG {
  RH_LOG_DEBUG,
  RH_LOG_NORMAL,
  RH_LOG_ERROR
};

#define RH_LOG_LEVEL RH_LOG_NORMAL

#ifdef RH_DEBUG
#define DP(format, args...) \
  do {  \
    fprintf(stderr, "%s - %s: %s (DBG): ", timemsg(), __FILE__, __FUNCTION__); \
    fprintf(stderr, format "\n", ## args); \
  } while (0)
#else
#define DP(format, args...)
#endif

enum saved_bandwidth_setting {
  SAVED_DEFAULT = 0,
  SAVED_CURRENT = 1
};

struct rahunas_map {
  GList *members;
  in_addr_t first_ip;
  in_addr_t last_ip;
  unsigned int size;
};

struct rahunas_member {
  struct vserver *vs;
  uint32_t id; 
  unsigned short expired;
  time_t session_start;
  time_t session_timeout;
  long saved_bandwidth_max_down[2];
  long saved_bandwidth_max_up[2];
  long bandwidth_max_down;
  long bandwidth_max_up;
  uint16_t bandwidth_slot_id;
  char *serviceclass_name;
  const char *serviceclass_description;
  uint32_t serviceclass_slot_id;
  char *mapping_ip;
  char *username;
  char *session_id;
  unsigned char mac_address[ETH_ALEN];
  uint64_t download_bytes;
  uint64_t upload_bytes;
  time_t   last_interimupdate;
  int      interim_interval;
  char     secure_token[65];
  void    *luastate;
  time_t   last_update;
  int      postupdate;
  int      deleted;
};

void rh_free_member(struct rahunas_member *member);
void rh_data_sync (int vserver_id, struct rahunas_member *member);

static const char *timemsg()
{
  static char tmsg[32] = "";
  char tfmt[] = "%b %e %T";
  time_t t;

  t = time(NULL);
  strftime(tmsg, sizeof tmsg, tfmt, localtime(&t));
  return tmsg; 
}

#endif // __RAHUNASD_H
