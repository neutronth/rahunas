/**
 * RahuNAS XML-RPC Server implementation
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-08-07
 */

#include <stdio.h>
#include <stdlib.h>
#include "rahunasd.h"
#include "rh-xmlrpc-server.h"
#include "rh-ipset.h"
#include "rh-utils.h"

extern struct set *rahunas_set;
extern const char* termstring;

int do_startsession(GNetXmlRpcServer *server,
                    const gchar *command,
										const gchar *param,
										gpointer user_data,
										gchar **reply_string) 
{
  struct rahunas_map *map = (struct rahunas_map *)user_data;
	struct rahunas_member *members = NULL;
  unsigned char ethernet[ETH_ALEN] = {0,0,0,0,0,0};
	gchar *ip = NULL;
	gchar *username = NULL;
	gchar *session_id = NULL;
  gchar *mac_address = NULL;
	uint32_t id;
  int res = 0;

	if (!map)
	  goto out;

  if (!map->members)
    goto out;

  if (param == NULL)
    goto out;

	members = map->members;

 	ip          = rh_string_get_sep(param, "|", 1);
	username    = rh_string_get_sep(param, "|", 2);
	session_id  = rh_string_get_sep(param, "|", 3);
  mac_address = rh_string_get_sep(param, "|", 4);

  if (ip == NULL || username == NULL 
        || session_id == NULL || mac_address == NULL)
    goto out;

	id = iptoid(map, ip);

	if (id < 0) {
    *reply_string = g_strdup("Invalid IP Address");
		return 0;
	}

  res = set_adtip(rahunas_set, ip, mac_address, IP_SET_OP_ADD_IP);
  if (res == 0) {
    members[id].flags = 1;
    if (members[id].username && members[id].username != termstring)
      free(members[id].username);

    if (members[id].session_id && members[id].username != termstring)
      free(members[id].session_id);

    members[id].username   = strdup(username);
    if (!members[id].username)
      members[id].username = termstring;

    members[id].session_id = strdup(session_id);
    if (!members[id].session_id)
      members[id].session_id = termstring;

		time(&(members[id].session_start));

    parse_mac(mac_address, &ethernet);
    memcpy(&members[id].mac_address, &ethernet, ETH_ALEN);

    logmsg(RH_LOG_NORMAL, "Session Start, User: %s, IP: %s, "
                          "Session ID: %s, MAC: %s",
                          members[id].username, 
                          idtoip(map, id), 
                          members[id].session_id,
                          mac_tostring(members[id].mac_address)); 

  } else if (res == RH_IS_IN_SET) {
    members[id].flags = 1;
  }

  *reply_string = g_strdup_printf("Greeting! Got: IP %s, User %s, ID %s", 
	                                 ip, members[id].username, 
																	 members[id].session_id);
  g_free(ip);
	return 0;

out:
    *reply_string = g_strdup("Invalid input parameters");
    g_free(ip);
		return 0;
}

int do_stopsession(GNetXmlRpcServer *server,
                   const gchar *command,
									 const gchar *param,
									 gpointer user_data,
									 gchar **reply_string)
{
  struct rahunas_map *map = (struct rahunas_map *)user_data;
	struct rahunas_member *members;
	gchar *ip = NULL;
	gchar *mac_address = NULL;
	uint32_t   id;
  int res = 0;
  unsigned char ethernet[ETH_ALEN] = {0,0,0,0,0,0};

	if (!map)
	  goto out;

  if (!map->members)
    goto out;

	members = map->members;

	if (param == NULL)
	  goto out;

  DP("RPC Receive: %s", param);

  ip          = rh_string_get_sep(param, "|", 1);
  mac_address = rh_string_get_sep(param, "|", 2);

  if (ip == NULL || mac_address == NULL)
    goto out;

	id = iptoid(map, ip);

	if (id < 0) {
    *reply_string = g_strdup("Invalid IP Address");
		return 0;
	}

	if (members[id].flags) {
    parse_mac(mac_address, &ethernet);
    if (memcmp(&ethernet, &members[id].mac_address, ETH_ALEN) == 0) {
      res = set_adtip(rahunas_set, idtoip(map, id), mac_address,
                      IP_SET_OP_DEL_IP);
      if (res == 0) {
        if (!members[id].username)
          members[id].username = termstring;

        if (!members[id].session_id)
          members[id].session_id = termstring;

        logmsg(RH_LOG_NORMAL, "Session Stop, User: %s, IP: %s, "
                              "Session ID: %s, MAC: %s",
                              members[id].username, 
                              idtoip(map, id), 
                              members[id].session_id,
                              mac_tostring(members[id].mac_address)); 

        rh_free_member(&members[id]);   
 
        *reply_string = g_strdup_printf("Client IP %s was removed!", 
  			                                  idtoip(map, id));
  	    return 0;
      } else {
         *reply_string = g_strdup_printf("Client IP %s error remove!", 
  			                                idtoip(map, id));
  			  return 0;
      }
		}	
  }

  *reply_string = g_strdup_printf("%s", ip);
	return 0;

out:
    *reply_string = g_strdup("Invalid input parameters");
		return 0;
}

int do_getsessioninfo(GNetXmlRpcServer *server,
                      const gchar *command,
									    const gchar *param,
									    gpointer user_data,
									    gchar **reply_string)
{
  struct rahunas_map *map = (struct rahunas_map *)user_data;
	struct rahunas_member *members = NULL;
	gchar *ip = NULL;
	uint32_t   id;

	if (!map)
	  goto out;

  if (!map->members)
    goto out;

	members = map->members;

	if (param == NULL)
	  goto out;

  ip = param;
	id = iptoid(map, ip);

	if (id < 0) {
    *reply_string = g_strdup("Invalid IP Address");
		return 0;
	}

	if (members[id].flags) {
    if (!members[id].username) {
      *reply_string = g_strdup("Invalid Username");
      return 0;
    }

    if (!members[id].session_id) {
      *reply_string = g_strdup("Invalid Session ID");
      return 0;
    }
    
    *reply_string = g_strdup_printf("%s|%s|%s|%d|%s", 
                                      ip, 
		                                  members[id].username,
																		  members[id].session_id,
																		  members[id].session_start,
                                      mac_tostring(members[id].mac_address));
		return 0;
	}

  *reply_string = g_strdup_printf("%s", ip);
	return 0;

out:
    *reply_string = g_strdup("Invalid input parameters");
		return 0;
}
