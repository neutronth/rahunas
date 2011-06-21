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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gnet.h>
#include "xmlrpc.h"

typedef struct {
  GNetXmlRpcServer server; 
  GHashTable *command_hash;
} GNetXmlRpcServerPrivate;

typedef struct {
  GNetXmlRpcCommandCallback *callback;
  GNetXmlRpcCommandAsyncCallback *async_callback;
  gpointer user_data;
} command_hash_value_t;

typedef struct {
  GNetXmlRpcServerPrivate *xmlrpc_server;
  GString *connection_string;
} connection_state_t;

static void ob_server_func (GServer* server, GConn* conn, gpointer user_data);
static void ob_client_func (GConn* conn, GConnEvent* event, 
                            gpointer user_data);
gchar *create_response_string(const gchar *reply);
static GServer* ob_server = NULL;

int extract_xmlrpc_request(const gchar *str,
                           /* output */
                           gchar **command_name,
                           gchar **parameter
                           );

GNetXmlRpcServer *gnet_xmlrpc_server_new(const gchar* hostname, int port)
{
  GNetXmlRpcServerPrivate *xmlrpc_server = g_new0(GNetXmlRpcServerPrivate, 1);
  
  /* Create the server */
	GInetAddr* addr = gnet_inetaddr_new (hostname, port);
  GServer *gnet_server = gnet_server_new (addr, port, ob_server_func, xmlrpc_server);
	gnet_inetaddr_delete (addr);
  if (!gnet_server)
    {
      fprintf (stderr, "Error: Could not start server\n");
      return NULL;
    }
  xmlrpc_server->server.gnet_server = gnet_server;
  xmlrpc_server->command_hash = g_hash_table_new(g_str_hash,
                                                 g_str_equal);

  return (GNetXmlRpcServer*)xmlrpc_server;
}

void gnet_xmlrpc_server_delete(GNetXmlRpcServer *server)
{
  gnet_server_delete (ob_server);
}

static void
ob_server_func (GServer* server, GConn* conn, gpointer user_data)
{
  if (conn)
    {
      connection_state_t *conn_state = g_new0(connection_state_t, 1);
      conn_state->xmlrpc_server = (GNetXmlRpcServerPrivate*)user_data;
      conn_state->connection_string = g_string_new("");
      
      gnet_conn_set_callback (conn, ob_client_func, conn_state);
      gnet_conn_set_watch_error (conn, TRUE);
      gnet_conn_readline (conn);
    }
  else  /* Error */
    {
      gnet_server_delete (server);
      exit (EXIT_FAILURE);
    }
}


static void
ob_client_func (GConn* conn, GConnEvent* event, gpointer user_data)
{
  connection_state_t *conn_state = (connection_state_t*)user_data;
  GNetXmlRpcServerPrivate *xmlrpc_server = conn_state->xmlrpc_server;
  GString *req_string = conn_state->connection_string;

  switch (event->type)
    {
    case GNET_CONN_READ:
      {
        event->buffer[event->length-1] = '\n';
        g_string_append_len(req_string, event->buffer, event->length);

        /* Check if we have the whole request */
        if (g_strstr_len(req_string->str,
                         req_string->len,
                         "</methodCall>") != NULL)
          {
            /* Extract command name */
            gchar *command_name=NULL, *parameter=NULL;
            gchar *response=NULL;
            command_hash_value_t *command_val;
            
            extract_xmlrpc_request(req_string->str,
                                   /* output */
                                   &command_name,
                                   &parameter
                                   );

            /* Call the callback */
            command_val = g_hash_table_lookup(xmlrpc_server->command_hash,
                                              command_name);
            if (!command_val)
              response = create_response_string("No such method!");
            else if (command_val->async_callback)
              {
                (*command_val->async_callback)((GNetXmlRpcServer*)xmlrpc_server,
                                               conn,
                                               command_name,
                                               parameter,
                                               command_val->user_data);
              }
            else
              {
                gchar *reply;
                (*command_val->callback)((GNetXmlRpcServer*)xmlrpc_server,
                                         command_name,
                                         parameter,
                                         command_val->user_data,
                                         /* output */
                                         &reply);
                response = create_response_string(reply);
                g_free(reply);
              }

            if (response)
              {
                /* Send reply */
                gnet_conn_write (conn, response, strlen(response));
                g_free(response);
              }
            
            /* printf("rs = ((%s)) len=%d\n", req_string->str, event->length); */
            g_string_assign(conn_state->connection_string, "");

            g_free(command_name);
            if (parameter)
                g_free(parameter);
          }

        gnet_conn_readline (conn);
        break;
      }

    case GNET_CONN_WRITE:
      {
        ; /* Do nothing */
        break;
      }

    case GNET_CONN_CLOSE:
    case GNET_CONN_TIMEOUT:
    case GNET_CONN_ERROR:
      {
        g_string_free(conn_state->connection_string, TRUE);
        g_free(conn_state);
        gnet_conn_delete (conn);
        break;
      }

    default:
      g_assert_not_reached ();
    }
}

static int
gnet_xmlrpc_server_register_command_full(GNetXmlRpcServer *_xmlrpc_server,
                                         const gchar *command,
                                         GNetXmlRpcCommandCallback *callback,
                                         GNetXmlRpcCommandAsyncCallback *async_callback,
                                         gpointer user_data)
{
  GNetXmlRpcServerPrivate *xmlrpc_server = (GNetXmlRpcServerPrivate*)_xmlrpc_server;
  /* TBD - Check if command exist and override it */
  command_hash_value_t *val = g_new0(command_hash_value_t, 1);
  val->callback = callback;
  val->async_callback = async_callback;
  val->user_data = user_data;
  g_hash_table_insert(xmlrpc_server->command_hash,
                      g_strdup(command),
                      val);

  return 0;
}

int gnet_xmlrpc_server_register_command(GNetXmlRpcServer *_xmlrpc_server,
                                        const gchar *command,
                                        GNetXmlRpcCommandCallback *callback,
                                        gpointer user_data)
{
  gnet_xmlrpc_server_register_command_full(_xmlrpc_server,
                                           command,
                                           callback,
                                           NULL, /* async */
                                           user_data);

  return 0;
}

int gnet_xmlrpc_server_register_async_command(GNetXmlRpcServer *_xmlrpc_server,
                                              const gchar *command,
                                              GNetXmlRpcCommandAsyncCallback *async_callback,
                                              gpointer user_data)
{
  gnet_xmlrpc_server_register_command_full(_xmlrpc_server,
                                           command,
                                           NULL,
                                           async_callback, /* async */
                                           user_data);
  return 0;
}

/** 
 * Reply to an async request
 * 
 * @param gnet_client 
 * @param reply_string 
 * 
 * @return 
 */
int gnet_xmlrpc_async_client_response(GConn *conn,
                                      const gchar *reply)
{
  gchar *response = create_response_string(reply);
  /* Send reply */
  gnet_conn_write (conn, response, strlen(response));
  g_free(response);

  return 0;
}

/**
 * Extract the xmlrpc request. This is done by simple reg exp matching.
 * This should be changed to xml decoding
 */
int extract_xmlrpc_request(const gchar *str,
                           /* output */
                           gchar **command_name,
                           gchar **parameter
                           )
{
  int len = strlen(str);
  gchar *method_name_start, *method_name_end;
  gchar *param_name_start, *param_name_end;
  GString *param_string = g_string_new("");
  const gchar *p;
  
  method_name_start = g_strstr_len(str,
                                   len,
                                   "<methodName>");
  if (method_name_start == NULL)
    return -1;
  method_name_start += strlen("<methodName>");

  method_name_end = g_strstr_len(method_name_start,
                                 strlen(method_name_start),
                                 "</methodName>");
  if (method_name_end == NULL)
    return -1;

  *command_name = g_strndup(method_name_start,
                            method_name_end - method_name_start);
	
	param_name_start = g_strstr_len(str,
	                                len,
																	"<string>");
	if (param_name_start == NULL) {
    *parameter = NULL;
		return 0;
	}
  
	param_name_start += strlen("<string>");
	param_name_end = g_strstr_len(param_name_start,
	                              strlen(param_name_start),
																"</string>");

	if (param_name_end == NULL) {
    return -1;
	}

  /* Decode and build parameter */
  if (param_name_start) {
      p = param_name_start;
      while (*p)
        {
          gchar c = *p++;
          if (p > param_name_end)
            break;
    
          if (c == '&')
            {
              if (g_strstr_len(p, 4, "amp;") == p)
                {
                  g_string_append_c(param_string, '&');
                  p+= 4;
                }
              if (g_strstr_len(p, 5, "quot;") == p)
                {
                  g_string_append_c(param_string, '"');
                  p+= 5;
                }
              else if (g_strstr_len(p, 3, "lt;") == p)
                {
                  g_string_append_c(param_string, '<');
                  p+= 3;
                }
              else if (g_strstr_len(p, 3, "gt;") == p)
                {
                  g_string_append_c(param_string, '>');
                  p+= 3;
                }
              else
                {
                  /* Don't know what to do. Just add the ampersand.. */
                  g_string_append_c(param_string, '&');
                }
            }
          else
            g_string_append_c(param_string, c);
        }
      *parameter = param_string->str;
      g_string_free(param_string, FALSE);
  }
  else
      *parameter = NULL;
    
  return 0;
                                   
}

gchar *create_response_string(const gchar *reply)
{
  GString *response_string = g_string_new("");
  GString *content_string = g_string_new("");
  gchar *res;
  const gchar *p;

  g_string_append(content_string,
                  "<?xml version=\"1.0\"?> <methodResponse> <params> <param> <value><string>");

  /* Encode and add the reply */
  p=reply;
  while(*p) {
      gchar c = *p++;
      switch (c)
        {
        case '&' : g_string_append(content_string, "&amp;"); break;
        case '<': g_string_append(content_string, "&lt;"); break;
        case '>': g_string_append(content_string, "&gt;"); break;
        default:
          g_string_append_c(content_string, c);
        }
  }

  g_string_append(content_string,
                  "</string></value> </param> </params> </methodResponse>\n");


  g_string_append_printf(response_string,
                         "HTTP/1.1 200 OK\n"
                         "Connection: close\n"
                         "Content-Length: %" G_GSIZE_FORMAT "\n"
                         "Content-Type: text/xml\n"
                         "Server: RahuNASd XMLRPC server\n"
                         "\n"
                         "%s",
                         content_string->len,
                         content_string->str
                         );

  g_string_free(content_string, TRUE);

  res = response_string->str;

  g_string_free(response_string, FALSE);

  return res;
}

