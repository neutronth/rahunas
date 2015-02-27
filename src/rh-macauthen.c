/**
 * RahuNAS MAC address authentication
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2014-12-02
 */

#include <poll.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>

#include "rahunasd.h"
#include "rh-macauthen.h"
#include "jhash.h"
#include "rh-xmlrpc-cmd.h"

pthread_mutex_t RHMACAuthenMtxLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t RHMACAuthenDataMtxLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  RHMACAuthenCond    = PTHREAD_COND_INITIALIZER;
pthread_t       RHMACAuthenTID     = -1;
sem_t           sem_workers;

static GHashTable *macip_table  = NULL;
static MACAuthenSource *mac_src = NULL;
static GSourceFuncs     mac_event;
static gboolean mac_dispatch_handler (GSource *src, GSourceFunc callback,
                                      gpointer user_data);

static guint  macip_table_key_hash (gconstpointer key);
static gboolean macip_table_key_equal (gconstpointer v1, gconstpointer v2);
static void  macip_table_value_free (gpointer data);
static void *macauthen_service (void *data);
static void *do_macauthen (void *data);
static gboolean macauthen_verify  (RHMainServer *ms, MACAuthenElem *elem);
static gboolean macauthen_is_loggedin  (MACAuthenElem *elem);
static gboolean addr4_match (const struct in_addr *addr,
                             const struct in_addr *net, uint8_t bits);

static gboolean null_prepare_handler (GSource *src, gint *timeout);
static gboolean input_check_handler  (GSource *src);

static
int parse_attr_cb (const struct nlattr *attr, void *data)
{
        const struct nlattr **tb = data;
  int type = mnl_attr_get_type (attr);

  /* skip unsupported attribute in user-space */
  if (mnl_attr_type_valid (attr, NFQA_MAX) < 0)
    return MNL_CB_OK;

  switch (type) {
  case NFQA_MARK:
  case NFQA_IFINDEX_INDEV:
  case NFQA_IFINDEX_OUTDEV:
  case NFQA_IFINDEX_PHYSINDEV:
  case NFQA_IFINDEX_PHYSOUTDEV:
    if (mnl_attr_validate (attr, MNL_TYPE_U32) < 0) {
      return MNL_CB_ERROR;
    }
    break;
  case NFQA_TIMESTAMP:
    if (mnl_attr_validate2 (attr, MNL_TYPE_UNSPEC,
        sizeof (struct nfqnl_msg_packet_timestamp)) < 0) {
      return MNL_CB_ERROR;
    }
    break;
  case NFQA_HWADDR:
    if (mnl_attr_validate2 (attr, MNL_TYPE_UNSPEC,
        sizeof (struct nfqnl_msg_packet_hw)) < 0) {
      return MNL_CB_ERROR;
    }
    break;
  case NFQA_PAYLOAD:
    break;
  }
  tb[type] = attr;
  return MNL_CB_OK;
}

static int mac_queue_cb (const struct nlmsghdr *nlh, void *data)
{
  struct nlattr *tb[NFQA_MAX+1] = {};
  struct nfqnl_msg_packet_hdr *ph = NULL;
  struct nfqnl_msg_packet_hw  *mac_addr = NULL;
  uint32_t id = 0;

  mnl_attr_parse (nlh, sizeof (struct nfgenmsg), parse_attr_cb, tb);
  if (tb[NFQA_PACKET_HDR]) {
    ph = mnl_attr_get_payload (tb[NFQA_PACKET_HDR]);
    id = ntohl (ph->packet_id);

    mac_addr = mnl_attr_get_payload (tb[NFQA_HWADDR]);

    struct iphdr *hdr = NULL;
    struct in_addr src;
    if (tb[NFQA_PAYLOAD])
      {
        unsigned char *pkt = mnl_attr_get_payload (tb[NFQA_PAYLOAD]);
        hdr = (struct iphdr *)((u_int8_t *) pkt);
        src.s_addr = hdr->saddr;
      }

    DP ("MAC Authen main update table");

    macauthen_add_elem (mac_addr->hw_addr, hdr->saddr,
                        ntohl (mnl_attr_get_u32(tb[NFQA_IFINDEX_INDEV])),
                        MACAUTHEN_ELEM_RETRY_CLEARTIME);
  }

  return MNL_CB_OK + id;
}

static
struct nlmsghdr *
nfq_build_cfg_pf_request (char *buf, uint8_t command)
{
  struct nlmsghdr *nlh = mnl_nlmsg_put_header (buf);
  nlh->nlmsg_type = (NFNL_SUBSYS_QUEUE << 8) | NFQNL_MSG_CONFIG;
  nlh->nlmsg_flags = NLM_F_REQUEST;

  struct nfgenmsg *nfg = mnl_nlmsg_put_extra_header (nlh, sizeof (*nfg));
  nfg->nfgen_family = AF_UNSPEC;
  nfg->version = NFNETLINK_V0;

  struct nfqnl_msg_config_cmd cmd = {
    .command = command,
    .pf = htons (AF_INET),
  };
  mnl_attr_put (nlh, NFQA_CFG_CMD, sizeof (cmd), &cmd);

  return nlh;
}

static
struct nlmsghdr *
nfq_build_cfg_request (char *buf, uint8_t command, int queue_num)
{
  struct nlmsghdr *nlh = mnl_nlmsg_put_header (buf);
  nlh->nlmsg_type = (NFNL_SUBSYS_QUEUE << 8) | NFQNL_MSG_CONFIG;
  nlh->nlmsg_flags = NLM_F_REQUEST;

  struct nfgenmsg *nfg = mnl_nlmsg_put_extra_header (nlh, sizeof (*nfg));
  nfg->nfgen_family = AF_UNSPEC;
  nfg->version = NFNETLINK_V0;
  nfg->res_id = htons (queue_num);

  struct nfqnl_msg_config_cmd cmd = {
    .command = command,
    .pf = htons (AF_INET),
  };
  mnl_attr_put (nlh, NFQA_CFG_CMD, sizeof (cmd), &cmd);

  return nlh;
}

static
struct nlmsghdr *
nfq_build_cfg_params (char *buf, uint8_t mode, int range, int queue_num)
{
  struct nlmsghdr *nlh = mnl_nlmsg_put_header (buf);
  nlh->nlmsg_type = (NFNL_SUBSYS_QUEUE << 8) | NFQNL_MSG_CONFIG;
  nlh->nlmsg_flags = NLM_F_REQUEST;

  struct nfgenmsg *nfg = mnl_nlmsg_put_extra_header (nlh, sizeof (*nfg));
  nfg->nfgen_family = AF_UNSPEC;
  nfg->version = NFNETLINK_V0;
  nfg->res_id = htons (queue_num);

  struct nfqnl_msg_config_params params = {
    .copy_range = htonl (range),
    .copy_mode = mode,
  };
  mnl_attr_put (nlh, NFQA_CFG_PARAMS, sizeof (params), &params);

  return nlh;
}

static
struct nlmsghdr *
nfq_build_verdict (char *buf, int id, int queue_num, int verd)
{
  struct nlmsghdr *nlh;
  struct nfgenmsg *nfg;

  nlh = mnl_nlmsg_put_header (buf);
  nlh->nlmsg_type = (NFNL_SUBSYS_QUEUE << 8) | NFQNL_MSG_VERDICT;
  nlh->nlmsg_flags = NLM_F_REQUEST;
  nfg = mnl_nlmsg_put_extra_header (nlh, sizeof (*nfg));
  nfg->nfgen_family = AF_UNSPEC;
  nfg->version = NFNETLINK_V0;
  nfg->res_id = htons (queue_num);

  struct nfqnl_msg_verdict_hdr vh = {
    .verdict = htonl (verd),
    .id = htonl (id),
  };
  mnl_attr_put (nlh, NFQA_VERDICT_HDR, sizeof (vh), &vh);

  return nlh;
}

int
macauthen_setup (RHMainServer *main_server)
{
  struct mnl_socket *nl  = NULL;
  struct nlmsghdr   *nlh = NULL;;
  char buf[MNL_SOCKET_BUFFER_SIZE];
  unsigned int portid;
  unsigned int queue_num = 0;

  if (mac_src) {
    mnl_socket_close (mac_src->nl);
    mac_src->nl = NULL;

    g_source_remove (mac_src->src_id);
    g_source_destroy ((GSource *)mac_src);
    mac_src = NULL;
  }

  nl = mnl_socket_open (NETLINK_NETFILTER);
  if (!nl) {
    return -1;
  }

  if (mnl_socket_bind (nl, 0, MNL_SOCKET_AUTOPID) < 0) {
    goto fail;
  }

  portid = mnl_socket_get_portid (nl);
  nlh = nfq_build_cfg_pf_request (buf, NFQNL_CFG_CMD_PF_UNBIND);

  if (mnl_socket_sendto (nl, nlh, nlh->nlmsg_len) < 0) {
    goto fail;
  }

  nlh = nfq_build_cfg_pf_request (buf, NFQNL_CFG_CMD_PF_BIND);
  if (mnl_socket_sendto (nl, nlh, nlh->nlmsg_len) < 0) {
    goto fail;
  }

  nlh = nfq_build_cfg_request (buf, NFQNL_CFG_CMD_BIND, queue_num);
  if (mnl_socket_sendto (nl, nlh, nlh->nlmsg_len) < 0) {
    goto fail;
  }

  nlh = nfq_build_cfg_params (buf, NFQNL_COPY_PACKET, 0xFFFF, queue_num);
  if (mnl_socket_sendto (nl, nlh, nlh->nlmsg_len) < 0) {
    goto fail;
  }

  mac_event.prepare  = null_prepare_handler;
  mac_event.check    = input_check_handler;
  mac_event.dispatch = mac_dispatch_handler;
  mac_event.finalize = NULL;

  mac_src = (MACAuthenSource *) g_source_new (&mac_event,
                                              sizeof (MACAuthenSource));

  mac_src->pfd.fd = mnl_socket_get_fd (nl);
  mac_src->pfd.events = G_IO_IN;
  mac_src->nl = nl;
  mac_src->nl_portid = portid;
  mac_src->nl_queue_num = queue_num;

  fcntl (mac_src->pfd.fd, F_SETFL, fcntl (mac_src->pfd.fd, F_GETFL, 0) | O_NONBLOCK);

  if (!macip_table) {
    pthread_mutex_lock (&RHMACAuthenDataMtxLock);
    macip_table = g_hash_table_new_full (macip_table_key_hash,
                                         macip_table_key_equal,
                                         NULL,
                                         macip_table_value_free);
    pthread_mutex_unlock (&RHMACAuthenDataMtxLock);
  }

  sem_init (&sem_workers, 0, 16);

  if (pthread_create (&RHMACAuthenTID, NULL, macauthen_service,
                      (void *) main_server) == 0) {
    pthread_detach (RHMACAuthenTID);
    g_source_add_poll ((GSource *) mac_src, &mac_src->pfd);
    g_source_attach ((GSource *) mac_src, NULL);
  } else {
    sem_destroy (&sem_workers);
    g_source_destroy ((GSource *)mac_src);
    mac_src = NULL;
    goto fail;
  }

  return 0;

fail:
  DP ("MAC Authen setup failed!");
  mnl_socket_close (nl);
  return -1;
}

static
gboolean mac_dispatch_handler (GSource *src, GSourceFunc callback,
                               gpointer user_data)
{
  struct nlmsghdr *nlh = NULL;;
  char buf[MNL_SOCKET_BUFFER_SIZE];
  int  ret;
  uint32_t id;
  MACAuthenSource *mac_src = (MACAuthenSource *) src;

  ret = mnl_socket_recvfrom (mac_src->nl, buf, sizeof (buf));

  if (ret > 0) {
    ret = mnl_cb_run(buf, ret, 0, mac_src->nl_portid, mac_queue_cb, NULL);

    if (ret > 0) {
      id = ret - MNL_CB_OK;
      nlh = nfq_build_verdict (buf, id, mac_src->nl_queue_num, NF_ACCEPT);

      if (mnl_socket_sendto (mac_src->nl, nlh, nlh->nlmsg_len) < 0) {
        return FALSE;
      }

      DP ("Verdict ACCEPT - %u", id);

      if (pthread_mutex_trylock (&RHMACAuthenMtxLock) == 0) {
        pthread_cond_signal (&RHMACAuthenCond);
        pthread_mutex_unlock (&RHMACAuthenMtxLock);
      }
    }
  } else if (errno != EAGAIN) {
    DP ("Failed");
    return FALSE;
  }

  return TRUE;
}

static
gboolean null_prepare_handler (GSource *src, gint *timeout)
{
  *timeout = -1;
  return FALSE;
}

static
gboolean input_check_handler (GSource *src)
{
  MACAuthenSource *s = (MACAuthenSource *) src;

  return s->pfd.revents & POLLIN;
}

static void *do_macauthen (void *data)
{
  sem_wait (&sem_workers);
  MACAuthenElem *elem = (MACAuthenElem *) data;
  DP ("Send MACAuthen");
  send_xmlrpc_macauthen (elem);

  sem_post (&sem_workers);
}

static
void *macauthen_service (void *data)
{
  RHMainServer *ms = (RHMainServer *) data;
  time_t last_process = 0;

  for (;;) {
    pthread_mutex_lock (&RHMACAuthenMtxLock);
    pthread_cond_wait (&RHMACAuthenCond, &RHMACAuthenMtxLock);

    DP ("MAC Authen service table process");
    GHashTableIter iter;
    gpointer key;
    gpointer value;

    g_hash_table_iter_init (&iter, macip_table);

    time_t start_process = time (NULL);
    int    count = 0;

    while (g_hash_table_iter_next (&iter, &key, &value)) {
      MACAuthenElem *elem = (MACAuthenElem *) value;

      if (elem->last > 0) {
        time_t elapse = (start_process - elem->last);
        if (elapse > elem->cleartime) {
          DP ("Entry timeout - iface-idx: %d, ip: %s, "
              "mac: %02x:%02x:%02x:%02x:%02x:%02x, "
              "cleartime: %d",
          elem->iface_idx,
          inet_ntoa (elem->src),
          elem->mac[0], elem->mac[1], elem->mac[2],
          elem->mac[3], elem->mac[4], elem->mac[5],
          elem->cleartime);

          pthread_mutex_lock (&RHMACAuthenDataMtxLock);
          g_hash_table_iter_remove (&iter);
          pthread_mutex_unlock (&RHMACAuthenDataMtxLock);
        }

        continue;
      }

      DP ("Process - iface-idx: %d, ip: %s, mac: %02x:%02x:%02x:%02x:%02x:%02x"
          ", last: %d",
          elem->iface_idx,
          inet_ntoa (elem->src),
          elem->mac[0], elem->mac[1], elem->mac[2],
          elem->mac[3], elem->mac[4], elem->mac[5],
          elem->last);

      if (macauthen_verify (ms, elem) && !macauthen_is_loggedin (elem)) {
        time (&elem->last);

        pthread_t tid;
        pthread_attr_t attr;
        size_t stacksize;

        pthread_attr_init (&attr);
        pthread_attr_setstacksize (&attr, 512000);

        if (pthread_create (&tid, &attr, &do_macauthen, (void *) elem) == 0) {
          pthread_detach (tid);
        }
        pthread_attr_destroy (&attr);

        /* Limit the trhead creation not over the 100 thread/second */
        struct timespec delay, remain;
        delay.tv_sec = 0;
        delay.tv_nsec = 10000000; /* 10 ms */

        int s = nanosleep (&delay, &remain);
        while (s == -1) {
          delay.tv_nsec = remain.tv_nsec;
          s = nanosleep (&delay, &remain);
        }
      } else {
        pthread_mutex_lock (&RHMACAuthenDataMtxLock);
        g_hash_table_iter_remove (&iter);
        pthread_mutex_unlock (&RHMACAuthenDataMtxLock);
      }
    }

    pthread_mutex_unlock (&RHMACAuthenMtxLock);
  }
}

static
guint macip_table_key_hash (gconstpointer key)
{
  guint h = jhash (key, sizeof (MACAuthenKey), 0);
  DP ("Hash: %u", h);
  return h;
}

static
gboolean macip_table_key_equal (gconstpointer v1, gconstpointer v2)
{
  return memcmp (v1, v2, sizeof (MACAuthenKey)) == 0;
}

static
void macip_table_value_free (gpointer data)
{
  g_free (data);
}

static
gboolean macauthen_verify (RHMainServer *ms, MACAuthenElem *elem)
{
  GList *vserver_list = ms->vserver_list;
  GList *runner = g_list_first(vserver_list);
  RHVServer *vs = NULL;

  while (runner != NULL) {
    vs = (RHVServer *)runner->data;

    if ((vs->vserver_config->dev_internal_idx == elem->iface_idx) &&
        !NO_MAC (elem->mac) &&
        (addr4_match (&elem->src, &vs->vserver_config->clients_net,
          vs->vserver_config->clients_prefix)))
      {
        DP ("MAC Authen - MAC-IP/Network mapping valid");
        elem->vs = vs;
        return TRUE;
      }

    runner = g_list_next(runner);
  }

  return FALSE;
}

static
gboolean addr4_match (const struct in_addr *addr, const struct in_addr *net,
                      uint8_t bits)
{
  if (bits == 0)
    return TRUE;

  return !((addr->s_addr ^ net->s_addr) & htonl (0xffffffffu << (32 - bits)));
}

static
gboolean macauthen_is_loggedin  (MACAuthenElem *elem)
{
  uint32_t id;
  gboolean ret = FALSE;

  pthread_mutex_lock (&RHMtxLock);
  id = ntohl (elem->src.s_addr) - ntohl (elem->vs->v_map->first_ip);

  if (id > 0 && id <= elem->vs->v_map->size &&
        member_get_node_by_id(elem->vs, id)) {
    ret = TRUE;
  }

  pthread_mutex_unlock (&RHMtxLock);

  return ret;
}

MACAuthenElem *
macauthen_add_elem (uint8_t *mac, unsigned long s_addr, uint32_t iface_idx,
                    time_t cleartime)
{
  MACAuthenElem *elem = (MACAuthenElem *) g_malloc0 (sizeof (MACAuthenElem));
  MACAuthenElem *free_elem = NULL;

  memcpy (elem->mac, mac, 6);
  elem->src.s_addr = s_addr;
  elem->iface_idx  = iface_idx;
  elem->cleartime  = cleartime;
  elem->last       = 0;

  pthread_mutex_lock (&RHMACAuthenDataMtxLock);

  MACAuthenElem *found = (MACAuthenElem *) g_hash_table_lookup (macip_table, elem);

  if (!found) {
    if (elem->cleartime == MACAUTHEN_ELEM_DELAY_CLEARTIME)
      time (&elem->last);

    g_hash_table_insert (macip_table, elem, elem);

    found = (MACAuthenElem *) g_hash_table_lookup (macip_table, elem);
    if (!found)
      free_elem = elem;
  } else {
    if (found->cleartime < cleartime) {
      found->cleartime = cleartime;
      time (&found->last);
    }

    free_elem = elem;
    elem = found;
  }

  pthread_mutex_unlock (&RHMACAuthenDataMtxLock);

  if (free_elem)
    g_free (free_elem);

  return elem;
}
