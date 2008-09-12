/**
 * RahuNAS XML-RPC Server implementation
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 *         Suriya Soutmun <darksolar@gmail.com>
 * Date:   2008-08-07
 */

#include <stdio.h>
#include <stdlib.h>
#include "rahunasd.h"
#include "rh-xmlrpc-server.h"
#include "rh-radius.h"
#include "rh-ipset.h"
#include "rh-utils.h"
#include "rh-task.h"

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
  struct task_req req;
	gchar *ip = NULL;
	gchar *username = NULL;
	gchar *session_id = NULL;
  gchar *mac_address = NULL;
	uint32_t id;

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
		goto cleanup;
	}

  req.id = id;
  req.username = username;
  req.session_id = session_id;
  parse_mac(mac_address, &ethernet);
  memcpy(req.mac_address, &ethernet, ETH_ALEN);

  rh_task_startsess(map, &req);

  *reply_string = g_strdup_printf("Greeting! Got: IP %s, User %s, ID %s", 
	                                 ip, members[id].username, 
																	 members[id].session_id);
  goto cleanup;

out:
    *reply_string = g_strdup("Invalid input parameters");
    goto cleanup;

cleanup:
  g_free(ip);
  g_free(username);
  g_free(session_id);
  g_free(mac_address);
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
  struct task_req req;
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
    goto cleanup;
	}

  req.id = id;
  parse_mac(mac_address, &ethernet);
  memcpy(req.mac_address, &ethernet, ETH_ALEN);
  req.req_opt = RH_RADIUS_TERM_USER_REQUEST;

	if (members[id].flags) {
    if (memcmp(&ethernet, &members[id].mac_address, ETH_ALEN) == 0) {
      res = rh_task_stopsess(map, &req);
      if (res == 0) {
        *reply_string = g_strdup_printf("Client IP %s was removed!", 
  			                                  idtoip(map, id));
      } else {
         *reply_string = g_strdup_printf("Client IP %s error remove!", 
  			                                idtoip(map, id));
      }
      goto cleanup;
		}	
  }

  *reply_string = g_strdup_printf("%s", ip);
  goto cleanup;

out:
  *reply_string = g_strdup("Invalid input parameters");
  goto cleanup;

cleanup:
  g_free(ip);
  g_free(mac_address);
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
