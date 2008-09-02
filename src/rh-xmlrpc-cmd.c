/**
 * XML-RPC client command sender
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-09-02
 */

#include "../xmlrpc/xmlrpc.h"
#include "rahunasd.h"
#include "rh-xmlrpc-cmd.h"

int send_xmlrpc_stopacct(struct rahunas_map *map, uint32_t id) {
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

  client = gnet_xmlrpc_client_new(XMLSERVICE_HOST, XMLSERVICE_URL, 
	                                XMLSERVICE_PORT);

  if (!client) {
    logmsg(RH_LOG_ERROR, "Could not connect to XML-RPC service");
    return (-1);
  }
	
	params = g_strdup_printf("%s|%s|%s|%d|%s", 
                           idtoip(map, id),
	                         members[id].username,
													 members[id].session_id,
													 members[id].session_start,
                           mac_tostring(members[id].mac_address));

  if (params == NULL)
    return (-1);
  
  if (gnet_xmlrpc_client_call(client, "stopacct", params, &reply) == 0)
    {
      DP("stopacct reply = %s", reply);
      g_free(reply);
    }
  else
    DP("%s", "Failed executing stopacct!");
	
	g_free(params);

  return 0;
}
