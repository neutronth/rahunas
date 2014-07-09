/**
 * XML-RPC client command sender
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-09-02
 */

#include "../xmlrpc/xmlrpc.h"
#include "rahunasd.h"
#include "rh-xmlrpc-cmd.h"
#include "rh-task-memset.h"

int send_xmlrpc_stopacct(RHVServer *vs, uint32_t id, int cause) {
  GNetXmlRpcClient *client = NULL;
  gchar *reply  = NULL;
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
  params = g_strdup_printf("%s|%s|%s|%d|%s|%d", 
                           idtoip(vs->v_map, id),
                           member->username,
                           member->session_id,
                           member->session_start,
                           mac_tostring(member->mac_address),
                           cause);

  DP("Params = %s", params);

  if (params == NULL)
    return -1;

  client = gnet_xmlrpc_client_new(XMLSERVICE_HOST, XMLSERVICE_URL, 
                                  XMLSERVICE_PORT);

  if (!client) {
    logmsg(RH_LOG_ERROR, "[%s] Could not connect to XML-RPC service", vs->vserver_config->vserver_name);
    return -1;
  }
  
  if (gnet_xmlrpc_client_call(client, "stopacct", params, &reply) == 0)
    {
      DP("stopacct reply = %s", reply);
      if (strcmp (reply, "FAIL") == 0)
        ret = -1;

      g_free(reply);
    }
  else
    {
      if (reply != NULL) {
        DP("stopacct reply = %s", reply);
        g_free(reply);
      }

      DP("%s", "Failed executing stopacct!");
      ret = -1;
    }

  gnet_xmlrpc_client_delete(client);

  g_free(params);

  return ret;
}

int send_xmlrpc_offacct(RHVServer *vs) {
  GNetXmlRpcClient *client = NULL;
  gchar *reply  = NULL;
  gchar *params = NULL;
  int   ret     = 0;

  if (!vs)
    return -1;

  params = g_strdup_printf("%d", vs->vserver_config->vserver_id);

  DP("Params = %s", params);

  if (params == NULL)
    return -1;

  client = gnet_xmlrpc_client_new(XMLSERVICE_HOST, XMLSERVICE_URL,
                                  XMLSERVICE_PORT);

  if (!client) {
    logmsg(RH_LOG_ERROR, "[%s] Could not connect to XML-RPC service", vs->vserver_config->vserver_name);
    ret = -1;
  }

  if (gnet_xmlrpc_client_call(client, "offacct", params, &reply) == 0)
    {
      DP("offacct reply = %s", reply);
      g_free(reply);
    }
  else
    {
      if (reply != NULL) {
        DP("offacct reply = %s", reply);
        g_free(reply);
      }

      DP("%s", "Failed executing offacct!");
      ret = -1;
    }

  gnet_xmlrpc_client_delete(client);  

  g_free(params);

  return 0;
}
