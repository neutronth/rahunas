/**
 * RahuNAS task memset implementation 
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-09-07
 */
#ifndef __RH_TASK_MEMSET_H
#define __RH_TASK_MEMSET_H

extern void rh_task_memset_reg(struct main_server *ms);

GList *member_get_node_by_id(struct vserver *vs, uint32_t id);
gint idcmp(struct rahunas_member *a, struct rahunas_member *b);
#endif // __RH_TASK_MEMSET_H

