/**
 * ipset control header 
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-08-07
 */

#ifndef __IPSET_CONTROL_H
#define __IPSET_CONTROL_H

#include "rahunasd.h"

#define	RH_IPSET_CMD_ADD  1
#define RH_IPSET_CMD_DEL  2
#define RH_IPSET_CMD_FLS  3

int ctrl_add_to_set(struct rahunas_map *map, uint32_t id);

int ctrl_del_from_set(struct rahunas_map *map, uint32_t id);

int ctrl_flush();

#endif //__IPSET_CONTROL_H

