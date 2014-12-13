/**
 * XML-RPC client command sender
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-09-02
 */

#include <inttypes.h>
#include "../xmlrpc/xmlrpc.h"
#include "rahunasd.h"
#include "rh-xmlrpc-cmd.h"
#include "rh-task-memset.h"
#include "rh-ipset.h"
#include "rh-json.h"

static
int send_xmlrpc (RHVServer *vs, const char *cmd, const char *params)
{
  GNetXmlRpcClient *client = NULL;
  gchar *reply  = NULL;
  int    ret    = 0;

  client = gnet_xmlrpc_client_new(XMLSERVICE_HOST, XMLSERVICE_URL,
                                  XMLSERVICE_PORT);

  if (!client) {
    logmsg(RH_LOG_ERROR, "[%s] Could not connect to XML-RPC service", vs->vserver_config->vserver_name);
    return -1;
  }

  if (gnet_xmlrpc_client_call(client, cmd, params, &reply) == 0)
    {
      DP("%s reply = %d", cmd, get_json_reply_status (reply));
      if (get_json_reply_status (reply) != 200)
        ret = -1;

      g_free(reply);
    }
  else
    {
      if (reply != NULL) {
        g_free(reply);
      }

      DP("%s", "Failed executing stopacct!");
      ret = -1;
    }

  gnet_xmlrpc_client_delete(client);

  return ret;
}

int send_xmlrpc_stopacct(RHVServer *vs, uint32_t id, int cause) {
  gchar *params = NULL;
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;
  int    ret    = 0;

  if (!vs)
    return -1;

  if (id < 0 || id > (vs->v_map->size - 1))
    return -1;

  member_node = member_get_node_by_id(vs, id);
  if (member_node == NULL)
    return -1;

  member = (struct rahunas_member *)member_node->data;

  params = create_json_request ("{ss,ss,ss,si,ss,si,si,si}",
                                "IP", idtoip (vs->v_map, id),
                                "Username", member->username,
                                "SessionID", member->session_id,
                                "SessionStart", member->session_start,
                                "MAC", mac_tostring (member->mac_address),
                                "Cause", cause,
                                "DownloadBytes", member->download_bytes,
                                "UploadBytes", member->upload_bytes);

  if (!params)
    return -1;

  ret = send_xmlrpc (vs, "stopacct", params);
  g_free(params);

  return ret;
}

int send_xmlrpc_interimupdate(RHVServer *vs, uint32_t id) {
  gchar *params = NULL;
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;
  int    ret    = 0;

  if (!vs)
    return -1;

  if (id < 0 || id > (vs->v_map->size - 1))
    return -1;

  member_node = member_get_node_by_id(vs, id);
  if (member_node == NULL)
    return -1;

  member = (struct rahunas_member *)member_node->data;

  params = create_json_request ("{ss,ss,ss,si,ss,si,si}",
                                "IP", idtoip (vs->v_map, id),
                                "Username", member->username,
                                "SessionID", member->session_id,
                                "SessionStart", member->session_start,
                                "MAC", mac_tostring (member->mac_address),
                                "DownloadBytes", member->download_bytes,
                                "UploadBytes", member->upload_bytes);

  if (params == NULL)
    return -1;

  ret = send_xmlrpc (vs, "update", params);
  g_free(params);

  return ret;
}

int send_xmlrpc_offacct(RHVServer *vs) {
  gchar *params = NULL;
  int   ret     = 0;

  if (!vs)
    return -1;

  params = create_json_request ("{si}",
                                "VServerID", vs->vserver_config->vserver_id);

  if (params == NULL)
    return -1;

  ret = send_xmlrpc (vs, "offacct", params);
  g_free(params);

  return 0;
}

int send_xmlrpc_macauthen(MACAuthenElem *elem) {
  int   ret     = 0;
  gchar *params = NULL;
  char  mac[18];

  sprintf (mac, "%02x:%02x:%02x:%02x:%02x:%02x",
             elem->mac[0], elem->mac[1], elem->mac[2],
             elem->mac[3], elem->mac[4], elem->mac[5]);

  params = create_json_request ("{ss,ss}",
             "IP", inet_ntoa (elem->src),
             "MAC", mac);

  if (!params)
    return -1;

  ret = send_xmlrpc (elem->vs, "macauthen", params);

  g_free(params);

  return ret;
}
