/**
 * RahuNAS service class
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2010-07-04
 */
#ifndef __RH_SERVICECLASS_H
#define __RH_SERVICECLASS_H

#include <glib.h>
#include "rh-server.h"
#include "rh-config.h"

#define SERVICECLASS_SET_NAME   "rahunas_serviceclass"

typedef struct serviceclass RHSvClass;

struct serviceclass {
  struct rahunas_serviceclass_config *serviceclass_config;
  struct rahunas_serviceclass_config *dummy_config;
};


RHSvClass *serviceclass_exists    (GList *serviceclass_list,
                                   int serviceclass_id,
                                   const char *serviceclass_name);

RHSvClass *serviceclass_get_by_id (RHMainServer *ms, int search_id);
int serviceclass_cleanup          (RHSvClass *sc);

int register_serviceclass         (RHMainServer *ms,
                                   const char *serviceclass_cfg_file);
int unregister_serviceclass       (RHMainServer *ms, int serviceclass_id);
int unregister_serviceclass_all   (RHMainServer *ms);

void serviceclass_init            (RHMainServer *ms, RHSvClass *sc);
void serviceclass_reload          (RHMainServer *ms, RHSvClass *sc);
void serviceclass_unused_cleanup  (RHMainServer *ms);

#endif // __RH_SERVICECLASS_H
