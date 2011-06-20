/**
 * RahuNAS service class
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2010-07-04
 */
#ifndef __RH_SERVICECLASS_H
#define __RH_SERVICECLASS_H

#include <glib.h>
#include "rh-config.h"

#define SERVICECLASS_SET_NAME   "rahunas_serviceclass"

typedef struct serviceclass RHSvClass;

struct serviceclass {
  struct rahunas_serviceclass_config *serviceclass_config;
  struct rahunas_serviceclass_config *dummy_config;
};


struct serviceclass *serviceclass_exists(GList *serviceclass_list,
                                         int serviceclass_id,
                                         const char *servicclass_name);

struct servicclass *serviceclass_get_by_id(struct main_server *ms,
                                           int search_id);
int serviceclass_cleanup(struct serviceclass *sc);

int register_serviceclass(struct main_server *ms,
                          const char *serviceclass_cfg_file);
int unregister_serviceclass(struct main_server *ms, int serviceclass_id);
int unregister_serviceclass_all(struct main_server *ms);

void serviceclass_init(struct main_server *ms, struct serviceclass *sc);
void serviceclass_reload(struct main_server *ms, struct serviceclass *sc);
void serviceclass_unused_cleanup(struct main_server *ms);

#endif // __RH_SERVICECLASS_H
