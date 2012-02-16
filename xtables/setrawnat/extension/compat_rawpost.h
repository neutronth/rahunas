#ifndef XTA_COMPAT_RAWPOST_H
#define XTA_COMPAT_RAWPOST_H 1

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
typedef struct sk_buff sk_buff_t;
#else
typedef struct sk_buff *sk_buff_t;
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 21)
#define XT_TARGET_INIT(__name, __size)					       \
{									       \
	.target.u.user = {						       \
		.target_size	= XT_ALIGN(__size),			       \
		.name		= __name,				       \
	},								       \
}

#define IPT_ENTRY_INIT(__size)						       \
{									       \
	.target_offset	= sizeof(struct ipt_entry),			       \
	.next_offset	= (__size),					       \
}

#define IPT_STANDARD_INIT(__verdict)					       \
{									       \
	.entry		= IPT_ENTRY_INIT(sizeof(struct ipt_standard)),	       \
	.target		= XT_TARGET_INIT(IPT_STANDARD_TARGET,		       \
					 sizeof(struct xt_standard_target)),   \
	.target.verdict	= -(__verdict) - 1,				       \
}

#define IPT_ERROR_INIT							       \
{									       \
	.entry		= IPT_ENTRY_INIT(sizeof(struct ipt_error)),	       \
	.target		= XT_TARGET_INIT(IPT_ERROR_TARGET,		       \
					 sizeof(struct ipt_error_target)),     \
	.target.errorname = "ERROR",					       \
}

#define IP6T_ENTRY_INIT(__size)						       \
{									       \
	.target_offset	= sizeof(struct ip6t_entry),			       \
	.next_offset	= (__size),					       \
}

#define IP6T_STANDARD_INIT(__verdict)					       \
{									       \
	.entry		= IP6T_ENTRY_INIT(sizeof(struct ip6t_standard)),       \
	.target		= XT_TARGET_INIT(IP6T_STANDARD_TARGET,		       \
					 sizeof(struct ip6t_standard_target)), \
	.target.verdict	= -(__verdict) - 1,				       \
}

#define IP6T_ERROR_INIT							       \
{									       \
	.entry		= IP6T_ENTRY_INIT(sizeof(struct ip6t_error)),	       \
	.target		= XT_TARGET_INIT(IP6T_ERROR_TARGET,		       \
					 sizeof(struct ip6t_error_target)),    \
	.target.errorname = "ERROR",					       \
}

#endif /* 2.6.21 */

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 20)
#	include <linux/netfilter_ipv6/ip6_tables.h>
/* Standard entry */
struct ip6t_standard
{
	struct ip6t_entry entry;
	struct ip6t_standard_target target;
};

struct ip6t_error_target
{
	struct ip6t_entry_target target;
	char errorname[IP6T_FUNCTION_MAXNAMELEN];
};

struct ip6t_error
{
	struct ip6t_entry entry;
	struct ip6t_error_target target;
};
#endif /* 2.6.20 */

#endif /* XTA_COMPAT_RAWPOST_H */
