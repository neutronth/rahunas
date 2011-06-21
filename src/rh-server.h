/**
 * RahuNAS server 
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2009-03-24
 */
#ifndef __RH_SERVER_H
#define __RH_SERVER_H

#include <glib.h>
#include "rh-config.h"

typedef struct vserver     RHVServer;
typedef struct main_server RHMainServer;

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

RHVServer *vserver_exists    (GList *vserver_list, int vserver_id,
                                   const char *vserver_name);
RHVServer *vserver_get_by_id (RHMainServer *ms, int search_id);

int vserver_cleanup    (RHVServer *vs);
int mainserver_cleanup (RHMainServer *ms);

int walk_through_vserver    (int (*callback)(RHMainServer *, RHVServer *),
                             RHMainServer  *ms);
int register_vserver        (RHMainServer  *ms, const char *vserver_cfg_file);
int unregister_vserver      (RHMainServer  *ms, int vserver_id);
int unregister_vserver_all  (RHMainServer  *ms);
void vserver_init_done      (RHMainServer  *ms, RHVServer *vs);
void vserver_reload         (RHMainServer  *ms, RHVServer *vs);
void vserver_unused_cleanup (RHMainServer  *ms);

#endif // __RH_SERVER_H
