/**
 * RahuNAS XML-RPC Server implementation header
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-08-07
 */
#ifndef __RH_XMLRPC_SERVER_H
#define __RH_XMLRPC_SERVER_H

#include "../xmlrpc/xmlrpc.h"

int do_startsession(GNetXmlRpcServer *server,
                    const gchar *command,
                    const gchar *param,
                    gpointer user_data,
                    gchar **reply_string);

int do_stopsession(GNetXmlRpcServer *server,
                   const gchar *command,
                   const gchar *param,
                   gpointer user_data,
                   gchar **reply_string);

int do_getsessioninfo(GNetXmlRpcServer *server,
                      const gchar *command,
                      const gchar *param,
                      gpointer user_data,
                      gchar **reply_string);

int do_roaming(GNetXmlRpcServer *server,
               const gchar *command,
               const gchar *param,
               gpointer user_data,
               gchar **reply_string);

#endif // __RH_XMLRPC_SERVER_H
