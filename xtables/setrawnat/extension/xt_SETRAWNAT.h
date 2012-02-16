#ifndef _LINUX_NETFILTER_XT_TARGET_SETRAWNAT
#define _LINUX_NETFILTER_XT_TARGET_SETRAWNAT 1

#include <linux/netfilter/ipset/ip_set.h>

struct xt_setrawnat_tginfo {
	char           setname[IPSET_MAXNAMELEN];
	struct ip_set *set;
	uint16_t       set_id;
};

struct xt_userspace_setrawnat_tginfo {
	char           setname[IPSET_MAXNAMELEN];
};


#endif /* _LINUX_NETFILTER_XT_TARGET_SETRAWNAT */
