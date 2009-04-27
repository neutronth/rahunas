/**
 * RahuNAS server 
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2009-03-24
 */
#ifndef __RH_SERVER_H
#define __RH_SERVER_H

#include <glib.h>
#include "rh-config.h"

struct vserver {
  struct rahunas_vserver_config *vserver_config;
  struct rahunas_vserver_config *dummy_config;
  struct rahunas_map *v_map;
  struct set *v_set;
};

struct main_server {
  struct rahunas_main_config *main_config;
  GList *vserver_list;
  GList *task_list;
  int log_fd;
  int polling_blocked;
};

struct vserver *vserver_exists(GList *vserver_list, int vserver_id, 
                   const char *vserver_name);
struct vserver *vserver_get_by_id(struct main_server *ms, int search_id);
int vserver_cleanup(struct vserver *vs);
int mainserver_cleanup(struct main_server *ms);
int walk_through_vserver(int (*callback)(void *, void *), struct main_server *ms);
int register_vserver(struct main_server *ms, const char *vserver_cfg_file);
int unregister_vserver(struct main_server *ms, int vserver_id);
int unregister_vserver_all(struct main_server *ms);
void vserver_init_done(struct main_server *ms, struct vserver *vs);
void vserver_reload(struct main_server *ms, struct vserver *vs);
void vserver_unused_cleanup(struct main_server *ms);

#endif // __RH_SERVER_H
