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
#include <time.h>
#include <linux/if_ether.h>
#include <errno.h>

#include "rh-config.h"
#include "rh-utils.h"

#define PROGRAM "RahuNASd"
#define VERSION "0.1.1"
#define MAX_MEMBERS 0x00FFFF

extern struct rahunas_map *map;
extern struct set *rahunas_set;
extern const char *termstring; 

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

struct rahunas_map {
  struct rahunas_member *members;
	in_addr_t first_ip;
	in_addr_t last_ip;
	unsigned int size;
};

struct rahunas_member {
  unsigned short flags; 
  unsigned short expired;
	time_t session_start;
	char *username;
  char *session_id;
  unsigned char mac_address[ETH_ALEN];
};

uint32_t iptoid(struct rahunas_map *map, const char *ip);
char *idtoip(struct rahunas_map *map, uint32_t id);

void rh_free_member (struct rahunas_member *member);

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
