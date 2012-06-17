#ifndef __IP_SET_RAHUNAS_IPIPHASH_H
#define __IP_SET_RAHUNAS_IPIPHASH_H

#include <asm/types.h>

#include "ip_set.h"
#include "ip_set_hashes.h"

#ifdef  SETTYPE_NAME
#undef  SETTYPE_NAME
#define SETTYPE_NAME		"rahunas_ipiphash"
#endif

struct rahunas_ipip {
  ip_set_ip_t ip;
  ip_set_ip_t ip1;
};

struct ip_set_rahunas_ipiphash {
	struct rahunas_ipip *members;	/* the ipiphash proper */
	uint32_t elements;		/* number of elements */
	uint32_t hashsize;		/* hash size */
	uint16_t probes;		/* max number of probes  */
	uint16_t resize;		/* resize factor in percent */
	initval_t initval[0];		/* initvals for jhash_1word */
};

struct ip_set_req_rahunas_ipiphash_create {
	uint32_t hashsize;
	uint16_t probes;
	uint16_t resize;
};

struct ip_set_req_rahunas_ipiphash {
	ip_set_ip_t ip;
  ip_set_ip_t ip1;
};

#endif	/* __IP_SET_RAHUNAS_IPIPHASH_H */
