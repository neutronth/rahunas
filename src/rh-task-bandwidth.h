/**
 * RahuNAS task bandwidth implementation 
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-11-20
 */
#ifndef __RH_TASK_BANDWIDTH_H
#define __RH_TASK_BANDWIDTH_H

#include "rh-server.h"

/* MAX_SLOT_PAGE is calculated from the formula of
   MAX_SLOT_PAGE = ceil(MAX_SLOT_ID/PAGE_SIZE)

   where
     PAGE_SIZE = sizeof(short) * 8
*/

#define MAX_SLOT_ID    9900
#define PAGE_SIZE      16
#define MAX_SLOT_PAGE  619

extern void rh_task_bandwidth_reg(RHMainServer *ms);

struct bandwidth_req {
  char slot_id[5];
  char ip[16];
  char bandwidth_max_down[15];
  char bandwidth_max_up[15];
};

int bandwidth_add(RHVServer *vs, struct bandwidth_req *bw_req);
int bandwidth_exec(RHVServer *vs, char *const args[]);
void mark_reserved_slot_id(unsigned int slot_id);
#endif // __RH_TASK_BANDWIDTH_H
