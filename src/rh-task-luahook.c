/**
 * RahuNAS task LUA hook implementation
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2014-09-26
 */

#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "rahunasd.h"
#include "rh-task.h"
#include "rh-task-memset.h"
#include "rh-task-luahook.h"
#include "rh-utils.h"
#include "rh-xmlrpc-cmd.h"

enum luahook_verdict {
  LUAHOOK_STOP     = 1,
  LUAHOOK_CONTINUE = 2,
  LUAHOOK_UPDATE   = 3
};

static struct rahunas_member *luahook_getdataptr (lua_State *L);
static int  luahook_getinfo (lua_State *L);
static int  luahook_setinfo (lua_State *L);
static void luahook_defineverdict (lua_State *L);
static int  luahook_sessioninit (lua_State *L, struct rahunas_member *member);
static int  luahook_callfunc (lua_State *L, const char *func);
static int  luahook_verdictprocess (enum luahook_verdict v,
                                    struct rahunas_member *member);

static const struct luaL_reg core[] = {
  { NULL, NULL }
};

static const struct luaL_reg session[] = {
  { "getinfo", luahook_getinfo },
  { "setinfo", luahook_setinfo },
  { NULL, NULL }
};

static const struct luaL_reg verdict[] = {
  { NULL, NULL }
};

static const struct luaL_reg dataptr[] = {
  { NULL, NULL }
};

static void setfield   (lua_State *L, const char *key, const char *value);
static void setinteger (lua_State *L, const char *key, lua_Integer value);

static struct rahunas_member *luahook_getdataptr (lua_State *L)
{
  struct rahunas_member *m = NULL;
  lua_getglobal (L, "rahunas");
  lua_pushliteral (L, "dataptr");
  lua_gettable (L, -2);
  m = (struct rahunas_member *) lua_touserdata (L, -1);
  lua_pop (L, 2);

  return m;
}

static int luahook_callfunc (lua_State *L, const char *func)
{
  int ret = -1;
  const char *msg = NULL;
  struct rahunas_member *member = NULL;

  DP ("lua_gettop %d", lua_gettop (L));

  lua_getglobal (L, func);
  if (lua_isfunction (L, 1)) {
    lua_call (L, 0, 2);
    ret = lua_tonumber (L, 1);
    msg = lua_tostring (L, 2);
    lua_pop (L, 2);

    member = luahook_getdataptr (L);
    DP ("lua_ret - %s: %d, %s, %p, %s", func, ret, msg, member,
        member->session_id);

    ret = luahook_verdictprocess (ret, member);
  } else {
    lua_pop (L, 1);
  }

  return ret;
}

static int luahook_verdictprocess (enum luahook_verdict v,
                                    struct rahunas_member *member)
{
  int ret = 0;
  struct task_req req;

  req.id = member->id;
  memcpy(req.mac_address, &member->mac_address, ETH_ALEN);

  switch (v) {
    case LUAHOOK_STOP:
      req.req_opt = RH_RADIUS_TERM_NAS_REQUEST;
      if (send_xmlrpc_stopacct (member->vs, member->id, req.req_opt) == 0) {
        ret = rh_task_stopsess (member->vs, &req);
      }

      break;
    case LUAHOOK_UPDATE:
      ret = rh_task_updatesess (member->vs, &req);
      time (&member->last_update);
      break;

    case LUAHOOK_CONTINUE:
      /* Do nothing */
      break;
  }

  return ret;
}

static void luahook_defineverdict (lua_State *L)
{
  lua_getglobal (L, "rahunas");

  lua_newtable (L);
  lua_pushliteral (L, "stop");
  lua_pushnumber  (L, LUAHOOK_STOP);
  lua_settable    (L, -3);

  lua_pushliteral (L, "continue");
  lua_pushnumber  (L, LUAHOOK_CONTINUE);
  lua_settable    (L, -3);

  lua_pushliteral (L, "update");
  lua_pushnumber  (L, LUAHOOK_UPDATE);
  lua_settable    (L, -3);

  lua_pushliteral (L, "verdict");
  lua_insert (L, -2);
  lua_settable (L, -3);

  lua_pop (L, 1);
}

static int luahook_setdataptr (lua_State *L, struct rahunas_member *member)
{
  lua_getglobal (L, "rahunas");
  lua_pushliteral (L, "dataptr");
  lua_pushlightuserdata (L, member);
  lua_settable (L, -3);
  lua_pop (L, 1);

  return 0;
}

static int luahook_sessioninit (lua_State *L, struct rahunas_member *member)
{
  luaL_openlibs (L);

  luaL_register (L, "rahunas", core);
  luaL_register (L, "rahunas.session", session);
  luaL_register (L, "rahunas.verdict", verdict);
  luaL_register (L, "rahunas.dataptr", dataptr);
  lua_pop (L, 4);

  if (luahook_setdataptr (L, member) != 0) {
    goto fail;
  }

  luahook_defineverdict (L);

  if (luaL_dofile (L, RAHUNAS_LUA_SCRIPT)) {
    const char *cause = lua_tostring (L, -1);
    logmsg (RH_LOG_ERROR, "lua: script load failed: %s", cause);
    goto fail;
  }

  return 0;

fail:
  lua_close (L);
  member->luastate = NULL;
  return -1;
}

/* Start service task */
static int startservice (void)
{
  /* Do nothing */
}

/* Stop service task */
static int stopservice (void)
{
  /* Do nothing */
}

/* Initialize */
static void init (RHVServer *vs)
{
  logmsg(RH_LOG_NORMAL, "[%s] Task LUA hook initialize..",
         vs->vserver_config->vserver_name);
}

/* Cleanup */
static void cleanup (RHVServer *vs)
{
  logmsg(RH_LOG_NORMAL, "[%s] Task LUA hook cleanup..",
         vs->vserver_config->vserver_name);
}

/* Start session task */
static int startsess (RHVServer *vs, struct task_req *req)
{
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;
  lua_State *L = NULL;

  member_node = member_get_node_by_id(vs, req->id);
  if (member_node == NULL)
    return (-1);

  member = (struct rahunas_member *) member_node->data;

  L = luaL_newstate ();
  member->luastate = NULL;

  if (luahook_sessioninit (L, member) == 0) {
    member->luastate = (void *) L;
    luahook_callfunc (L, "session_start");
  }

  return 0;
}

/* Stop session task */
static int stopsess  (RHVServer *vs, struct task_req *req)
{
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;
  lua_State *L = NULL;

  member_node = member_get_node_by_id(vs, req->id);
  if (member_node == NULL)
    return (-1);

  member = (struct rahunas_member *) member_node->data;

  L = (lua_State *) member->luastate;

  DP ("lua_ret - %p", L);

  if (L) {
    luahook_callfunc (L, "session_stop");
    lua_close (L);
    member->luastate = NULL;
  }

  return 0;
}

/* Update session task */
static int updatesess (RHVServer *vs, struct task_req *req)
{
  GList *member_node = NULL;
  struct rahunas_member *member = NULL;
  lua_State *L = NULL;

  member_node = member_get_node_by_id(vs, req->id);
  if (member_node == NULL)
    return (-1);

  member = (struct rahunas_member *) member_node->data;

  L = (lua_State *) member->luastate;

  DP ("lua_ret - %p", L);

  if (L) {
    luahook_callfunc (L, "session_update");
  }

  return 0;
}

/* Commit start session task */
static int commitstartsess (RHVServer *vs, struct task_req *req)
{
  /* Do nothing or need to implement */
}

/* Commit stop session task */
static int commitstopsess  (RHVServer *vs, struct task_req *req)
{
  /* Do nothing or need to implement */
}

/* Rollback start session task */
static int rollbackstartsess (RHVServer *vs, struct task_req *req)
{
  /* Do nothing or need to implement */
}

/* Rollback stop session task */
static int rollbackstopsess  (RHVServer *vs, struct task_req *req)
{
  /* Do nothing or need to implement */
}

static void setfield (lua_State *L, const char *key, const char *value)
{
  lua_pushstring (L, key);
  lua_pushstring (L, value);
  lua_settable (L, -3);
}

static void setinteger (lua_State *L, const char *key, lua_Integer value)
{
  lua_pushstring (L, key);
  lua_pushinteger (L, value);
  lua_settable (L, -3);
}

static int luahook_getinfo (lua_State *L)
{
  int ret = 0;
  struct rahunas_member *member = NULL;
  const char *infoname = NULL;

  member = luahook_getdataptr (L);
  infoname = lua_tostring (L, 1);
  lua_pop (L, 1);

  if (!infoname)
    return 0;

  DP ("luahook_getinfo - %s", infoname);

#ifndef INFONAME
#define INFONAME(a) strcmp(a, infoname) == 0
#endif

  if (INFONAME ("session_id")) {
    lua_pushstring (L, member->session_id);
    ret = 1;
  } else if (INFONAME ("ip")) {
    lua_pushstring (L, idtoip (member->vs->v_map, member->id));
  } else if (INFONAME ("username")) {
    lua_pushstring (L, member->username);
    ret = 1;
  } else if (INFONAME ("mac")) {
    lua_pushstring (L, member->mac_address);
    ret = 1;
  } else if (INFONAME ("serviceclass")) {
    lua_pushstring (L, member->serviceclass_name);
    ret = 1;
  } else if (INFONAME ("session_start")) {
    lua_pushinteger (L, member->session_start);
    ret = 1;
  } else if (INFONAME ("session_timeout")) {
    lua_pushinteger (L, member->session_timeout);
    ret = 1;
  } else if (INFONAME ("speed_download")) {
    lua_pushinteger (L, member->bandwidth_max_down);
    ret = 1;
  } else if (INFONAME ("speed_upload")) {
    lua_pushinteger (L, member->bandwidth_max_up);
    ret = 1;
  } else if (INFONAME ("bytes_download")) {
    lua_pushinteger (L, member->download_bytes);
    ret = 1;
  } else if (INFONAME ("bytes_upload")) {
    lua_pushinteger (L, member->upload_bytes);
    ret = 1;
  }

  return ret;
}

static int luahook_setinfo (lua_State *L)
{
  int ret = 0;
  struct rahunas_member *member = NULL;
  const char *infoname = NULL;

  member = luahook_getdataptr (L);
  infoname = lua_tostring (L, 1);

  if (!infoname)
    goto out;

  DP ("luahook_setinfo - %s", infoname);

#ifndef INFONAME
#define INFONAME(a) strcmp(a, infoname) == 0
#endif

  if (INFONAME ("speed_download")) {
    long speed = lua_tointeger (L, 2);
    DP ("luahook_setinfo - %s - %d", infoname, speed);

    if (speed > 0) {
      member->saved_bandwidth_max_down[SAVED_CURRENT] = speed;
      ret = 1;
    }
  } else if (INFONAME ("speed_upload")) {
    long speed = lua_tointeger (L, 2);
    DP ("luahook_setinfo - %s - %d", infoname, speed);

    if (speed > 0) {
      member->saved_bandwidth_max_up[SAVED_CURRENT] = speed;
      ret = 1;
    }
  }

out:
  lua_pop (L, lua_gettop (L));

  lua_pushinteger (L, ret);
  return 1;
}

static struct task taskluahook = {
  .taskname = "LUAHOOK",
  .taskprio = 5,
  .init = &init,
  .cleanup = &cleanup,
  .startservice = &startservice,
  .stopservice = &stopservice,
  .startsess = &startsess,
  .stopsess = &stopsess,
  .updatesess = &updatesess,
  .commitstartsess = &commitstartsess,
  .commitstopsess = &commitstopsess,
  .rollbackstartsess = &rollbackstartsess,
  .rollbackstopsess = &rollbackstopsess,
};

void rh_task_luahook_reg(RHMainServer *ms) {
  task_register(ms, &taskluahook);
}
