/**
 * RahuNAS task memset implementation 
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 *         Suriya Soutmun <darksolar@gmail.com>
 * Date:   2008-09-07
 */

#include <syslog.h>

#include "rahunasd.h"
#include "rh-ipset.h"
#include "rh-task.h"
#include "rh-xmlrpc-cmd.h"

size_t nas_stopservice(void *data)
{
  struct ip_set_list *setlist = (struct ip_set_list *) data;
  struct set *set = set_list[setlist->index];
  size_t offset;
  struct ip_set_rahunas *table = NULL;
  struct rahunas_member *members = map->members;
  struct task_req req;
  unsigned int i;
  char *ip = NULL;
  int res  = 0;

  offset = sizeof(struct ip_set_list) + setlist->header_size;
  table = (struct ip_set_rahunas *)(data + offset);

  DP("Map size %d", map->size);
 
  for (i = 0; i < map->size; i++) {
    if (test_bit(IPSET_RAHUNAS_ISSET, (void *)&table[i].flags)) {
      DP("Found IP: %s in set, try logout", idtoip(map, i));
      req.id = i;
      memcpy(req.mac_address, &table[i].ethernet, ETH_ALEN);
      req.req_opt = RH_RADIUS_TERM_NAS_REBOOT;
			send_xmlrpc_stopacct(map, i, RH_RADIUS_TERM_NAS_REBOOT);
      rh_task_stopsess(map, &req);
    }
  }
}

/* Initialize */
static void init (void)
{
  logmsg(RH_LOG_NORMAL, "Task IPSET init..");  

  rahunas_set = set_adt_get(SET_NAME);

  DP("getsetname: %s", rahunas_set->name);
  DP("getsetid: %d", rahunas_set->id);
  DP("getsetindex: %d", rahunas_set->index);
}

/* Cleanup */
static void cleanup (void)
{
  logmsg(RH_LOG_NORMAL, "Task IPSET cleanup..");  
  set_flush(SET_NAME);

  rh_free(&rahunas_set);
}

/* Start service task */
static int startservice (struct rahunas_map *map)
{
  /* Ensure the set is empty */
  set_flush(SET_NAME);
}

/* Stop service task */
static int stopservice  (struct rahunas_map *map)
{
  logmsg(RH_LOG_NORMAL, "Task IPSET stop..");  
  walk_through_set(&nas_stopservice);

  return 0;
}

/* Start session task */
static int startsess (struct rahunas_map *map, struct task_req *req)
{
  int res = 0;
  ip_set_ip_t ip;
  parse_ip(idtoip(map, req->id), &ip);

  res = set_adtip_nb(rahunas_set, &ip, req->mac_address, IP_SET_OP_ADD_IP);

  return res;
}

/* Stop session task */
static int stopsess  (struct rahunas_map *map, struct task_req *req)
{
  int res = 0;
  ip_set_ip_t ip;
  parse_ip(idtoip(map, req->id), &ip);

  res = set_adtip_nb(rahunas_set, &ip, req->mac_address, IP_SET_OP_DEL_IP);

  return res;
}

/* Commit start session task */
static int commitstartsess (struct rahunas_map *map, struct task_req *req)
{
  /* Do nothing or need to implement */
}

/* Commit stop session task */
static int commitstopsess  (struct rahunas_map *map, struct task_req *req)
{
  /* Do nothing or need to implement */
}

/* Rollback start session task */
static int rollbackstartsess (struct rahunas_map *map, struct task_req *req)
{
  /* Do nothing or need to implement */
}

/* Rollback stop session task */
static int rollbackstopsess  (struct rahunas_map *map, struct task_req *req)
{
  /* Do nothing or need to implement */
}

static struct task task_ipset = {
  .taskname = "IPSET",
  .taskprio = 2,
  .init = &init,
  .cleanup = &cleanup,
  .startservice = &startservice,
  .stopservice = &stopservice,
  .startsess = &startsess,
  .stopsess = &stopsess,
  .commitstartsess = &commitstartsess,
  .commitstopsess = &commitstopsess,
  .rollbackstartsess = &rollbackstartsess,
  .rollbackstopsess = &rollbackstopsess,
};

void rh_task_ipset_reg(void) {
  task_register(&task_ipset);
}
