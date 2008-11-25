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
  const char *username;
  const char *session_id;
  unsigned char mac_address[ETH_ALEN];
	time_t session_start;
  time_t session_timeout;
  unsigned long bandwidth_max_down;
  unsigned long bandwidth_max_up;
  unsigned short req_opt;
};

struct task {

  char taskname[RH_TASK_MAXNAMELEN];
  unsigned int taskprio;

  /* Initialize */
  void (*init) (void);

  /* Cleanup */
  void (*cleanup) (void);
  
  /* Start service task */
  int (*startservice) (struct rahunas_map *map);

  /* Stop service task */
  int (*stopservice) (struct rahunas_map *map);

  /* Start session task */
  int (*startsess) (struct rahunas_map *map, struct task_req *req);

  /* Stop session task */
  int (*stopsess) (struct rahunas_map *map, struct task_req *req);

  /* Commit start session task */
  int (*commitstartsess) (struct rahunas_map *map, struct task_req *req);

  /* Commit stop session task */
  int (*commitstopsess) (struct rahunas_map *map, struct task_req *req);

  /* Rollback start session task */
  int (*rollbackstartsess) (struct rahunas_map *map, struct task_req *req);

  /* Rollback stop session task */
  int (*rollbackstopsess) (struct rahunas_map *map, struct task_req *req);
};

static GList *task_list = NULL;

extern void task_register(struct task *task);

void rh_task_init(void);
void rh_task_cleanup(void);
int  rh_task_startservice(struct rahunas_map *map);
int  rh_task_stopservice(struct rahunas_map *map);
int  rh_task_startsess(struct rahunas_map *map, struct task_req *req);
int  rh_task_stopsess(struct rahunas_map *map, struct task_req *req);
int  rh_task_commitstartsess(struct rahunas_map *map, struct task_req *req);
int  rh_task_commitstopsess(struct rahunas_map *map, struct task_req *req);
int  rh_task_rollbackstartsess(struct rahunas_map *map, struct task_req *req);
int  rh_task_rollbackstopsess(struct rahunas_map *map, struct task_req *req);

#endif // __RH_TASK_H
