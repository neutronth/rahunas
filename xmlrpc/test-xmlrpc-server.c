/* Example program for the gnet_xmlrpc server functionality.
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
 */
#include <stdio.h>
#include <stdlib.h>
#include "xmlrpc.h"
 
int do_foo(GNetXmlRpcServer *server,
	   const gchar *command,
	   const gchar *param,
	   gpointer user_data,
	   /* output */
	   gchar **reply_string);
int do_echo(GNetXmlRpcServer *server,
	    const gchar *command,
	    const gchar *param,
	    gpointer user_data,
	    /* output */
	    gchar **reply_string);

int do_async(GNetXmlRpcServer *server,
             GConn *conn,
             const gchar *command,
             const gchar *param,
             gpointer user_data);

gboolean say_hello(gpointer user_data);

int
main(int argc, char** argv)
{
  int port = 8123;
	gchar* addr = "localhost";
  GNetXmlRpcServer *server;
  GMainLoop* main_loop;

  gnet_init ();

  if (argc > 1)
      port = atoi(argv[1]);

  /* Create the main loop */
  main_loop = g_main_loop_new (NULL,
                               FALSE);

  server = gnet_xmlrpc_server_new(addr, port);

  if (!server)
    {
      fprintf (stderr, "Error: Could not start server\n");
      exit (EXIT_FAILURE);
    }

  gnet_xmlrpc_server_register_command(server,
				      "foo",
				      do_foo,
				      NULL);

  gnet_xmlrpc_server_register_command(server,
				      "echo",
				      do_echo,
				      NULL);
  
  gnet_xmlrpc_server_register_async_command(server,
				      "async",
				      do_async,
				      NULL);

  /* Start the main loop */
  g_main_loop_run(main_loop);

  exit (EXIT_SUCCESS);
  return 0;
}

int do_foo(GNetXmlRpcServer *server,
	   const gchar *command,
	   const gchar *param,
	   gpointer user_data,
	   // output
	   gchar **reply_string)
{
    *reply_string = g_strdup("BAR!");
    return 0;
}

int do_echo(GNetXmlRpcServer *server,
	    const gchar *command,
	    const gchar *param,
	    gpointer user_data,
	    // output
	    gchar **reply_string)
{
    *reply_string = g_strdup_printf("You said: \"%s\"", param);

    return 0;
}

int do_async(GNetXmlRpcServer *server,
             GConn *conn,
             const gchar *command,
             const gchar *param,
             gpointer user_data)
{
  g_timeout_add(2000,
                say_hello,
                conn);
  return 0;
}

gboolean say_hello(gpointer user_data)
{
  GConn *conn = (GConn*)user_data;
  gnet_xmlrpc_async_client_response(conn, "hi!!");
  return FALSE;
}
