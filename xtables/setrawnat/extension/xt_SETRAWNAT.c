/*
 *	"SETRAWNAT" target extension for Xtables - untracked NAT with ipset binding
 *	Copyright 2012, Neutron Soutmun <neo.neutron@gmail.com>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License; either
 *	version 2 of the License, or any later version, as published by the
 *	Free Software Foundation.
 *
 *	Derived from RAWNAT written by Jan Engelhardt
 */
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/version.h>
#include <linux/netfilter.h>
#include <linux/netfilter/nf_conntrack_common.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter_ipv6/ip6_tables.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include "compat_xtables.h"
#include <linux/netfilter/ipset/ip_set.h>
#include "xt_SETRAWNAT.h"

#if defined(CONFIG_IPV6) || defined(CONFIG_IPV6_MODULE)
#	define WITH_IPV6 1
#endif

/* IPSet hash:iplookup */
#define TYPE    hash_iplookup

/* The type variant functions: IPv4 */
struct hash_iplookup4_elem {
	__be32 ip;
	__be32 ip2;
};

static inline bool
hash_iplookup4_data_equal(const struct hash_iplookup4_elem *ip1,
                          const struct hash_iplookup4_elem *ip2,
                          u32 *multi)
{
	return ip1->ip == ip2->ip;
}

static inline void
hash_iplookup4_data_copy(struct hash_iplookup4_elem *dst,
                         const struct hash_iplookup4_elem *src)
{
	memcpy(dst, src, sizeof(*dst));
}

#define PF    4
#include "xt_SETRAWNAT_iplookup.h"

#undef PF

#ifdef WITH_IPV6
/* The type variant functions: IPv6 */
struct hash_iplookup6_elem {
	union nf_inet_addr ip;
	union nf_inet_addr ip2;
};

static inline bool
hash_iplookup6_data_equal(const struct hash_iplookup6_elem *ip1,
                          const struct hash_iplookup6_elem *ip2,
                          u32 *multi)
{
	return ipv6_addr_cmp(&ip1->ip.in6, &ip2->ip.in6) == 0;
}

static inline void
hash_iplookup6_data_copy(struct hash_iplookup6_elem *dst,
                         const struct hash_iplookup6_elem *src)
{
	memcpy(dst, src, sizeof(*dst));
}

#define PF    6
#include "xt_SETRAWNAT_iplookup.h"
#endif

static void rawnat4_update_l4(struct sk_buff *skb, __be32 oldip, __be32 newip)
{
	struct iphdr *iph = ip_hdr(skb);
	void *transport_hdr = (void *)iph + ip_hdrlen(skb);
	struct tcphdr *tcph;
	struct udphdr *udph;
	bool cond;

	switch (iph->protocol) {
	case IPPROTO_TCP:
		tcph = transport_hdr;
		inet_proto_csum_replace4(&tcph->check, skb, oldip, newip, true);
		break;
	case IPPROTO_UDP:
	case IPPROTO_UDPLITE:
		udph = transport_hdr;
		cond = udph->check != 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
		cond |= skb->ip_summed == CHECKSUM_PARTIAL;
#endif
		if (cond) {
			inet_proto_csum_replace4(&udph->check, skb,
				oldip, newip, true);
			if (udph->check == 0)
				udph->check = CSUM_MANGLED_0;
		}
		break;
	}
}

static unsigned int rawnat4_writable_part(const struct iphdr *iph)
{
	unsigned int wlen = sizeof(*iph);

	switch (iph->protocol) {
	case IPPROTO_TCP:
		wlen += sizeof(struct tcphdr);
		break;
	case IPPROTO_UDP:
		wlen += sizeof(struct udphdr);
		break;
	}
	return wlen;
}

static unsigned int
setrawsnat_tg4(struct sk_buff **pskb, const struct xt_action_param *par)
{
	const struct xt_setrawnat_tginfo *info = par->targinfo;
	struct iphdr *iph;
	struct hash_iplookup4_elem new;

	iph = ip_hdr(*pskb);

	new.ip = iph->saddr;
    if (!hash_iplookup4_test(info->set, &new))
		return XT_CONTINUE;

	if (!skb_make_writable(pskb, rawnat4_writable_part(iph)))
		return NF_DROP;

	iph = ip_hdr(*pskb);
	csum_replace4(&iph->check, iph->saddr, new.ip2);
	rawnat4_update_l4(*pskb, iph->saddr, new.ip2);
	iph->saddr = new.ip2;
	return XT_CONTINUE;
}

static unsigned int
setrawdnat_tg4(struct sk_buff **pskb, const struct xt_action_param *par)
{
	const struct xt_setrawnat_tginfo *info = par->targinfo;
	struct iphdr *iph;
	struct hash_iplookup4_elem new;

	iph = ip_hdr(*pskb);

	new.ip = iph->daddr;
    if (!hash_iplookup4_test(info->set, &new))
		return XT_CONTINUE;

	if (!skb_make_writable(pskb, rawnat4_writable_part(iph)))
		return NF_DROP;

	iph = ip_hdr(*pskb);
	csum_replace4(&iph->check, iph->daddr, new.ip2);
	rawnat4_update_l4(*pskb, iph->daddr, new.ip2);
	iph->daddr = new.ip2;
	return XT_CONTINUE;
}

#ifdef WITH_IPV6
static bool rawnat6_prepare_l4(struct sk_buff **pskb, unsigned int *l4offset,
    unsigned int *l4proto)
{
	static const unsigned int types[] =
		{IPPROTO_TCP, IPPROTO_UDP, IPPROTO_UDPLITE};
	unsigned int i;
	int err;

	*l4proto = NEXTHDR_MAX;

	for (i = 0; i < ARRAY_SIZE(types); ++i) {
		err = ipv6_find_hdr(*pskb, l4offset, types[i], NULL);
		if (err >= 0) {
			*l4proto = types[i];
			break;
		}
		if (err != -ENOENT)
			return false;
	}

	switch (*l4proto) {
	case IPPROTO_TCP:
		if (!skb_make_writable(pskb, *l4offset + sizeof(struct tcphdr)))
			return false;
		break;
	case IPPROTO_UDP:
	case IPPROTO_UDPLITE:
		if (!skb_make_writable(pskb, *l4offset + sizeof(struct udphdr)))
			return false;
		break;
	}

	return true;
}

static void rawnat6_update_l4(struct sk_buff *skb, unsigned int l4proto,
    unsigned int l4offset, const struct in6_addr *oldip,
    const struct in6_addr *newip)
{
	const struct ipv6hdr *iph = ipv6_hdr(skb);
	struct tcphdr *tcph;
	struct udphdr *udph;
	unsigned int i;
	bool cond;

	switch (l4proto) {
	case IPPROTO_TCP:
		tcph = (void *)iph + l4offset;
		for (i = 0; i < 4; ++i)
			inet_proto_csum_replace4(&tcph->check, skb,
				oldip->s6_addr32[i], newip->s6_addr32[i], true);
		break;
	case IPPROTO_UDP:
	case IPPROTO_UDPLITE:
		udph = (void *)iph + l4offset;
		cond = udph->check;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
		cond |= skb->ip_summed == CHECKSUM_PARTIAL;
#endif
		if (cond) {
			for (i = 0; i < 4; ++i)
				inet_proto_csum_replace4(&udph->check, skb,
					oldip->s6_addr32[i],
					newip->s6_addr32[i], true);
			if (udph->check == 0)
				udph->check = CSUM_MANGLED_0;
		}
		break;
	}
}

static unsigned int
setrawsnat_tg6(struct sk_buff **pskb, const struct xt_action_param *par)
{
	const struct xt_setrawnat_tginfo *info = par->targinfo;
	unsigned int l4offset, l4proto;
	struct ipv6hdr *iph;
	struct hash_iplookup6_elem new;

	iph = ipv6_hdr(*pskb);

	memcpy(&new.ip, &iph->saddr, sizeof(struct in6_addr));
	if (!hash_iplookup6_test(info->set, &new))
		return XT_CONTINUE;

	if (!rawnat6_prepare_l4(pskb, &l4offset, &l4proto))
		return NF_DROP;

	iph = ipv6_hdr(*pskb);
	rawnat6_update_l4(*pskb, l4proto, l4offset, &iph->saddr,
	                  (const struct in6_addr *) &new.ip2);
	memcpy(&iph->saddr, &new.ip2, sizeof(struct in6_addr));
	return XT_CONTINUE;
}

static unsigned int
setrawdnat_tg6(struct sk_buff **pskb, const struct xt_action_param *par)
{
	const struct xt_setrawnat_tginfo *info = par->targinfo;
	unsigned int l4offset, l4proto;
	struct ipv6hdr *iph;
	struct hash_iplookup6_elem new;

	iph = ipv6_hdr(*pskb);

	memcpy(&new.ip, &iph->daddr, sizeof(struct in6_addr));
	if (!hash_iplookup6_test(info->set, &new))
		return XT_CONTINUE;

	if (!rawnat6_prepare_l4(pskb, &l4offset, &l4proto))
		return NF_DROP;

	iph = ipv6_hdr(*pskb);
	rawnat6_update_l4(*pskb, l4proto, l4offset, &iph->daddr,
	                  (const struct in6_addr *) &new.ip2);
	memcpy(&iph->daddr, &new.ip2, sizeof(struct in6_addr));
	return XT_CONTINUE;
}
#endif

static int setrawnat_tg_check(const struct xt_tgchk_param *par)
{
	struct xt_setrawnat_tginfo *info = par->targinfo;

	if (strcmp(par->table, "raw") == 0 ||
	    strcmp(par->table, "rawpost") == 0)
		goto chkset;

	printk(KERN_ERR KBUILD_MODNAME " may only be used in the \"raw\" or "
	       "\"rawpost\" table.\n");
	return -EINVAL;

chkset:
	info->set = NULL;
	info->set_id = ip_set_get_byname(info->setname, &info->set);

	if (!info->set) {
		printk(KERN_ERR KBUILD_MODNAME " hash:iplookup set name '%s' "
		       "not found\n", info->setname);
		return -EINVAL;
	}

	if (strcmp("hash:iplookup", info->set->type->name) != 0) {
		printk(KERN_ERR KBUILD_MODNAME "set name '%s' is not "
		       "hash:iplookup type", info->setname);
		return -EINVAL;
	}

	return 0;
}

static void setrawnat_tg_destroy(const struct xt_tgdtor_param *par)
{
	const struct xt_setrawnat_tginfo *info = par->targinfo;

	ip_set_put_byindex(info->set_id);
}

static struct xt_target setrawnat_tg_reg[] __read_mostly = {
	{
		.name       = "SETRAWSNAT",
		.revision   = 0,
		.family     = NFPROTO_IPV4,
		.target     = setrawsnat_tg4,
		.targetsize = sizeof(struct xt_setrawnat_tginfo),
		.checkentry = setrawnat_tg_check,
		.destroy    = setrawnat_tg_destroy,
		.me         = THIS_MODULE,
	},
#ifdef WITH_IPV6
	{
		.name       = "SETRAWSNAT",
		.revision   = 0,
		.family     = NFPROTO_IPV6,
		.target     = setrawsnat_tg6,
		.targetsize = sizeof(struct xt_setrawnat_tginfo),
		.checkentry = setrawnat_tg_check,
		.destroy    = setrawnat_tg_destroy,
		.me         = THIS_MODULE,
	},
#endif
	{
		.name       = "SETRAWDNAT",
		.revision   = 0,
		.family     = NFPROTO_IPV4,
		.target     = setrawdnat_tg4,
		.targetsize = sizeof(struct xt_setrawnat_tginfo),
		.checkentry = setrawnat_tg_check,
		.destroy    = setrawnat_tg_destroy,
		.me         = THIS_MODULE,
	},
#ifdef WITH_IPV6
	{
		.name       = "SETRAWDNAT",
		.revision   = 0,
		.family     = NFPROTO_IPV6,
		.target     = setrawdnat_tg6,
		.targetsize = sizeof(struct xt_setrawnat_tginfo),
		.checkentry = setrawnat_tg_check,
		.destroy    = setrawnat_tg_destroy,
		.me         = THIS_MODULE,
	},
#endif
};

static int __init setrawnat_tg_init(void)
{
	return xt_register_targets(setrawnat_tg_reg, ARRAY_SIZE(setrawnat_tg_reg));
}

static void __exit setrawnat_tg_exit(void)
{
	xt_unregister_targets(setrawnat_tg_reg, ARRAY_SIZE(setrawnat_tg_reg));
}

module_init(setrawnat_tg_init);
module_exit(setrawnat_tg_exit);
MODULE_AUTHOR("Neutron Soutmun <neo.neutron@gmail.com>");
MODULE_DESCRIPTION("Xtables: conntrack-less raw NAT with ipset binding");
MODULE_LICENSE("GPL");
MODULE_ALIAS("ipt_SETRAWSNAT");
MODULE_ALIAS("ipt_SETRAWDNAT");
MODULE_ALIAS("ip6t_SETRAWSNAT");
MODULE_ALIAS("ip6t_SETRAWDNAT");
