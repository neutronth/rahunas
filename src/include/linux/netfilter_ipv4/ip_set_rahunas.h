#ifndef __IP_SET_RAHUNAS_H
#define __IP_SET_RAHUNAS_H

#ifdef __KERNEL__
#  include <linux/time.h>
#else
#  include <time.h>
#endif

#include <linux/netfilter_ipv4/ip_set.h>

#define SETTYPE_NAME "rahunas"
#define MAX_RANGE 0x0000FFFF

/* general flags */
#define IPSET_RAHUNAS_MATCHUNSET	1

/* per ip flags */
#define IPSET_RAHUNAS_ISSET	1

struct ip_set_rahunasmap {
	void *members;			/* the rahunas proper */
	ip_set_ip_t first_ip;		/* host byte order, included in range */
	ip_set_ip_t last_ip;		/* host byte order, included in range */
	u_int32_t flags;
};

struct ip_set_req_rahunas_create {
	ip_set_ip_t from;
	ip_set_ip_t to;
	u_int32_t flags;
};

struct ip_set_req_rahunas {
	ip_set_ip_t ip;
	unsigned char ethernet[ETH_ALEN];
};

struct ip_set_rahunas {
	unsigned short flags;
	unsigned char ethernet[ETH_ALEN];
	time_t timestamp;
};

#endif	/* __IP_SET_RAHUNAS_H */
