/* Copyright (C) 2003-2013 Jozsef Kadlecsik <kadlec@blackhole.kfki.hu>
 *               2014      Neutron Soutmun <neutron@rahunas.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* Kernel module implementing an IP set type: the rahunas type
 * based on the hash:ip written by Jozsef Kadlecsik <kadlec@blackhole.kfki.hu>
*/

#include <linux/jhash.h>
#include <linux/module.h>
#include <linux/ip.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/errno.h>
#include <linux/random.h>
#include <linux/workqueue.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/rculist.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include <net/netlink.h>
#include <net/genetlink.h>
#include <net/tcp.h>

#include <linux/netfilter.h>
#include <linux/netfilter/ipset/pfxlen.h>
#include <linux/netfilter/ipset/ip_set.h>
#include <linux/netfilter/ipset/ip_set_hash.h>

#define NO_ETHER_ADDR(e) (e[0] == 0 && e[1] == 0 && e[2] == 0 && \
                          e[3] == 0 && e[4] == 0 && e[5] == 0)

/* RahuNAS netlink */
#define RHNL_NAME  "rahunas_event"
#define RHNL_VER   1

#define IPSET_ATTR_TIMESTAMP  IPSET_ATTR_TIMEOUT

#define RHNL_RETRY_PERIOD     20

/* commands */
enum {
  RHNL_CMD_UNSPEC,
  RHNL_CMD_EVENT,
  __RHNL_CMD_MAX,
};

#define RHNL_CMD_MAX (__RHNL_CMD_MAX - 1)

static struct genl_ops rhnl_ops[] = {
};

/* family definition */
static struct genl_family rhnl_family = {
  .id      = GENL_ID_GENERATE,
  .hdrsize = 0,
  .name    = RHNL_NAME,
  .version = RHNL_VER,
  .maxattr = IPSET_ATTR_CMD_MAX,
};

/* multicast group */
static struct genl_multicast_group rhnl_mcgrps[] = {
  { .name = "notify" },
};

static int
rhnl_register(void)
{
  int rc = 0;

  rc = genl_register_family_with_ops_groups(&rhnl_family, rhnl_ops,
                                            rhnl_mcgrps);
  if (rc != 0)
    return rc;

  return 0;
}

static void
rhnl_unregister(void)
{
  genl_unregister_family(&rhnl_family);
}

static int
rh_timeout_notify_msg(struct sk_buff **msg, const char *setname,
                      const struct sockaddr *sa, const unsigned char *ether)
{
  struct sk_buff *skb;
  void *hdr;
  struct nlattr *nested;
  struct timespec ts;
  int rc;

  skb = nlmsg_new(GENLMSG_DEFAULT_SIZE, GFP_KERNEL);
  if (!skb)
    return -ENOMEM;

  hdr = genlmsg_put(skb, 0, 0, &rhnl_family, 0, RHNL_CMD_EVENT);
  if (!hdr) {
    nlmsg_free(skb);
    return -ENOMEM;
  }

  rc = nla_put_u8(skb, IPSET_ATTR_PROTOCOL, RHNL_VER);
  if (rc != 0)
    goto nla_put_failure;

  rc = nla_put_string(skb, IPSET_ATTR_SETNAME, setname);
  if (rc != 0)
    goto nla_put_failure;

  nested = ipset_nest_start(skb, IPSET_ATTR_DATA);
  if (!nested)
    goto nla_put_failure;

  rc = nla_put_u8(skb, IPSET_ATTR_PROTO, sa->sa_family);
  if (rc != 0)
    goto nla_put_failure;

  switch (sa->sa_family) {
    case NFPROTO_IPV4: {
      struct sockaddr_in *in4 = (struct sockaddr_in *)sa;
      rc = nla_put_ipaddr4(skb, IPSET_ATTR_IP, in4->sin_addr.s_addr);
      if (rc != 0)
        goto nla_put_failure;
      break;
    }
    case NFPROTO_IPV6: {
      struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)sa;
      rc = nla_put_ipaddr6(skb, IPSET_ATTR_IP, &in6->sin6_addr);
      if (rc != 0)
        goto nla_put_failure;
      break;
    }
  }

  if (!NO_ETHER_ADDR(ether))
    if (nla_put(skb, IPSET_ATTR_ETHER, ETH_ALEN, ether))
      goto nla_put_failure;

  getnstimeofday(&ts);
  if (nla_put_u32(skb, IPSET_ATTR_TIMESTAMP, ts.tv_sec))
    goto nla_put_failure;

  ipset_nest_end(skb, nested);

  rc = genlmsg_end(skb, hdr);
  if (rc < 0)
    goto nla_put_failure;

  *msg = skb;
  return 0;

nla_put_failure:
  nlmsg_free(skb);
  return -EINVAL;
}

static int
rh_send_notify_msg(struct sk_buff *msg)
{
  int ret = genlmsg_multicast(&rhnl_family, msg, 0, 0, GFP_ATOMIC);
  pr_debug("[%s] genlmsg_multicast(),ret=%d\n", RHNL_NAME, ret);

  return ret;
}

/* rahunas workqueue */
#define RH_WORKQUEUE_NAME  "rahunas_wq"

static struct workqueue_struct *rh_wq = NULL;
static atomic_t rhwork_stop = ATOMIC_INIT(0);

struct rhwork {
  struct delayed_work dwork;
  struct list_head    list;
  struct sk_buff*     msg;
};

struct rhwork rhwork_list;

static inline void
rhwork_free_entry(struct rhwork *work)
{
  pr_debug("[%s] rhwork_free_entry(%p)\n", RH_WORKQUEUE_NAME, work);
  kfree_skb(work->msg);
  kfree(work);
}

static inline int
rhwork_list_add_entry(struct rhwork *work, struct rhwork *head)
{
  list_add_tail_rcu(&work->list, &head->list);
  return 0;
}

static inline int
rhwork_list_del_entry(struct rhwork *work, struct rhwork *head)
{
  struct rhwork *e;
  list_for_each_entry(e, &head->list, list) {
    if (&work->dwork == &e->dwork) {
      list_del_rcu(&e->list);
      rhwork_free_entry(e);
      return 0;
    }
  }

  return -EFAULT;
}

static inline void
flush_rhwork(struct rhwork *head)
{
  struct rhwork *e;
  list_for_each_entry(e, &head->list, list) {
    pr_debug("[%s] rhwork flushing: %p\n", RH_WORKQUEUE_NAME, e);
    cancel_delayed_work_sync(&e->dwork);
    list_del_rcu(&e->list);
    rhwork_free_entry(e);
  }
}

static int
rhwq_init(void)
{
  rh_wq = create_workqueue(RH_WORKQUEUE_NAME);
  if (!rh_wq)
    return -1;

  INIT_LIST_HEAD_RCU(&rhwork_list.list);

  return 0;
}

static void
rhwq_finish(void)
{
  atomic_set(&rhwork_stop, 1);

  flush_rhwork(&rhwork_list);
  flush_workqueue(rh_wq);
  destroy_workqueue(rh_wq);
}

static void
rh_notify_cb(struct work_struct *work)
{
  struct delayed_work *dwork = to_delayed_work (work);
  struct rhwork *rhwork = container_of(dwork, struct rhwork, dwork);
  struct sk_buff *msg = NULL;

  if (atomic_read(&rhwork_stop)) {
    return;
  }

  msg = skb_get(rhwork->msg);

  if (rh_send_notify_msg(msg) == 0) {
    rhwork_list_del_entry(rhwork, &rhwork_list);
  } else {
    /* Defer work with delay */
    int ret = 0;
    ret = queue_delayed_work(rh_wq, &rhwork->dwork,
                             msecs_to_jiffies(RHNL_RETRY_PERIOD * 1000));
    pr_debug ("[%s] Delay queue needed, queue: ret=%d\n", RH_WORKQUEUE_NAME,
               ret);
  }
}

static int
rh_queue_notify_msg(const char *setname, const struct sockaddr *sa,
                    const unsigned char *ether)
{
  struct rhwork *rhwork = NULL;

  if (atomic_read(&rhwork_stop))
    return -EBUSY;

  rhwork = (struct rhwork *)kmalloc(sizeof(struct rhwork), GFP_ATOMIC);
  if (!rhwork) {
    return -ENOMEM;
  }

  INIT_DELAYED_WORK(&rhwork->dwork, rh_notify_cb);

  if (rh_timeout_notify_msg(&rhwork->msg, setname, sa, ether) != 0) {
    kfree(rhwork);
    return -1;
  }

  pr_debug("[%s] Queue notify: %s, %pIS, %s, %02x:%02x:%02x:%02x:%02x:%02x\n",
           RH_WORKQUEUE_NAME, setname, sa,
           sa->sa_family == NFPROTO_IPV4 ? "IPv4" : "IPv6",
           ether[0], ether[1], ether[2], ether[3], ether[4], ether[5]);

  if (queue_delayed_work(rh_wq, &rhwork->dwork, 0)) {
    rhwork_list_add_entry(rhwork, &rhwork_list);
    return 0;
  } else {
    kfree(rhwork);
    return -EFAULT;
  }
}

#define IPSET_TYPE_REV_MIN  0
/*        1    Counters support */
/*        2    Comments support */
#define IPSET_TYPE_REV_MAX  3 /* Forceadd support */

#define IPSET_FLAG_IGNOREMAC      IPSET_FLAG_NOMATCH

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Neutron Soutmun <neutron@rahunas.org>");
IP_SET_MODULE_DESC("rahunas", IPSET_TYPE_REV_MIN, IPSET_TYPE_REV_MAX);
MODULE_ALIAS("ip_set_rahunas");

/* Type specific function prefix */
#define HTYPE   rahunas
#undef  IPSET_GC_PERIOD
#define IPSET_GC_PERIOD(a) (10)  /* rahunas override */

/* IPv4 variant */

/* Member elements */
struct rahunas4_elem {
  /* Zero valued IP addresses cannot be stored */
  __be32 ip;
  unsigned char ether[ETH_ALEN];
  u32 timeout;
  u8 ignoremac;
};

/* Common functions */

static inline bool
rahunas4_data_equal(const struct rahunas4_elem *e1,
                    const struct rahunas4_elem *e2,
                    u32 *multi)
{
  if (e1->ip != e2->ip)
    return 0;

  return e1->ignoremac || NO_ETHER_ADDR(e2->ether) ||
           ether_addr_equal(e1->ether, e2->ether);
}

static inline bool
rahunas4_data_list(struct sk_buff *skb, const struct rahunas4_elem *e)
{
  u32 flags = e->ignoremac ? IPSET_FLAG_IGNOREMAC : 0;

  if (nla_put_ipaddr4(skb, IPSET_ATTR_IP, e->ip))
    goto nla_put_failure;

  if (!NO_ETHER_ADDR(e->ether))
    if (nla_put(skb, IPSET_ATTR_ETHER, ETH_ALEN, e->ether))
      goto nla_put_failure;

  if (flags)
    if (nla_put_net32 (skb, IPSET_ATTR_CADT_FLAGS, htonl(flags)))
      goto nla_put_failure;

  return 0;

nla_put_failure:
  return 1;
}

static inline void
rahunas4_data_next(struct rahunas4_elem *next, const struct rahunas4_elem *e)
{
  next->ip = e->ip;
}

static int
rahunas4_timeout_notify(const char *setname, struct rahunas4_elem *e)
{
  struct sockaddr_in sin;

  sin.sin_addr.s_addr = e->ip;
  sin.sin_family = NFPROTO_IPV4;
  return rh_queue_notify_msg (setname, (struct sockaddr *)&sin, e->ether);
}

#define MTYPE         rahunas4
#define MTYPE_NAME    "rahunas4"
#define PF            4
#define HOST_MASK     32
#undef  HKEY_DATALEN
#define HKEY_DATALEN  4

#include "ip_set_rahunas_gen.h"

static int
rahunas4_kadt(struct ip_set *set, const struct sk_buff *skb,
              const struct xt_action_param *par,
              enum ipset_adt adt, struct ip_set_adt_opt *opt)
{
  ipset_adtfn adtfn = set->variant->adt[adt];
  struct rahunas4_elem e = {};
  struct ip_set_ext ext = IP_SET_INIT_KEXT(skb, opt, set);
  __be32 ip;

  ip4addrptr(skb, opt->flags & IPSET_DIM_ONE_SRC, &ip);
  if (ip == 0)
    return -EINVAL;

  e.ip = ip;
  if (opt->flags & IPSET_DIM_TWO_SRC) {
    if (skb_mac_header(skb) < skb->head ||
          (skb_mac_header(skb) + ETH_ALEN) > skb->data)
      return -EINVAL;

    memcpy (e.ether, eth_hdr(skb)->h_source, ETH_ALEN);

    if (NO_ETHER_ADDR(e.ether))
      return -EINVAL;
  } else {
    memset (e.ether, 0, ETH_ALEN);
  }

  return adtfn(set, &e, &ext, &opt->ext, opt->cmdflags);
}

static int
rahunas4_uadt(struct ip_set *set, struct nlattr *tb[],
              enum ipset_adt adt, u32 *lineno, u32 flags, bool retried)
{
  struct rahunas *h = set->data;
  ipset_adtfn adtfn = set->variant->adt[adt];
  struct rahunas4_elem e = {};
  struct ip_set_ext ext  = IP_SET_INIT_UEXT(set);
  u32 ip  = 0;
  int ret = 0;

  if (unlikely(!tb[IPSET_ATTR_IP] ||
         !ip_set_optattr_netorder(tb, IPSET_ATTR_CADT_FLAGS) ||
         !ip_set_optattr_netorder(tb, IPSET_ATTR_TIMEOUT) ||
         !ip_set_optattr_netorder(tb, IPSET_ATTR_PACKETS) ||
         !ip_set_optattr_netorder(tb, IPSET_ATTR_BYTES)))
    return -IPSET_ERR_PROTOCOL;

  if (tb[IPSET_ATTR_LINENO])
    *lineno = nla_get_u32(tb[IPSET_ATTR_LINENO]);

  ret = ip_set_get_hostipaddr4(tb[IPSET_ATTR_IP], &ip) ||
        ip_set_get_extensions(set, tb, &ext);
  if (ret)
    return ret;

  if (adt == IPSET_TEST) {
    e.ip = htonl(ip);
    if (e.ip == 0)
      return -IPSET_ERR_HASH_ELEM;

    if (tb[IPSET_ATTR_ETHER]) {
      memcpy (e.ether, nla_data(tb[IPSET_ATTR_ETHER]), ETH_ALEN);

      if (NO_ETHER_ADDR(e.ether))
        return -IPSET_ERR_HASH_ELEM;
    } else {
      memset (e.ether, 0, ETH_ALEN);
    }

    return adtfn(set, &e, &ext, &ext, flags);
  }

  if (retried)
    ip = ntohl(h->next.ip);

  e.ip = htonl(ip);
  if (e.ip == 0)
    return -IPSET_ERR_HASH_ELEM;

  if (tb[IPSET_ATTR_ETHER])
    memcpy (e.ether, nla_data(tb[IPSET_ATTR_ETHER]), ETH_ALEN);
  else
    memset (e.ether, 0, ETH_ALEN);

  if (tb[IPSET_ATTR_CADT_FLAGS]) {
    u32 cadt_flags = ip_set_get_h32(tb[IPSET_ATTR_CADT_FLAGS]);
    if (cadt_flags & IPSET_FLAG_IGNOREMAC)
      e.ignoremac = 1;
  }

  if (tb[IPSET_ATTR_TIMEOUT])
    e.timeout = ip_set_timeout_uget(tb[IPSET_ATTR_TIMEOUT]);
  else
    e.timeout = 0;

  ret = adtfn(set, &e, &ext, &ext, flags);

  if (ret && !ip_set_eexist(ret, flags))
    return ret;

  return 0;
}

/* IPv6 variant */

/* Member elements */
struct rahunas6_elem {
  union nf_inet_addr ip;
  unsigned char ether[ETH_ALEN];
  u32 timeout;
  u8 ignoremac;
};

/* Common functions */

static inline bool
rahunas6_data_equal(const struct rahunas6_elem *ip1,
                    const struct rahunas6_elem *ip2, u32 *multi)
{
  if (!ipv6_addr_equal(&ip1->ip.in6, &ip2->ip.in6))
    return 0;

  return ip1->ignoremac || NO_ETHER_ADDR(ip2->ether) ||
         ether_addr_equal(ip1->ether, ip2->ether);
}

static bool
rahunas6_data_list(struct sk_buff *skb, const struct rahunas6_elem *e)
{
  u32 flags = e->ignoremac ? IPSET_FLAG_IGNOREMAC : 0;

  if (nla_put_ipaddr6(skb, IPSET_ATTR_IP, &e->ip.in6))
    goto nla_put_failure;

  if (!NO_ETHER_ADDR(e->ether))
    if (nla_put(skb, IPSET_ATTR_ETHER, ETH_ALEN, e->ether))
      goto nla_put_failure;

  if (flags)
    if (nla_put_net32 (skb, IPSET_ATTR_CADT_FLAGS, htonl(flags)))
      goto nla_put_failure;

  return 0;

nla_put_failure:
  return 1;
}

static inline void
rahunas6_data_next(struct rahunas4_elem *next, const struct rahunas6_elem *e)
{
}

static int
rahunas6_timeout_notify(const char *setname, struct rahunas6_elem *e)
{
  struct sockaddr_in6 sin6;

  memcpy(sin6.sin6_addr.s6_addr, &e->ip.in6, sizeof (sin6.sin6_addr.s6_addr));
  sin6.sin6_family = NFPROTO_IPV6;
  return rh_queue_notify_msg (setname, (struct sockaddr *)&sin6, e->ether);
}

#undef MTYPE
#undef MTYPE_NAME
#undef PF
#undef HOST_MASK
#undef HKEY_DATALEN

#define MTYPE         rahunas6
#define MTYPE_NAME    "rahunas6"
#define PF            6
#define HOST_MASK     128
#undef  HKEY_DATALEN
#define HKEY_DATALEN  16

#define IP_SET_EMIT_CREATE

#include "ip_set_rahunas_gen.h"

static int
rahunas6_kadt(struct ip_set *set, const struct sk_buff *skb,
              const struct xt_action_param *par,
              enum ipset_adt adt, struct ip_set_adt_opt *opt)
{
  ipset_adtfn adtfn = set->variant->adt[adt];
  struct rahunas6_elem e = {};
  struct ip_set_ext ext = IP_SET_INIT_KEXT(skb, opt, set);

  ip6addrptr(skb, opt->flags & IPSET_DIM_ONE_SRC, &e.ip.in6);
  if (ipv6_addr_any(&e.ip.in6))
    return -EINVAL;

  if (opt->flags & IPSET_DIM_TWO_SRC) {
    if (skb_mac_header(skb) < skb->head ||
          (skb_mac_header(skb) + ETH_ALEN) > skb->data)
      return -EINVAL;

    memcpy (e.ether, eth_hdr(skb)->h_source, ETH_ALEN);

    if (NO_ETHER_ADDR(e.ether))
      return -EINVAL;
  } else {
    memset (e.ether, 0, ETH_ALEN);
  }

  return adtfn(set, &e, &ext, &opt->ext, opt->cmdflags);
}

static int
rahunas6_uadt(struct ip_set *set, struct nlattr *tb[],
              enum ipset_adt adt, u32 *lineno, u32 flags, bool retried)
{
  ipset_adtfn adtfn = set->variant->adt[adt];
  struct rahunas6_elem e = {};
  struct ip_set_ext ext = IP_SET_INIT_UEXT(set);
  int ret;

  if (unlikely(!tb[IPSET_ATTR_IP] ||
         !ip_set_optattr_netorder(tb, IPSET_ATTR_CADT_FLAGS) ||
         !ip_set_optattr_netorder(tb, IPSET_ATTR_TIMEOUT) ||
         !ip_set_optattr_netorder(tb, IPSET_ATTR_PACKETS) ||
         !ip_set_optattr_netorder(tb, IPSET_ATTR_BYTES)))
    return -IPSET_ERR_PROTOCOL;

  if (tb[IPSET_ATTR_LINENO])
    *lineno = nla_get_u32(tb[IPSET_ATTR_LINENO]);

  ret = ip_set_get_ipaddr6(tb[IPSET_ATTR_IP], &e.ip) ||
        ip_set_get_extensions(set, tb, &ext);
  if (ret)
    return ret;

  if (ipv6_addr_any(&e.ip.in6))
    return -IPSET_ERR_HASH_ELEM;

  if (tb[IPSET_ATTR_ETHER])
    memcpy (e.ether, nla_data(tb[IPSET_ATTR_ETHER]), ETH_ALEN);
  else
    memset (e.ether, 0, ETH_ALEN);

  if (tb[IPSET_ATTR_CADT_FLAGS]) {
    u32 cadt_flags = ip_set_get_h32(tb[IPSET_ATTR_CADT_FLAGS]);
    if (cadt_flags & IPSET_FLAG_IGNOREMAC)
      e.ignoremac = 1;
  }

  if (tb[IPSET_ATTR_TIMEOUT])
    e.timeout = ip_set_timeout_uget(tb[IPSET_ATTR_TIMEOUT]);
  else
    e.timeout = 0;

  ret = adtfn(set, &e, &ext, &ext, flags);

  return ip_set_eexist(ret, flags) ? 0 : ret;
}

static struct ip_set_type rahunas_type __read_mostly = {
  .name          = "rahunas",
  .protocol      = IPSET_PROTOCOL,
  .features      = IPSET_TYPE_IP | IPSET_TYPE_MAC,
  .dimension     = IPSET_DIM_TWO,
  .family        = NFPROTO_UNSPEC,
  .revision_min  = IPSET_TYPE_REV_MIN,
  .revision_max  = IPSET_TYPE_REV_MAX,
  .create        = rahunas_create,
  .create_policy = {
    [IPSET_ATTR_HASHSIZE]   = { .type = NLA_U32 },
    [IPSET_ATTR_MAXELEM]    = { .type = NLA_U32 },
    [IPSET_ATTR_PROBES]     = { .type = NLA_U8 },
    [IPSET_ATTR_RESIZE]     = { .type = NLA_U8  },
    [IPSET_ATTR_TIMEOUT]    = { .type = NLA_U32 },
    [IPSET_ATTR_CADT_FLAGS] = { .type = NLA_U32 },
  },
  .adt_policy = {
    [IPSET_ATTR_IP]      = { .type = NLA_NESTED },
    [IPSET_ATTR_TIMEOUT] = { .type = NLA_U32 },
    [IPSET_ATTR_LINENO]  = { .type = NLA_U32 },
    [IPSET_ATTR_BYTES]   = { .type = NLA_U64 },
    [IPSET_ATTR_PACKETS] = { .type = NLA_U64 },
    [IPSET_ATTR_COMMENT] = { .type = NLA_NUL_STRING },
  },
  .me = THIS_MODULE,
};

static int __init
rahunas_init(void)
{
  int rc = 0;

  rc = rhnl_register();
  if (rc)
    return rc;

  rc = rhwq_init();
  if (rc) {
    rhnl_unregister();
    return rc;
  }

  return ip_set_type_register(&rahunas_type);
}

static void __exit
rahunas_fini(void)
{
  rhnl_unregister();
  rhwq_finish();
  ip_set_type_unregister(&rahunas_type);
}

module_init(rahunas_init);
module_exit(rahunas_fini);
