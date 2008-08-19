/**
 * RahuNASd header 
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-08-07
 */

#ifndef __RAHUNASD_H
#define __RAHUNASD_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define PROGRAM "RuhuNASd"
#define VERSION "0.1.1"
#define MAX_MEMBERS 0x00FFFF

/* Configuration */
#define DEFAULT_LOG "/var/log/rahunas/rahunas.log"
#define IDLE_THRESHOLD 30
#define POLLING 30 
#define SET_NAME "rahunas_set"
#define XMLSERVICE_HOST	"localhost"
#define XMLSERVICE_PORT	8888
#define XMLSERVICE_URL	"/xmlrpc_service.php"


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
};

uint32_t iptoid(struct rahunas_map *map, const char *ip);
char *idtoip(struct rahunas_map *map, uint32_t id);

void *rh_malloc(size_t size);
void rh_free(void **data);

static char *timemsg()
{
  static char tmsg[32] = "";
  char tfmt[] = "%b %e %T";
  time_t t;

  t = time(NULL);
  strftime(tmsg, sizeof tmsg, tfmt, localtime(&t));
  return tmsg; 
}

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
#define DP(priority, format, args...)
#endif

#endif // __RAHUNASD_H
