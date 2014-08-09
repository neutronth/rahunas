#ifndef __IP_SET_RAHUNAS_H
#define __IP_SET_RAHUNAS_H

#include "ip_set.h"
#include "ip_set_bitmaps.h"

#ifdef __KERNEL__
#  include <linux/time.h>
#else
#  include <time.h>
#endif

#ifdef  SETTYPE_NAME
#undef  SETTYPE_NAME
#define SETTYPE_NAME "rahunas"
#endif

/* general flags */
#define IPSET_RAHUNAS_MATCHUNSET	1
#define IPSET_RAHUNAS_IGNOREMAC   2

/* per ip flags */
#define IPSET_RAHUNAS_ISSET	1

struct ip_set_rahunas {
	void *members;			/* the rahunas proper */
	ip_set_ip_t first_ip;		/* host byte order, included in range */
	ip_set_ip_t last_ip;		/* host byte order, included in range */
	u_int32_t flags;
  u_int32_t size;
};

struct ip_set_req_rahunas_create {
	ip_set_ip_t from;
	ip_set_ip_t to;
	u_int32_t flags;
};

struct ip_set_req_rahunas {
	ip_set_ip_t ip;
	unsigned char ethernet[ETH_ALEN];
	time_t timestamp;
};

struct ip_set_rahu {
	unsigned short match;
	unsigned char ethernet[ETH_ALEN];
	time_t timestamp;
};

#endif	/* __IP_SET_RAHUNAS_H */
