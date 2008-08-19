/* Example program for a client using the gnet_xmlrpc library.
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
#include "xmlrpc.h"

int
main(int argc, char *argv[])
{
  GNetXmlRpcClient *client;
  gchar *reply;
  gint port = 8123;

  if (argc > 1)
    port = atoi(argv[1]);

  gnet_init ();

  client = gnet_xmlrpc_client_new("localhost", "/RPC2", port);

  if (!client)
    {
      printf("Failed connecting to localhost!\n");
      exit(-1);
    }
  
  if (gnet_xmlrpc_client_call(client,
			      "echo",
			      "Hello world",
			      /* output  */
			      &reply) == 0)
    {
      printf("echo reply = %s\n", reply);
      g_free(reply);
    }
  else
    printf("Failed executing echo!\n");

  if (gnet_xmlrpc_client_call(client,
			      "async",
			      "",
			      // output
			      &reply) == 0)
    {
      printf("async reply = %s\n", reply);
      g_free(reply);
    }
  else
    printf("Failed executing echo!\n");

  return 0;
}
