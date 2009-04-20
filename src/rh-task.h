/**
 * RahuNAS task header
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-09-05
 */
#ifndef __RH_TASK_H
#define __RH_TASK_H

#include "rahunasd.h"
#include <glib.h>

#define RH_TASK_MAXNAMELEN 64

struct task_req {
  uint32_t id;
  int vserver_id;
  const char *username;
  const char *session_id;
  unsigned char mac_address[ETH_ALEN];
  time_t session_start;
  time_t session_timeout;
  unsigned short bandwidth_slot_id; 
  unsigned long bandwidth_max_down;
  unsigned long bandwidth_max_up;
  unsigned short req_opt;
};

struct task {

  char taskname[RH_TASK_MAXNAMELEN];
  unsigned int taskprio;

  /* Start service task */
  int (*startservice) (void);

  /* Stop service task */
  int (*stopservice) (void);

  /* Initialize */
  void (*init) (struct vserver *vs);

  /* Cleanup */
  void (*cleanup) (struct vserver *vs);

  /* Start session task */
  int (*startsess) (struct vserver *vs, struct task_req *req);

  /* Stop session task */
  int (*stopsess) (struct vserver *vs, struct task_req *req);

  /* Commit start session task */
  int (*commitstartsess) (struct vserver *vs, struct task_req *req);

  /* Commit stop session task */
  int (*commitstopsess) (struct vserver *vs, struct task_req *req);

  /* Rollback start session task */
  int (*rollbackstartsess) (struct vserver *vs, struct task_req *req);

  /* Rollback stop session task */
  int (*rollbackstopsess) (struct vserver *vs, struct task_req *req);
};

extern void task_register(struct main_server *ms, struct task *task);

void rh_task_register(struct main_server *ms);
void rh_task_unregister(struct main_server *ms);
int  rh_task_startservice(struct main_server *ms);
int  rh_task_stopservice(struct main_server *ms);
void rh_task_init(struct main_server *ms, struct vserver *vs);
void rh_task_cleanup(struct main_server *ms, struct vserver *vs);
int  rh_task_startsess(struct vserver *vs, struct task_req *req);
int  rh_task_stopsess(struct vserver *vs, struct task_req *req);
int  rh_task_commitstartsess(struct vserver *vs, struct task_req *req);
int  rh_task_commitstopsess(struct vserver *vs, struct task_req *req);
int  rh_task_rollbackstartsess(struct vserver *vs, struct task_req *req);
int  rh_task_rollbackstopsess(struct vserver *vs, struct task_req *req);

#endif // __RH_TASK_H
