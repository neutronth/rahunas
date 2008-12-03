/**
 * XML-RPC client command sender
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-09-02
 */

#include "../xmlrpc/xmlrpc.h"
#include "rahunasd.h"
#include "rh-xmlrpc-cmd.h"

int send_xmlrpc_stopacct(struct rahunas_map *map, uint32_t id, int cause) {
  struct rahunas_member *members = NULL;
  GNetXmlRpcClient *client = NULL;
  gchar *reply  = NULL;
  gchar *params = NULL;

  if (!map)
    return (-1);

  if (!map->members)
    return (-1);

  if (id < 0 || id > (map->size - 1))
    return (-1);
  
  members = map->members;

  params = g_strdup_printf("%s|%s|%s|%d|%s|%d", 
                           idtoip(map, id),
                           members[id].username,
                           members[id].session_id,
                           members[id].session_start,
                           mac_tostring(members[id].mac_address),
                           cause);

  if (params == NULL)
    return (-1);

  client = gnet_xmlrpc_client_new(rh_config.xml_serv_host, 
                                  rh_config.xml_serv_url, 
                                  rh_config.xml_serv_port);

  if (!client) {
    logmsg(RH_LOG_ERROR, "Could not connect to XML-RPC service");
    return (-1);
  }
  
  if (gnet_xmlrpc_client_call(client, "stopacct", params, &reply) == 0)
    {
      DP("stopacct reply = %s", reply);
      g_free(reply);
    }
  else
    {
      DP("%s", "Failed executing stopacct!");
      return (-1);
    }

  gnet_xmlrpc_client_delete(client);  

  g_free(params);

  return 0;
}
