/**
 * RahuNAS task vipmap implementation 
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2010-05-11
 */
#ifndef __RH_TASK_VIPMAP_H
#define __RH_TASK_VIPMAP_H

struct vipmap_req {
  char ip[16];
  char vip_ip[16];
};

extern void rh_task_vipmap_reg(struct main_server *ms);

#endif // __RH_TASK_VIPMAP_H

