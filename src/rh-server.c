/**
 * RahuNAS server
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2009-03-24
 */
#include <stdio.h>
#include "rahunasd.h"
#include "rh-server.h"
#include "rh-utils.h"

int vserver_exists(GList *vserver_list, int vserver_id, 
                   const char *vserver_name)
{
  GList *runner = g_list_first(vserver_list);
  struct vserver *lvserver = NULL;

  while (runner != NULL) {
    lvserver = (struct vserver *)runner->data;
   
    if (lvserver->vserver_config->vserver_id == vserver_id)
      return 1; 

    if (strcmp(lvserver->vserver_config->vserver_name, vserver_name) == 0)
      return 2;

    runner = g_list_next(runner); 
  } 
  return 0;
}

struct vserver *vserver_get_by_id(struct main_server *ms, int search_id)
{
  GList *runner = g_list_first(ms->vserver_list);
  struct vserver *lvserver = NULL;

  while (runner != NULL) {
    lvserver = (struct vserver *)runner->data;
   
    if (lvserver->vserver_config->vserver_id == search_id) {
      return lvserver;
    }

    runner = g_list_next(runner); 
  } 
  return NULL;
}

int vserver_cleanup(struct vserver *vs)
{
  if (vs == NULL)
    return 0;

  if (vs->vserver_config != NULL)
    cleanup_vserver_config(vs->vserver_config);  

  if (vs->v_map != NULL)
    // TODO: cleanup map

  if (vs->v_set != NULL)
    // TODO: cleanup set

  return 0;
}

int mainserver_cleanup(struct main_server *ms)
{
  if (ms == NULL)
    return 0;

  if (ms->main_config != NULL)
    cleanup_mainserver_config(ms->main_config);  

  return 0;
}

int register_vserver(struct main_server *ms, const char *vserver_cfg_file)
{
  GList *vserver_list = ms->vserver_list;
  GList *chk = NULL;
  GList *node = NULL;
  FILE  *cfg_file = NULL;
  
  union rahunas_config *cfg_get = NULL;
  struct rahunas_vserver_config *vserver_config = NULL;
  struct vserver *new_vserver = NULL;

  union rahunas_config config = {
    .rh_vserver.vserver_id = VSERVER_ID,
    .rh_vserver.vserver_ip = NULL, 
    .rh_vserver.vserver_name = NULL,
    .rh_vserver.idle_timeout = IDLE_TIMEOUT,
    .rh_vserver.xml_serv_port = XMLSERVICE_PORT,
    .rh_vserver.xml_serv_url = strdup(XMLSERVICE_URL),
  };

  cfg_file = fopen(vserver_cfg_file, "r");
  if (cfg_file == NULL)
    return -1;

  vserver_config = (struct rahunas_vserver_config *) rh_malloc(sizeof(struct rahunas_vserver_config));

  if (vserver_config == NULL)
    return -1;

  memset(vserver_config, 0, sizeof(struct rahunas_vserver_config));

  if (get_config(vserver_cfg_file, &config) != 0) {
    rh_free(&config.rh_vserver.vserver_ip);
    rh_free(&config.rh_vserver.xml_serv_url);
    return -1;
  }

  memcpy(vserver_config, &config, sizeof(struct rahunas_vserver_config));

  if (vserver_exists(vserver_list, vserver_config->vserver_id, 
                     vserver_config->vserver_name)) {
    return 1;
  }

  new_vserver = (struct vserver *) rh_malloc(sizeof(struct vserver));

  if (new_vserver == NULL)
    return -1;

  memset(new_vserver, 0, sizeof(struct vserver));

  new_vserver->vserver_config = vserver_config;

  ms->vserver_list = g_list_append(ms->vserver_list, new_vserver);
  return 0; 
}

int unregister_vserver(struct main_server *ms, int vserver_id)
{
}

int unregister_vserver_all(struct main_server *ms)
{
  GList *vserver_list = ms->vserver_list;
  GList *runner = g_list_first(vserver_list);
  struct vserver *lvserver = NULL;

  while (runner != NULL) {
    lvserver = (struct vserver *)runner->data;
    vserver_cleanup(lvserver);
    runner = g_list_delete_link(runner, runner);
  }
  
  return 0;
}

int walk_through_vserver(int (*callback)(void *, void *), struct main_server *ms)
{
  GList *vserver_list = ms->vserver_list;
  GList *runner = g_list_first(vserver_list);
  struct vserver *vs = NULL;

  while (runner != NULL) {
    vs = (struct vserver *)runner->data;

    (*callback)(ms, vs);

    runner = g_list_next(runner); 
  } 

  return 0;
}
