/**
 * RahuNAS server
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2009-03-24
 */
#include <stdio.h>
#include <string.h>
#include "rahunasd.h"
#include "rh-server.h"
#include "rh-utils.h"

RHVServer *
vserver_exists (GList *vserver_list, int vserver_id, const char *vserver_name)
{
  GList *runner = g_list_first(vserver_list);
  RHVServer *lvserver = NULL;

  while (runner != NULL) {
    lvserver = (RHVServer *)runner->data;
   
    if (lvserver->vserver_config->vserver_id == vserver_id)
      return lvserver; 

    if (strcmp(lvserver->vserver_config->vserver_name, vserver_name) == 0)
      return lvserver;

    runner = g_list_next(runner); 
  } 
  return NULL;
}

RHVServer *vserver_get_by_id(RHMainServer *ms, int search_id)
{
  GList *runner = g_list_first(ms->vserver_list);
  RHVServer *lvserver = NULL;

  while (runner != NULL) {
    lvserver = (RHVServer *)runner->data;
   
    if (lvserver->vserver_config->vserver_id == search_id) {
      return lvserver;
    }

    runner = g_list_next(runner); 
  } 
  return NULL;
}

int vserver_cleanup(RHVServer *vs)
{
  if (vs == NULL)
    return 0;

  if (vs->vserver_config != NULL)
    cleanup_vserver_config(vs->vserver_config);  

  return 0;
}

int mainserver_cleanup(RHMainServer *ms)
{
  if (ms == NULL)
    return 0;

  if (ms->main_config != NULL)
    cleanup_mainserver_config(ms->main_config);  

  return 0;
}

int register_vserver(RHMainServer *ms, const char *vserver_cfg_file)
{
  GList *vserver_list = ms->vserver_list;
  GList *chk = NULL;
  GList *node = NULL;
  FILE  *cfg_file = NULL;
  
  union rahunas_config *cfg_get = NULL;
  struct rahunas_vserver_config *vserver_config = NULL;
  RHVServer *new_vserver = NULL;
  RHVServer *old_vserver = NULL;

  union rahunas_config config = {
    .rh_vserver.vserver_name = NULL,
    .rh_vserver.vserver_id = VSERVER_ID,
    .rh_vserver.init_flag = VS_INIT,
    .rh_vserver.dev_external = NULL,
    .rh_vserver.dev_internal = NULL,
    .rh_vserver.vlan = NULL,
    .rh_vserver.vlan_raw_dev_external = NULL,
    .rh_vserver.vlan_raw_dev_internal = NULL,
    .rh_vserver.bridge = NULL,
    .rh_vserver.masquerade = NULL,
    .rh_vserver.ignore_mac = NULL,
    .rh_vserver.vserver_ip = NULL, 
    .rh_vserver.vserver_fqdn = NULL,
    .rh_vserver.vserver_ports_allow = NULL,
    .rh_vserver.vserver_ports_intercept = NULL,
    .rh_vserver.clients = NULL,
    .rh_vserver.excluded = NULL,
    .rh_vserver.idle_timeout = IDLE_TIMEOUT,
    .rh_vserver.dns = NULL,
    .rh_vserver.ssh = NULL,
    .rh_vserver.proxy = NULL,
    .rh_vserver.proxy_host = NULL,
    .rh_vserver.proxy_port = NULL,
    .rh_vserver.bittorrent = NULL,
    .rh_vserver.bittorrent_allow = NULL,
    .rh_vserver.radius_host = NULL,
    .rh_vserver.radius_secret = NULL,
    .rh_vserver.radius_encrypt = NULL,
    .rh_vserver.radius_auth_port = NULL,
    .rh_vserver.radius_account_port = NULL,
    .rh_vserver.nas_identifier = NULL,
    .rh_vserver.nas_port = NULL,
    .rh_vserver.nas_login_title = NULL,
    .rh_vserver.nas_default_redirect = NULL,
    .rh_vserver.nas_default_language = NULL,
    .rh_vserver.nas_weblogin_template = NULL,
  };

  cfg_file = fopen(vserver_cfg_file, "r");
  if (cfg_file == NULL)
    return -1;

  if (get_config(vserver_cfg_file, &config) == 0) {
    old_vserver = vserver_exists(vserver_list, config.rh_vserver.vserver_id, 
                    config.rh_vserver.vserver_name);

    if (old_vserver != NULL) {
      if (old_vserver->dummy_config != NULL) {
        DP("Cleanup old dummy config");
        rh_free((void **) &old_vserver->dummy_config);
      }

      old_vserver->dummy_config = 
        (struct rahunas_vserver_config *) rh_malloc(sizeof(struct rahunas_vserver_config));

      if (old_vserver->dummy_config == NULL)
        return -1;

      memset(old_vserver->dummy_config, 0, 
        sizeof(struct rahunas_vserver_config));
      memcpy(old_vserver->dummy_config, &config, 
        sizeof(struct rahunas_vserver_config));

      if (strncmp(config.rh_vserver.vserver_name, 
            old_vserver->vserver_config->vserver_name, 32) != 0 || 
          strncmp(config.rh_vserver.clients, 
            old_vserver->vserver_config->clients, 18) != 0) {
        old_vserver->vserver_config->init_flag = VS_RESET;
      } else {
        old_vserver->vserver_config->init_flag = VS_RELOAD;  
      }
      return 1;
    }
  } else {
    return -1;
  }

  vserver_config = (struct rahunas_vserver_config *) rh_malloc(sizeof(struct rahunas_vserver_config));

  if (vserver_config == NULL)
    return -1;

  memset(vserver_config, 0, sizeof(struct rahunas_vserver_config));

  memcpy(vserver_config, &config, sizeof(struct rahunas_vserver_config));

  new_vserver = (RHVServer *) rh_malloc(sizeof(RHVServer));

  if (new_vserver == NULL)
    return -1;

  memset(new_vserver, 0, sizeof(RHVServer));

  new_vserver->vserver_config = vserver_config;

  new_vserver->main_server = ms;
  new_vserver->vserver_config->init_flag = VS_INIT;
  ms->vserver_list = g_list_append(ms->vserver_list, new_vserver);
  return 0; 
}

int unregister_vserver(RHMainServer *ms, int vserver_id)
{
  GList *vserver_list = ms->vserver_list;
  GList *runner = g_list_first(vserver_list);
  RHVServer *lvserver = NULL;

  while (runner != NULL) {
    lvserver = (RHVServer *)runner->data;
    if (lvserver->vserver_config->vserver_id == vserver_id) {
      vserver_cleanup(lvserver);

      ms->vserver_list = g_list_delete_link(ms->vserver_list, runner);
      break;
    } else {
      runner = g_list_next(runner);
    }
  }
  return 0;
}

int unregister_vserver_all(RHMainServer *ms)
{
  GList *vserver_list = ms->vserver_list;
  GList *runner = g_list_first(vserver_list);
  GList *deleting = NULL;
  RHVServer *lvserver = NULL;

  while (runner != NULL) {
    lvserver = (RHVServer *)runner->data;
    vserver_cleanup(lvserver);
    deleting = runner;
    runner = g_list_next(runner);

    cleanup_vserver_config(lvserver->vserver_config);
    ms->vserver_list = g_list_delete_link(ms->vserver_list, deleting);
  }
  
  return 0;
}

int walk_through_vserver(int (*callback)(RHMainServer *, RHVServer*),
                         RHMainServer *ms)
{
  GList *vserver_list = ms->vserver_list;
  GList *runner = g_list_first(vserver_list);
  RHVServer *vs = NULL;

  while (runner != NULL) {
    vs = (RHVServer *)runner->data;

    (*callback)(ms, vs);

    runner = g_list_next(runner); 
  } 

  return 0;
}

void vserver_init_done(RHMainServer *ms, RHVServer *vs)
{
  if (vs != NULL) {
    vs->vserver_config->init_flag = VS_DONE;
    DP("Virtual Sever (%s) - Configured", vs->vserver_config->vserver_name);
  }
}

void vserver_reload(RHMainServer *ms, RHVServer *vs)
{
  if (vs->vserver_config->init_flag == VS_DONE) {
    // Indicate the unused virtual server
    vs->vserver_config->init_flag = VS_NONE;
    return;
  }

  while (vs->vserver_config->init_flag != VS_DONE) {
    if (vs->vserver_config->init_flag == VS_INIT) {
      logmsg(RH_LOG_NORMAL, "[%s] - Config init",
             vs->vserver_config->vserver_name);

      rh_task_init(ms, vs); 
      vs->vserver_config->init_flag = VS_DONE;
    } else if (vs->vserver_config->init_flag == VS_RELOAD) {
      logmsg(RH_LOG_NORMAL, "[%s] - Config reload",
             vs->vserver_config->vserver_name);

      rh_task_cleanup(ms, vs);

      if (vs->dummy_config != NULL) {
        cleanup_vserver_config(vs->vserver_config);
        memcpy(vs->vserver_config, vs->dummy_config, 
          sizeof(struct rahunas_vserver_config));
      }

      vs->vserver_config->init_flag = VS_RELOAD;
      rh_task_init(ms, vs); 
      vs->vserver_config->init_flag = VS_DONE;
    } else if (vs->vserver_config->init_flag == VS_RESET) {
      logmsg(RH_LOG_NORMAL, "[%s] - Config reset",
             vs->vserver_config->vserver_name);

      rh_task_cleanup(ms, vs);

      if (vs->dummy_config != NULL) {
        cleanup_vserver_config(vs->vserver_config);
        memcpy(vs->vserver_config, vs->dummy_config, 
          sizeof(struct rahunas_vserver_config));
        rh_free((void **) &vs->dummy_config);
      }

      vs->vserver_config->init_flag = VS_INIT;
    } else {
      /* Prevent infinite loop */
      vs->vserver_config->init_flag = VS_DONE;
    }
  } 
}


void vserver_unused_cleanup(RHMainServer *ms)
{
  GList *vserver_list = ms->vserver_list;
  GList *runner = g_list_first(vserver_list);
  RHVServer *lvserver = NULL;

  while (runner != NULL) {
    lvserver = (RHVServer *)runner->data;
    if (lvserver->vserver_config->init_flag == VS_NONE) {
      logmsg(RH_LOG_NORMAL, "[%s] - Config removed",
             lvserver->vserver_config->vserver_name);
      rh_task_cleanup(ms, lvserver);
      unregister_vserver(ms, lvserver->vserver_config->vserver_id);

      // Set runner to the first of list due to unregister may delete head
      runner = g_list_first(ms->vserver_list);
    } else {
      runner = g_list_next(runner);
    }
  }
}
