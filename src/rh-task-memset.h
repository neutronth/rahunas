/**
 * RahuNAS task memset implementation 
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-09-07
 */
#ifndef __RH_TASK_MEMSET_H
#define __RH_TASK_MEMSET_H

#include <glib.h>
#include "rh-server.h"

extern void rh_task_memset_reg(RHMainServer *ms);

GList *member_get_node_by_id(RHVServer *vs, uint32_t id);
#endif // __RH_TASK_MEMSET_H

