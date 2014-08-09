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
#include <string.h>
#include <stdlib.h>
#include "xmlrpc.h"

typedef struct  {
  GNetXmlRpcClient client;
	GString *url;
  GTcpSocket* socket;
  GString *xmlrpc_string;
  gboolean in_header;
  gint header_lineno;
  gint message_size;
  gint error;
} GNetXmlRpcClientPrivate;

static gchar *build_xmlrpc_message(GNetXmlRpcClient *_client,
                                   const gchar *method,
                                   const gchar *param);

GNetXmlRpcClient *gnet_xmlrpc_client_new(const gchar *hostname,
                                         const gchar *url,
                                         int port)
{
  GNetXmlRpcClientPrivate *client = g_new0(GNetXmlRpcClientPrivate, 1);
  GInetAddr* addr;

  addr = gnet_inetaddr_new (hostname, port);
  if (!addr)
    {
      fprintf (stderr, "Error: Name lookup for %s failed\n", hostname);
      return NULL;
    }

  /* Create the socket */
  client->socket = gnet_tcp_socket_new (addr);
  gnet_inetaddr_delete (addr);
  if (!client->socket)
    {
      g_free (client);
      fprintf (stderr, "Error: Could not connect to %s:%d\n", hostname, port);
      return NULL;
    }
  
  client->client.gnet_channel = gnet_tcp_socket_get_io_channel (client->socket);
  g_assert (client->client.gnet_channel != NULL);

	client->url = g_string_new("");
	g_string_append_len (client->url, url, strlen (url));

  client->xmlrpc_string = g_string_new("");
  client->in_header = TRUE;
  client->header_lineno = 0;
  client->message_size = 0;
  client->error = 0;

  return (GNetXmlRpcClient*)client;
}

void gnet_xmlrpc_client_delete(GNetXmlRpcClient *_client)
{
  GNetXmlRpcClientPrivate *client = (GNetXmlRpcClientPrivate*)_client;

  g_string_free(client->xmlrpc_string, TRUE);
	g_string_free(client->url, TRUE);
  gnet_tcp_socket_delete (client->socket);
  g_free(client);
}

int gnet_xmlrpc_client_call(GNetXmlRpcClient *_client,
                            const gchar *method,
                            const gchar *param,
                            // output
                            gchar **reply)
{
  GNetXmlRpcClientPrivate *client = (GNetXmlRpcClientPrivate*)_client;
  gchar *xmlrpc_message = build_xmlrpc_message((GNetXmlRpcClient*)client, 
	                                              method, param);
  gchar *msg_start, *msg_end;
  gchar *p;
  GString *xmlrpc_string = g_string_new("");
  GString *reply_string = g_string_new("");
  gsize n;
  gchar buffer[1024];
  gint error;
  
  n = strlen(xmlrpc_message);
  //  printf("Writing...\n"); fflush(stdout);
  error = gnet_io_channel_writen (client->client.gnet_channel,
                                  xmlrpc_message, n, &n);
  // printf("error = %d\n", error); fflush(stdout);
  if (error != G_IO_ERROR_NONE)
      return -1;

  // fprintf(stderr, "entering while loop\n");
  while (gnet_io_channel_readline (client->client.gnet_channel, buffer, sizeof(buffer), &n) == G_IO_ERROR_NONE)
    {
      if (client->in_header)
        {
          // Check for a valid response
          if (client->header_lineno == 0)
            {
              // If we don't have HTTP we've got a problem
              if (g_strstr_len(buffer, 5, "HTTP") == NULL)
                {
                  return -1;
                }
            }
          else if (n < 5) // Assume that empty line the line less than 5 chars
            {
              client->in_header = FALSE;
            }
          else
            {
              // Look for the content-length string case independant.
              char *p;

              // Lower case buf
              p = buffer;
              while(*p)
                {
                  *p = g_ascii_tolower(*p);
                  p++;
                }
              
              // Search for string
              if ((p = g_strstr_len(buffer, n-1,
                                    "content-length:")) != NULL)
                {
                  p += strlen("Content-length:");
                  client->message_size = atoi(p);
                }
            }
          client->header_lineno++;
        }
      // If we are not in the header then append the line to the xmlrpc string.
      else
        {
          g_string_append_len(xmlrpc_string, buffer, n-1);
          g_string_append_c(xmlrpc_string, '\n');
        }

      // Check if we are finished
      if (xmlrpc_string->len
          && xmlrpc_string->len >= client->message_size)
        {
          // Quit and reset parsing for next message
          client->in_header = 1;
          client->header_lineno = 0;
          break;
        }
    }

  // Extract the response. Should be exchanged to some more robust
  // XML parsing.
  msg_start = g_strstr_len(xmlrpc_string->str,
                           xmlrpc_string->len,
                           "<string>");
  if (msg_start == NULL)
    return -1;
  
  msg_start += strlen("<string>");
  msg_end = g_strrstr(msg_start, "</string>");

  // Decode the response
  p = msg_start;
  while(*p != '<')
    {
      gchar c = *p++;

      if (c == '&')
        {
          if (g_strstr_len(p, 4, "amp;") == p)
            {
              g_string_append_c(reply_string, '&');
              p+= 4;
            }
          else if (g_strstr_len(p, 3, "lt;") == p)
            {
              g_string_append_c(reply_string, '<');
              p+= 3;
            }
          else if (g_strstr_len(p, 3, "gt;") == p)
            {
              g_string_append_c(reply_string, '>');
              p+= 3;
            }
          else
            {
              // Don't know what to do. Just add the ampersand..
              g_string_append_c(reply_string, '&');
            }
        }
      else
        g_string_append_c(reply_string, c);
    }
  *reply = reply_string->str;
  g_string_free(reply_string, FALSE);
  g_string_free(xmlrpc_string, TRUE);

  return 0;
}
  
static gchar *build_xmlrpc_message(GNetXmlRpcClient *_client,
                                   const gchar *method,
                                   const gchar *param)
{
  GNetXmlRpcClientPrivate *client = (GNetXmlRpcClientPrivate*)_client;
  GString *xmlrpc_msg = g_string_new("");
  GString *req_string = g_string_new("");
  const gchar *p;
  gchar *ret;
  
  g_string_append_printf(xmlrpc_msg,
                         "<?xml version=\"1.0\"?>\n"
                         "<methodCall>\n"
                         "<methodName>%s</methodName>\n"
                         "<params>\n"
                         "<param><value><string>",
                         method);

  // Encode the param string
  p = param;
  while(*p)
    {
      gchar c = *p++;
      switch (c)
        {
        case '&' : g_string_append(xmlrpc_msg, "&amp;"); break;
        case '<': g_string_append(xmlrpc_msg, "&lt;"); break;
        case '>': g_string_append(xmlrpc_msg, "&gt;"); break;
        default:
          g_string_append_c(xmlrpc_msg, c);
        }
    }
  
  g_string_append(xmlrpc_msg,
                  "</string></value></param>\n"
                  "</params>\n"
                  "</methodCall>\n");
  
  g_string_append_printf(req_string,
                         "POST %s HTTP/1.0\n"
                         "User-Agent: RahuNASd/0.1\n"
                         "Host: localhost\n"
                         "Content-Type: text/xml\n"
                         "Content-length: %u\n\n",
												 client->url->str,
                         (guint) xmlrpc_msg->len);
  g_string_append_len(req_string,
                      xmlrpc_msg->str,
                      xmlrpc_msg->len);
  g_string_free(xmlrpc_msg, TRUE);

  ret = req_string->str;
  g_string_free(req_string, FALSE);

  return ret;
}
