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
#include <semaphore.h>
#include <pthread.h>

struct vserver_info
{
  guint    id;
  guint32  net_ip;
  gushort  mask;
};

struct helper_opts
{
  gchar   ch_tag[16];
  gchar   ip[40];
};

typedef struct helper_opts HelperOpts;

GList *vservers = 0;

pthread_mutex_t helper_mtx = PTHREAD_MUTEX_INITIALIZER;
sem_t           helper_sem;

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

static void *
service (void *args)
{
  HelperOpts *opts = (HelperOpts *) args;
  GList   *vs;
  struct in_addr ip_addr;
  guint32  src_ip;
  guint    vserver_id;
  gchar    query[32];
  gchar   *reply;
  GNetXmlRpcClient *client;
  gint port = 8123;

  if (inet_aton (opts->ip, &ip_addr) == 0)
    {
      /* ERR: Invalid source IP address */
      pthread_mutex_lock (&helper_mtx);
      printf ("%sOK user=%s\n", opts->ch_tag, opts->ip);
      fflush (stdout);
      pthread_mutex_unlock (&helper_mtx);
      goto out;
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
      /* ERR: vserver_id not found */
      pthread_mutex_lock (&helper_mtx);
      printf ("%sOK user=%s\n", opts->ch_tag, opts->ip);
      fflush (stdout);
      pthread_mutex_unlock (&helper_mtx);
      goto out;
    }

  /* query RahuNAS daemon for user name via xmlrpc */
  client = gnet_xmlrpc_client_new ("localhost", "/RPC2", port);
  if (!client)
    {
      /* ERR: Failed to connect to localhost XMLRPC */
      pthread_mutex_lock (&helper_mtx);
      printf ("%sOK user=%s\n", opts->ch_tag, opts->ip);
      fflush (stdout);
      pthread_mutex_unlock (&helper_mtx);
      goto out;
    }

  snprintf (query, sizeof query, "%s|%d", opts->ip, vserver_id);
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
      pthread_mutex_lock (&helper_mtx);
      printf ("%sOK user=%s\n", opts->ch_tag, tok);
      fflush (stdout);
      pthread_mutex_unlock (&helper_mtx);
      g_free (reply);
    }
  else
    {
      /* ERR: Error calling 'getsessioninfo' XMLRPC */
      pthread_mutex_lock (&helper_mtx);
      printf ("%sOK user=%s\n", opts->ch_tag, opts->ip);
      fflush (stdout);
      pthread_mutex_unlock (&helper_mtx);
    }

  gnet_xmlrpc_client_delete (client);

out:
  sem_post (&helper_sem);
  pthread_exit (NULL);
}

int
main(int argc, char *argv[])
{
  GList *vs;
  gchar line[1024];
  int   max_concurrent = 8;
  HelperOpts rr_opts[8];
  int   rr_idx = 0;

  gnet_init ();

  /* read vserver config */
  ftw ("/etc/rahunas/rahunas.d", ftw_read_vserver, 20);

  /* Init semaphore */
  sem_init (&helper_sem, 0, max_concurrent);

  /* Init helper options */
  memset (&rr_opts, '\0', sizeof (rr_opts));

  /* read input from squid */
  while (fgets (line, sizeof (line), stdin))
    {
      pthread_t p;
      int       len;
      gchar    *sep = NULL;

      line[sizeof (line) - 1] = '\0';
      len = strlen (line);
      if (line[len - 1] == '\n')
        line[--len] = '\0';

      sep = strchr (line, ' ');
      if (sep)
        {
          *sep = '\0';
          strncpy (rr_opts[rr_idx].ch_tag, line,
                     sizeof (rr_opts[rr_idx].ch_tag) - 2);
          strcat (rr_opts[rr_idx].ch_tag, " ");

          /* Strip white-space */
          sep++;
          while (*sep == ' ' && (sep - line) < sizeof (line))
            sep++;

          if ((sep - line) < sizeof (line))
            {
              strncpy (rr_opts[rr_idx].ip, sep,
                         sizeof (rr_opts[rr_idx].ip) - 1);
            }
          else
            {
              rr_opts[rr_idx].ip[0] = '\0';
            }
        }
      else
        {
          rr_opts[rr_idx].ch_tag[0] = '\0';
          strncpy (rr_opts[rr_idx].ip, line, sizeof (rr_opts[rr_idx].ip) - 1);
        }

      sem_wait (&helper_sem);

      if (pthread_create (&p, NULL, (void *(*)(void *))service,
            (void *)&rr_opts[rr_idx]) != 0)
        {
          pthread_mutex_lock (&helper_mtx);
          printf ("%sOK user=%s\n", rr_opts[rr_idx].ch_tag, rr_opts[rr_idx].ip);
          fflush (stdout);
          pthread_mutex_unlock (&helper_mtx);
        }
      else
        {
          pthread_detach (p);
        }

      rr_idx++;
      rr_idx = rr_idx < max_concurrent ? rr_idx : 0;
    }

  for (vs = vservers; vs; vs = vs->next)
    free (vs->data);
  g_list_free (vservers);

  return 0;
}
