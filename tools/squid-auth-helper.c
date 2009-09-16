/**
 * Squid external authentication helper
 * Author: Theppitak Karoonboonyanan <thep@linux.thai.net>
 *         Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2009-08-31
 * License: GPL-2+
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <syslog.h>
#include <ftw.h>
#include <lcfg_static.h>
#include <arpa/inet.h>
#include <xmlrpc.h>

struct vserver_info
{
  guint    id;
  guint32  net_ip;
  gushort  mask;
};

GList *vservers = 0;

enum lcfg_status
lcfg_visitor_read_vserver (const char *key, void *data, size_t size,
                           void *user_data)
{
  struct vserver_info *info = (struct vserver_info *) user_data;
  const char *value = (const char *) data;

  key = strchr (key, '.');
  ++key;
  if (strcmp (key, "vserver_id") == 0)  
    {
      info->id = atoi (value);
    }
  else if (strcmp (key, "clients") == 0)
    {
      const char *tok_end;
      char *ip_val;
      struct in_addr ip_addr;

      tok_end = strchr (value, '/');
      if (!tok_end)
        {
          /* log error ("Invalid net addr format '%s'", value) */
          return lcfg_status_error;
        }
      ip_val = strndup (value, tok_end - value);
      if (inet_aton (ip_val, &ip_addr) == 0)
        {
          /* log error ("Invalid net addr format '%s'", value) */
          return lcfg_status_error;
        }
      free (ip_val);
      info->net_ip = ntohl (ip_addr.s_addr);
      info->mask = atoi (tok_end + 1);
    }

  return lcfg_status_ok;
}

int ftw_read_vserver (const char *fpath, const struct stat *sb, int typeflag)
{
  struct lcfg *cfg;
  struct vserver_info *info;

  if (typeflag != FTW_F)
    return 0;

  cfg = lcfg_new (fpath);
  if (lcfg_parse (cfg) != lcfg_status_ok)
    {
      syslog (LOG_ERR, "config error: %s", lcfg_error_get (cfg));
      lcfg_delete (cfg);
      return -1;
    }

  info = (struct vserver_info *) malloc (sizeof (struct vserver_info));
  if (!info)
    {
      syslog (LOG_ERR, "config error: out of memory");
      lcfg_delete (cfg);
      return -1;
    }
  if (lcfg_accept (cfg, lcfg_visitor_read_vserver, info)
      != lcfg_status_ok)
    {
      syslog (LOG_ERR, "config error: %s", lcfg_error_get (cfg));
      lcfg_delete (cfg);
      return -1;
    }
  vservers = g_list_append (vservers, (gpointer) info);

  lcfg_delete (cfg);

  return 0;
}

int
main(int argc, char *argv[])
{
  GNetXmlRpcClient *client;
  gint port = 8123;
  gchar line[1024];
  GList *vs;

  gnet_init ();

  /* read vserver config */
  ftw ("/etc/rahunas/rahunas.d", ftw_read_vserver, 20);

  client = gnet_xmlrpc_client_new ("localhost", "/RPC2", port);
  if (!client)
    {
      printf ("ERR message=\"Failed connecting to localhost XMLRPC\"");
      fflush (stdout);
      goto exit_1;
    }
  
  /* read input from squid */
  while (fgets (line, sizeof line, stdin))
    {
      int      len;
      struct in_addr ip_addr;
      guint32  src_ip;
      guint    vserver_id;
      gchar    query[32];
      gchar   *reply;

      len = strlen (line);
      if (line[len] == '\n')
        line[len--] = '\0';

      if (inet_aton (line, &ip_addr) == 0)
        {
          printf ("ERR message=\"Invalid source IP address '%s'\"", line);
          fflush (stdout);
          continue;
        }
      src_ip = ntohl (ip_addr.s_addr);

      vserver_id = ~0;
      for (vs = vservers; vs; vs = vs->next)
        {
          struct vserver_info *info = (struct vserver_info *) vs->data;
          if ((src_ip & (~0 << (32 - info->mask))) == info->net_ip)
            {
              vserver_id = info->id;
              break;
            }
        }

      if (vserver_id == ~0)
        {
          printf ("ERR message=\"vserver_id not found for '%s'", line);
          fflush (stdout);
          continue;
        }

      /* query RahuNAS daemon for user name via xmlrpc */
      snprintf (query, sizeof query, "%s|%d", line, vserver_id);
      if (gnet_xmlrpc_client_call (client,
			           "getsessioninfo",
			           query,
			           /* output  */
			           &reply) == 0)
        {
          gchar *tok;
          /* reply format: id|user|sess-id|sess-start|mac|sess-timeout */
          tok = strtok (reply, "|"); /* id */
          tok = strtok (NULL, "|"); /* user */
          printf ("OK user=%s\n", tok);
          fflush (stdout);
          g_free (reply);
        }
    }

  gnet_xmlrpc_client_delete (client);

exit_1:
  for (vs = vservers; vs; vs = vs->next)
    free (vs->data);
  g_list_free (vservers);

  return 0;
}
