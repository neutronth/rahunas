/**
 * RahuNAS XML-RPC Server implementation
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 *         Suriya Soutmun <darksolar@gmail.com>
 * Date:   2008-08-07
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
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
#include "rh-json.h"
#include "rh-macauthen.h"

extern const char* termstring;

static
json_t *get_json_params (const gchar *param)
{
  json_t       *root         = NULL;
  gsize         decoded_size = 0;
  gchar        *decoded      = NULL;
  json_error_t  error;

  DP("RPC Receive: %s", param);

  decoded = g_base64_decode (param, &decoded_size);
  DP ("RPC JSON: %s", decoded);

  root = json_loads (decoded, 0, &error);
  g_free (decoded);

  return root;
}

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
  const char *vserver_id = NULL;
  const char *ip = NULL;
  const char *username = NULL;
  const char *session_id = NULL;
  const char *mac_address = NULL;
  time_t      session_timeout    = 0;
  long        bandwidth_max_down = 0;
  long        bandwidth_max_up   = 0;
  const char *serviceclass_name = NULL;
  const char *secure_token = NULL;
  uint32_t id;
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;
  json_t *root = NULL;

  pthread_mutex_lock (&RHMtxLock);

  if (param == NULL)
    goto out;

  root = get_json_params (param);

  vserver_id  = get_json_data_string ("VServerID", root);
  ip          = get_json_data_string ("IP", root);
  username    = get_json_data_string ("Username", root);
  session_id  = get_json_data_string ("SessionID", root);
  mac_address = get_json_data_string ("MAC", root);
  session_timeout    = (time_t) get_json_data_integer ("Session-Timeout", root);
  bandwidth_max_down = get_json_data_integer ("Bandwidth-Max-Down", root);
  bandwidth_max_up   = get_json_data_integer ("Bandwidth-Max-Up", root);
  serviceclass_name  = get_json_data_string ("Class-Of-Service", root);
  secure_token       = get_json_data_string ("SecureToken", root);

  if (ip == NULL || username == NULL || session_id == NULL
      || vserver_id == NULL)
    goto out;

  vs = vserver_get_by_id(ms, atoi(vserver_id));

  if (vs == NULL)
    goto out;

  if (mac_address == NULL)
    mac_address = DEFAULT_MAC;

  id = iptoid(vs->v_map, ip);

  if (id < 0) {
    *reply_string = create_json_reply (400, "{ss}", "Message",
                                       "Invalid IP Address");
    goto cleanup;
  }

  /* Check if client already registered */
  member_node = member_get_node_by_id(vs, id);

  if (member_node != NULL)
    goto pass;

  req.id = id;
  req.vserver_id = atoi(vserver_id);
  req.username = username;
  req.session_id = session_id;
  parse_mac(mac_address, ethernet);
  memcpy(req.mac_address, ethernet, ETH_ALEN);
  req.session_start = 0;
  req.session_timeout = 0;
  req.bandwidth_slot_id = 0;

  if (session_timeout > 0)
    req.session_timeout = time(NULL) + session_timeout;

  req.bandwidth_max_down = bandwidth_max_down;
  req.bandwidth_max_up   = bandwidth_max_up;

  if (secure_token != NULL) {
    memset (req.secure_token, '\0', sizeof (req.secure_token));
    strncpy (req.secure_token, secure_token, sizeof (req.secure_token) - 1);
  }

  req.serviceclass_name    = serviceclass_name;
  req.serviceclass_slot_id = 0;

  rh_task_startsess(vs, &req);
  member_node = member_get_node_by_id(vs, id);

  if (member_node != NULL) {
pass:
    member = (struct rahunas_member *)member_node->data;
    *reply_string = create_json_reply (200, "{ss}",
                                       "Mapping", member->mapping_ip);

    goto cleanup;
  }


out:
  *reply_string = create_json_reply (400, "{ss}",
                                     "Message", "Invalid input parameters");

cleanup:
  pthread_mutex_unlock (&RHMtxLock);

  json_decref (root);

  DP("RPC Reply: %s", *reply_string);
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
  const char *vserver_id = NULL;
  const char *ip = NULL;
  const char *mac_address = NULL;
  int cause_id = 0;
  uint32_t   id;
  int res = 0;
  unsigned char ethernet[ETH_ALEN] = {0,0,0,0,0,0};
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;
  json_t *root = NULL;

  pthread_mutex_lock (&RHMtxLock);

  if (param == NULL)
    goto out;

  root = get_json_params (param);

  vserver_id  = get_json_data_string ("VServerID", root);
  ip          = get_json_data_string ("IP", root);
  mac_address = get_json_data_string ("MAC", root);
  cause_id    = get_json_data_integer ("TerminateCause", root);

  if (ip == NULL || vserver_id == NULL)
    goto out;
  vs = vserver_get_by_id(ms, atoi(vserver_id));

  if (vs == NULL)
    goto out;

  if (mac_address == NULL)
    mac_address = DEFAULT_MAC;

  id = iptoid(vs->v_map, ip);

  if (id < 0) {
    *reply_string = create_json_reply (400, "{ss}", "Message",
                                       "Invalid IP Address");
    goto cleanup;
  }

  parse_mac(mac_address, ethernet);
  memcpy(req.mac_address, ethernet, ETH_ALEN);

  member_node = member_get_node_by_id(vs, id);

  if (member_node != NULL) {
    member = (struct rahunas_member *) member_node->data;

    rh_data_sync (vs->vserver_config->vserver_id, member);

    if (memcmp(ethernet, member->mac_address, ETH_ALEN) == 0) {
      req.id = id;

      if (cause_id >= RH_RADIUS_TERM_USER_REQUEST &&
          cause_id <= RH_RADIUS_TERM_HOST_REQUEST) {
        req.req_opt = cause_id;
      } else {
        req.req_opt = RH_RADIUS_TERM_USER_REQUEST;
      }

      res = send_xmlrpc_stopacct (vs, id, cause_id);
      if (res == 0) {
        res = rh_task_stopsess(vs, &req);

        if (!NO_MAC (member->mac_address)) {
          macauthen_add_elem (member->mac_address,
                              htonl ((ntohl (vs->v_map->first_ip) + id)),
                              vs->vserver_config->dev_internal_idx,
                              MACAUTHEN_ELEM_DELAY_CLEARTIME);
        }
      }

      if (res == 0) {
        *reply_string = create_json_reply (200, "{ss}", "Message",
                                           "Client was removed");
      } else {
        *reply_string = create_json_reply (400, "{ss}", "Message",
                                           "Error");
      }

      goto cleanup;
    }
  }

out:
  *reply_string = create_json_reply (400, "{ss}", "Message",
                                     "Invalid input parameters");

cleanup:
  pthread_mutex_unlock (&RHMtxLock);

  json_decref (root);

  DP("RPC Reply: %s", *reply_string);
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
  const char *vserver_id = NULL;
  const char *ip = NULL;
  uint32_t   id;
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;
  json_t *root = NULL;

  pthread_mutex_lock (&RHMtxLock);

  if (param == NULL)
    goto out;

  root = get_json_params (param);

  vserver_id  = get_json_data_string ("VServerID", root);
  ip          = get_json_data_string ("IP", root);

  if (ip == NULL || vserver_id == NULL)
    goto out;

  vs = vserver_get_by_id(ms, atoi(vserver_id));

  if (vs == NULL)
    goto out;

  id = iptoid(vs->v_map, ip);

  if (id < 0) {
    *reply_string = create_json_reply (400, "{ss}", "Message",
                                       "Invalid IP Address");
    goto cleanup;
  }

  member_node = member_get_node_by_id(vs, id);

  if (member_node != NULL) {
    member = (struct rahunas_member *) member_node->data;
    if (!member->username) {
      *reply_string = create_json_reply (400, "{ss}", "Message",
                                         "Invalid Username");
      goto cleanup;
    }

    if (!member->session_id) {
      *reply_string = create_json_reply (400, "{ss}", "Message",
                                         "Invalid Session ID");
      goto cleanup;
    }

    *reply_string = create_json_reply (200,
                      "{ss,ss,ss,si,ss,si,ss,si,si,si,si}",
                      "ip", ip,
                      "username", member->username,
                      "session_id", member->session_id,
                      "session_start", member->session_start,
                      "mac_address", mac_tostring (member->mac_address),
                      "session_timeout", member->session_timeout,
                      "serviceclass_description",
                        member->serviceclass_description,
                      "download_bytes", member->download_bytes,
                      "upload_bytes", member->upload_bytes,
                      "download_speed", member->bandwidth_max_down,
                      "upload_speed", member->bandwidth_max_up);

    goto cleanup;
  }

  *reply_string = g_strdup_printf("%s", ip);
  goto cleanup;

out:
  *reply_string = create_json_reply (400, "{ss}",
                                     "Message", "Invalid input parameters");

cleanup:
  pthread_mutex_unlock (&RHMtxLock);

  json_decref (root);

  DP("RPC Reply: %s", *reply_string);
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
  const char *vserver_id   = NULL;
  const char *session_id   = NULL;
  const char *ip = NULL;
  const char *secure_token = NULL;
  const char *roaming_ip   = NULL;
  const char *mac_address  = NULL;
  gchar *prev_mac_address = NULL;
  time_t new_session_timeout  = 0;
  uint32_t   id;
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;
  int    valid = 0;
  struct task_req req;
  json_t *root = NULL;

  pthread_mutex_lock (&RHMtxLock);

  if (param == NULL)
    goto out;

  root = get_json_params (param);

  vserver_id   = get_json_data_string ("VServerID", root);
  session_id   = get_json_data_string ("SessionID", root);
  ip           = get_json_data_string ("IP", root);
  secure_token = get_json_data_string ("SecureToken", root);
  roaming_ip   = get_json_data_string ("RoamingIP", root);
  mac_address  = get_json_data_string ("MAC", root);

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

      *reply_string = create_json_reply (200,
                        "{ss,ss,ss,si,ss,si,ss,si,si}",
                        "ip", ip,
                        "username", member->username,
                        "session_id", member->session_id,
                        "session_start", member->session_start,
                        "mac_address", mac_tostring (member->mac_address),
                        "session_timeout", new_session_timeout,
                        "serviceclass_name",
                          member->serviceclass_name ?
                            member->serviceclass_name : "",
                        "bandwidth_max_down", member->bandwidth_max_down,
                        "bandwidth_max_up", member->bandwidth_max_up);

      if (send_xmlrpc_stopacct(vs, id, RH_RADIUS_TERM_USER_REQUEST) == 0) {
          req.id = id;
          memcpy (req.mac_address, member->mac_address, ETH_ALEN);
          req.req_opt = RH_RADIUS_TERM_USER_REQUEST;
          rh_task_stopsess (vs, &req);
          goto cleanup;
      } else {
        g_free (*reply_string);
      }
    }
  }

out:
  *reply_string = create_json_reply (400, "{ss}",
                                     "Message", "Invalid Roaming");

cleanup:
  pthread_mutex_unlock (&RHMtxLock);

  g_free (prev_mac_address);
  json_decref (root);

  DP("RPC Reply: %s", *reply_string);
  return 0;
}
