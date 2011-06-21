/* An XMLRPC client and server library for GNet
 *
 * Copyright (c) 2006 Dov Grobgeld <dov.grobgeld@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef GNET_XMLRPC_H
#define GNET_XMLRPC_H

#include <gnet.h>

G_BEGIN_DECLS

/* GNetXmlRpcServer API */

typedef struct {
  /*< private >*/
  GServer *gnet_server;
} GNetXmlRpcServer; 

typedef gint (GNetXmlRpcCommandCallback) (GNetXmlRpcServer * server,
                                          const gchar      * command,
                                          const gchar      * param,
                                          gpointer           user_data,
                                          gchar           ** reply_string);

typedef int (GNetXmlRpcCommandAsyncCallback)(GNetXmlRpcServer *server,
                                             GConn *gnet_client,
                                             const gchar *command,
                                             const gchar *param,
                                             gpointer user_data);

GNetXmlRpcServer * gnet_xmlrpc_server_new    (const gchar *hostname, gint port);

void               gnet_xmlrpc_server_delete (GNetXmlRpcServer * server);

gint               gnet_xmlrpc_server_register_command (GNetXmlRpcServer          * server,
                                                        const gchar               * command,
                                                        GNetXmlRpcCommandCallback * callback,
                                                        gpointer                    user_data);
gint               gnet_xmlrpc_server_register_async_command (GNetXmlRpcServer               * server,
                                                              const gchar                    * command,
                                                              GNetXmlRpcCommandAsyncCallback * async_callback,
                                                              gpointer                         user_data);

/* GNetXmlRpcClient API */

typedef struct {
  /*< private >*/
  GIOChannel *gnet_channel;
} GNetXmlRpcClient;

GNetXmlRpcClient * gnet_xmlrpc_client_new    (const gchar      * hostname,
                                              const gchar      * url,
                                              gint               port);

void               gnet_xmlrpc_client_delete (GNetXmlRpcClient * client);

gint               gnet_xmlrpc_client_call   (GNetXmlRpcClient * client,
                                              const gchar      * method,
                                              const gchar      * param,
                                              gchar           ** reply);  /* output */

int                gnet_xmlrpc_async_client_response(GConn *gnet_client,
                                                     const gchar *reply);


G_END_DECLS

#endif /* GNET_XMLRPC */
