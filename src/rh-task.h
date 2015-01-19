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
  uint16_t bandwidth_slot_id;
  unsigned long bandwidth_max_down;
  unsigned long bandwidth_max_up;
  const char *serviceclass_name;
  uint32_t serviceclass_slot_id;
  char     secure_token[65];
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
  void (*init) (RHVServer  *vs);

  /* Cleanup */
  void (*cleanup) (RHVServer  *vs);

  /* Start session task */
  int (*startsess) (RHVServer  *vs, struct task_req *req);

  /* Stop session task */
  int (*stopsess) (RHVServer  *vs, struct task_req *req);

  /* Commit start session task */
  int (*commitstartsess) (RHVServer  *vs, struct task_req *req);

  /* Commit stop session task */
  int (*commitstopsess) (RHVServer  *vs, struct task_req *req);

  /* Rollback start session task */
  int (*rollbackstartsess) (RHVServer  *vs, struct task_req *req);

  /* Rollback stop session task */
  int (*rollbackstopsess) (RHVServer  *vs, struct task_req *req);
};

extern void task_register(RHMainServer *ms, struct task *task);

void rh_task_register          (RHMainServer *ms);
void rh_task_unregister        (RHMainServer *ms);
int  rh_task_startservice      (RHMainServer *ms);
int  rh_task_stopservice       (RHMainServer *ms);
void rh_task_init              (RHMainServer *ms, RHVServer  *vs);
void rh_task_cleanup           (RHMainServer *ms, RHVServer  *vs);
int  rh_task_startsess         (RHVServer  *vs, struct task_req *req);
int  rh_task_stopsess          (RHVServer  *vs, struct task_req *req);
int  rh_task_commitstartsess   (RHVServer  *vs, struct task_req *req);
int  rh_task_commitstopsess    (RHVServer  *vs, struct task_req *req);
int  rh_task_rollbackstartsess (RHVServer  *vs, struct task_req *req);
int  rh_task_rollbackstopsess  (RHVServer  *vs, struct task_req *req);

#endif // __RH_TASK_H
