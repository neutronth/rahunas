/**
 * RahuNAS XML-RPC Server implementation
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 *         Suriya Soutmun <darksolar@gmail.com>
 * Date:   2008-08-07
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <inttypes.h>
#include "rahunasd.h"
#include "rh-server.h"
#include "rh-xmlrpc-server.h"
#include "rh-radius.h"
#include "rh-ipset.h"
#include "rh-utils.h"
#include "rh-task.h"
#include "rh-task-memset.h"

extern const char* termstring;

int do_startsession(GNetXmlRpcServer *server,
                    const gchar *command,
                    const gchar *param,
                    gpointer user_data,
                    gchar **reply_string) 
{
  RHMainServer *ms = (RHMainServer *)user_data;
  RHVServer    *vs = NULL;
  unsigned char ethernet[ETH_ALEN] = {0,0,0,0,0,0};
  struct task_req req;
  gchar *vserver_id = NULL;
  gchar *ip = NULL;
  gchar *username = NULL;
  gchar *session_id = NULL;
  gchar *mac_address = NULL;
  gchar *session_timeout = NULL;
  gchar *bandwidth_max_down = NULL;
  gchar *bandwidth_max_up = NULL;
  gchar *serviceclass_name = NULL;
  gchar *secure_token = NULL;
  uint32_t id;
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;

  pthread_mutex_lock (&RHMtxLock);

  if (param == NULL)
    goto out;

  DP("RPC Receive: %s", param);

  vserver_id  = rh_string_get_sep(param, "|", 1);
  ip          = rh_string_get_sep(param, "|", 2);
  username    = rh_string_get_sep(param, "|", 3);
  session_id  = rh_string_get_sep(param, "|", 4);
  mac_address = rh_string_get_sep(param, "|", 5);
  session_timeout    = rh_string_get_sep(param, "|", 6);
  bandwidth_max_down = rh_string_get_sep(param, "|", 7);
  bandwidth_max_up   = rh_string_get_sep(param, "|", 8);
  serviceclass_name  = rh_string_get_sep(param, "|", 9);
  secure_token       = rh_string_get_sep(param, "|", 10);

  if (ip == NULL || username == NULL || session_id == NULL 
      || vserver_id == NULL)
    goto out;

  vs = vserver_get_by_id(ms, atoi(vserver_id));

  if (vs == NULL)
    goto out;

  if (mac_address == NULL)
    mac_address = g_strdup(DEFAULT_MAC);

  id = iptoid(vs->v_map, ip);

  if (id < 0) {
    *reply_string = g_strdup("Invalid IP Address");
    goto cleanup;
  }

  /* Check if client already registered */
  member_node = member_get_node_by_id(vs, id);

  if (member_node != NULL)
    goto greeting;

  req.id = id;
  req.vserver_id = atoi(vserver_id);
  req.username = username;
  req.session_id = session_id;
  parse_mac(mac_address, ethernet);
  memcpy(req.mac_address, ethernet, ETH_ALEN);
  req.session_start = 0;
  req.session_timeout = 0;
  req.bandwidth_slot_id = 0;

  if (session_timeout != NULL) {
    if (atol(session_timeout) != 0)
      req.session_timeout = time(NULL) + atol(session_timeout);
  }

  if (bandwidth_max_down != NULL)
    req.bandwidth_max_down = atol(bandwidth_max_down);
  else
    req.bandwidth_max_down = 0;

  if (bandwidth_max_up != NULL)
    req.bandwidth_max_up = atol(bandwidth_max_up);
  else
    req.bandwidth_max_up = 0;

  if (secure_token != NULL) {
    memset (req.secure_token, '\0', sizeof (req.secure_token));
    strncpy (req.secure_token, secure_token, sizeof (req.secure_token) - 1);
  }

  req.serviceclass_name    = serviceclass_name;
  req.serviceclass_slot_id = 0;

  rh_task_startsess(vs, &req);
  member_node = member_get_node_by_id(vs, id);

greeting:
  if (member_node != NULL) {
    member = (struct rahunas_member *)member_node->data;
    *reply_string = g_strdup_printf("Greeting! Got: IP %s, User %s, ID %s, "
                                    "Service Class %s, Mapping %s",
                                    ip, member->username, 
                                    member->session_id,
                                    member->serviceclass_name,
                                    member->mapping_ip);
    goto cleanup;
  }

out:
  *reply_string = g_strdup("Invalid input parameters");

cleanup:
  pthread_mutex_unlock (&RHMtxLock);

  DP("RPC Reply: %s", *reply_string);
  g_free(ip);
  g_free(username);
  g_free(session_id);
  g_free(mac_address);
  g_free(session_timeout);
  g_free(bandwidth_max_down);
  g_free(bandwidth_max_up);
  g_free(serviceclass_name);
  g_free(vserver_id);
  g_free(secure_token);
  return 0;
}

int do_stopsession(GNetXmlRpcServer *server,
                   const gchar *command,
                   const gchar *param,
                   gpointer user_data,
                   gchar **reply_string)
{
  RHMainServer *ms = (RHMainServer *)user_data;
  RHVServer    *vs = NULL;
  struct task_req req;
  gchar *vserver_id = NULL;
  gchar *ip = NULL;
  gchar *mac_address = NULL;
  gchar *cause = NULL;
  int cause_id = 0;
  uint32_t   id;
  int res = 0;
  unsigned char ethernet[ETH_ALEN] = {0,0,0,0,0,0};
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;

  pthread_mutex_lock (&RHMtxLock);

  if (param == NULL)
    goto out;

  DP("RPC Receive: %s", param);

  vserver_id  = rh_string_get_sep(param, "|", 1);
  ip          = rh_string_get_sep(param, "|", 2);
  mac_address = rh_string_get_sep(param, "|", 3);
  cause       = rh_string_get_sep(param, "|", 4);

  if (ip == NULL || vserver_id == NULL)
    goto out;
  vs = vserver_get_by_id(ms, atoi(vserver_id));

  if (vs == NULL)
    goto out;

  if (mac_address == NULL)
    mac_address = g_strdup(DEFAULT_MAC);

  id = iptoid(vs->v_map, ip);

  if (id < 0) {
    *reply_string = g_strdup("Invalid IP Address");
    goto cleanup;
  }

  parse_mac(mac_address, ethernet);
  memcpy(req.mac_address, ethernet, ETH_ALEN);

  member_node = member_get_node_by_id(vs, id);

  if (member_node != NULL) {
    member = (struct rahunas_member *) member_node->data;

    rh_data_sync (vs->vserver_config->vserver_id, member);

    if (memcmp(&ethernet, &member->mac_address, 
        ETH_ALEN) == 0) {
      req.id = id;
      
      if (cause == NULL) {
        req.req_opt = RH_RADIUS_TERM_USER_REQUEST;
      } else {
        cause_id = atoi(cause);
        if (cause_id >= RH_RADIUS_TERM_USER_REQUEST && 
            cause_id <= RH_RADIUS_TERM_HOST_REQUEST) {
          req.req_opt = cause_id;
        } else {
          req.req_opt = RH_RADIUS_TERM_USER_REQUEST;
        }
      }

      res = send_xmlrpc_stopacct (vs, id, cause_id);
      if (res == 0) {
        res = rh_task_stopsess(vs, &req);
      }

      if (res == 0) {
        *reply_string = g_strdup_printf("Client IP %s was removed!", 
                                          idtoip(vs->v_map, id));
      } else {
         *reply_string = g_strdup_printf("Client IP %s error remove!", 
                                        idtoip(vs->v_map, id));
      }
      goto cleanup;
    } 
  }

  *reply_string = g_strdup_printf("%s", ip);
  goto cleanup;

out:
  *reply_string = g_strdup("Invalid input parameters");

cleanup:
  pthread_mutex_unlock (&RHMtxLock);

  DP("RPC Reply: %s", *reply_string);
  g_free(ip);
  g_free(mac_address);
  g_free(cause);
  g_free(vserver_id);
  return 0;
}

int do_getsessioninfo(GNetXmlRpcServer *server,
                      const gchar *command,
                      const gchar *param,
                      gpointer user_data,
                      gchar **reply_string)
{
  RHMainServer *ms = (RHMainServer *)user_data;
  RHVServer    *vs = NULL;
  gchar *vserver_id = NULL;
  gchar *ip = NULL;
  uint32_t   id;
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;

  pthread_mutex_lock (&RHMtxLock);

  if (param == NULL)
    goto out;

  DP("RPC Receive: %s", param);

  vserver_id  = rh_string_get_sep(param, "|", 1);
  ip          = rh_string_get_sep(param, "|", 2);

  if (ip == NULL || vserver_id == NULL)
    goto out;

  vs = vserver_get_by_id(ms, atoi(vserver_id));

  if (vs == NULL)
    goto out;

  id = iptoid(vs->v_map, ip);

  if (id < 0) {
    *reply_string = g_strdup("Invalid IP Address");
    goto cleanup;
  }

  member_node = member_get_node_by_id(vs, id);

  if (member_node != NULL) {
    member = (struct rahunas_member *) member_node->data;
    if (!member->username) {
      *reply_string = g_strdup("Invalid Username");
      goto cleanup;
    }

    if (!member->session_id) {
      *reply_string = g_strdup("Invalid Session ID");
      goto cleanup;
    }
    
    *reply_string = g_strdup_printf("%s|%s|%s|%d|%s|%d|%s|%" PRId64 "|%" PRId64
                                    "|%lu|%lu",
                                    ip, 
                                    member->username,
                                    member->session_id,
                                    member->session_start,
                                    mac_tostring(member->mac_address),
                                    member->session_timeout,
                                    member->serviceclass_description,
                                    member->download_bytes,
                                    member->upload_bytes,
                                    member->bandwidth_max_down,
                                    member->bandwidth_max_up);
    goto cleanup;
  }

  *reply_string = g_strdup_printf("%s", ip);
  goto cleanup;

out:
  *reply_string = g_strdup("Invalid input parameters");

cleanup:
  pthread_mutex_unlock (&RHMtxLock);

  DP("RPC Reply: %s", *reply_string);
  g_free(ip);
  g_free(vserver_id);
  return 0;
}

int do_roaming(GNetXmlRpcServer *server,
                      const gchar *command,
                      const gchar *param,
                      gpointer user_data,
                      gchar **reply_string)
{
  RHMainServer *ms = (RHMainServer *)user_data;
  RHVServer    *vs = NULL;
  gchar *vserver_id   = NULL;
  gchar *session_id   = NULL;
  gchar *ip = NULL;
  gchar *secure_token = NULL;
  gchar *roaming_ip   = NULL;
  gchar *mac_address  = NULL;
  gchar *prev_mac_address = NULL;
  time_t new_session_timeout  = 0;
  uint32_t   id;
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;
  int    valid = 0;
  struct task_req req;

  pthread_mutex_lock (&RHMtxLock);

  if (param == NULL)
    goto out;

  DP("RPC Receive: %s", param);

  vserver_id   = rh_string_get_sep(param, "|", 1);
  session_id   = rh_string_get_sep(param, "|", 2);
  ip           = rh_string_get_sep(param, "|", 3);
  secure_token = rh_string_get_sep(param, "|", 4);
  roaming_ip   = rh_string_get_sep(param, "|", 5);
  mac_address  = rh_string_get_sep(param, "|", 6);

  if (ip == NULL || vserver_id == NULL || session_id == NULL)
    goto out;

  vs = vserver_get_by_id(ms, atoi(vserver_id));

  if (vs == NULL)
    goto out;

  id = iptoid(vs->v_map, ip);

  if (id < 0) {
    goto out;
  }

  member_node = member_get_node_by_id(vs, id);

  if (member_node != NULL) {
    member = (struct rahunas_member *) member_node->data;
    if (!member->username) {
      goto out;
    }

    if (!member->session_id) {
      goto out;
    }


    if (strcmp (member->session_id, session_id) == 0 &&
        strcmp (member->secure_token, secure_token) == 0) {

      prev_mac_address = g_strdup (mac_tostring (member->mac_address));

      if (strcasecmp (prev_mac_address, DEFAULT_MAC) == 0) {
        valid = 1;
      } else if (strcasecmp (prev_mac_address, mac_address) == 0){
        valid = 1;
      }
    }

    if (valid) {
      new_session_timeout = member->session_timeout == 0 ? 0 :
        (member->session_timeout - (time (NULL) - member->session_start))
          - time (NULL);

      *reply_string = g_strdup_printf("%s|%s|%s|%d|%s|%d|%s|%lu|%lu",
                                      ip,
                                      member->username,
                                      member->session_id,
                                      member->session_start,
                                      mac_tostring(member->mac_address),
                                      new_session_timeout,
                                      member->serviceclass_name,
                                      member->bandwidth_max_down,
                                      member->bandwidth_max_up);

      if (send_xmlrpc_stopacct(vs, id, RH_RADIUS_TERM_USER_REQUEST) == 0) {
          req.id = id;
          memcpy (req.mac_address, member->mac_address, ETH_ALEN);
          req.req_opt = RH_RADIUS_TERM_USER_REQUEST;
          rh_task_stopsess (vs, &req);
          goto cleanup;
      } else {
        g_free (reply_string);
      }
    }
  }

out:
  *reply_string = g_strdup("Invalid");

cleanup:
  pthread_mutex_unlock (&RHMtxLock);

  DP("RPC Reply: %s", *reply_string);
  g_free(vserver_id);
  g_free(session_id);
  g_free(ip);
  g_free(secure_token);
  g_free(roaming_ip);
  g_free(mac_address);
  g_free(prev_mac_address);

  return 0;
}
