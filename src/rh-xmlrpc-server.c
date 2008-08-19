/**
 * RahuNAS XML-RPC Server implementation
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-08-07
 */

#include <stdio.h>
#include <stdlib.h>
#include "rahunasd.h"
#include "rh-xmlrpc-server.h"
#include "ipset-control.h"

int do_startsession(GNetXmlRpcServer *server,
                    const gchar *command,
										const gchar *param,
										gpointer user_data,
										gchar **reply_string) 
{
  struct rahunas_map *map = (struct rahunas_map *)user_data;
	struct rahunas_member *members = NULL;
	gchar *ip = NULL;
	gchar *username = NULL;
	gchar *session_id = NULL;
	gchar *pStart = NULL;
	gchar *pEnd = NULL;
	uint32_t id;

	if (!map)
	  goto out;

  if (!map->members)
    goto out;

  if (param == NULL)
    goto out;

	members = map->members;

  pStart = param;
	pEnd = g_strstr_len(pStart, strlen(pStart), "|");
	if (pEnd == NULL) {
	  goto out;
	}

	ip = g_strndup(pStart, pEnd - pStart);
  if (ip == NULL)
    goto out;

	pStart = pEnd + 1;
	pEnd = g_strstr_len(pStart, strlen(pStart), "|");
	if (pEnd == NULL) {
    goto out;
	}
	username = g_strndup(pStart, pEnd - pStart);
	pStart = pEnd + 1;
  session_id = g_strndup(pStart, strlen(pStart));

	id = iptoid(map, ip);

	if (id < 0) {
    *reply_string = g_strdup("Invalid IP Address");
		return 0;
	}

	if (members[id].flags) {
    *reply_string = g_strdup("Client already login");
		return 0;
	}

  if (ctrl_add_to_set(map, id) == 0) {
	  if (!members[id].username)
		  free(members[id].username);
		members[id].username = username;
    
		if (!members[id].session_id)
		  free(members[id].session_id);
		members[id].session_id = session_id;

		time(&(members[id].session_start));

    logmsg(RH_LOG_NORMAL, "Session Start, User: %s, IP: %s, "
                          "Session ID: %s",
                          members[id].username, 
                          idtoip(map, id), 
                          members[id].session_id); 
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
	gchar *ip;
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
	  if (ctrl_del_from_set(map, id) == 0) {

    logmsg(RH_LOG_NORMAL, "Session Stop, User: %s, IP: %s, "
                          "Session ID: %s",
                          members[id].username, 
                          idtoip(map, id), 
                          members[id].session_id); 

      if (members[id].username)
        free(members[id].username);
         
      if (members[id].session_id)
        free(members[id].session_id);

	    memset(&members[id], 0, sizeof(struct rahunas_member));
			*reply_string = g_strdup_printf("Client IP %s was removed!", 
			                                idtoip(map, id));
			return 0;
		} else {
      *reply_string = g_strdup_printf("Client IP %s remove error", ip);
		  return 0;
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
    
    *reply_string = g_strdup_printf("%s|%s|%s|%d", ip, 
		                                               members[id].username,
																								   members[id].session_id,
																								   members[id].session_start);
		return 0;
	}

  *reply_string = g_strdup_printf("%s", ip);
	return 0;

out:
    *reply_string = g_strdup("Invalid input parameters");
		return 0;
}
