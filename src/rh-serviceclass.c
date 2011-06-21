/**
 * RahuNAS service class
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2009-03-24
 */
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "rahunasd.h"
#include "rh-serviceclass.h"
#include "rh-utils.h"
#include "rh-ipset.h"

int serviceclass_do_init (RHMainServer *ms, RHSvClass *sc);

RHSvClass *serviceclass_exists(GList *serviceclass_list,
                                         int serviceclass_id,
                                         const char *serviceclass_name)
{
  GList *runner = g_list_first(serviceclass_list);
  RHSvClass *lserviceclass = NULL;

  while (runner != NULL) {
    lserviceclass = (RHSvClass *)runner->data;

    if (lserviceclass->serviceclass_config->serviceclass_id == serviceclass_id)
      return lserviceclass;

    if (strcmp(lserviceclass->serviceclass_config->serviceclass_name,
        serviceclass_name) == 0)
      return lserviceclass;

    runner = g_list_next(runner);
  }
  return NULL;
}

RHSvClass *serviceclass_get_by_id(RHMainServer *ms, int search_id)
{
  GList *runner = g_list_first(ms->serviceclass_list);
  RHSvClass *lserviceclass = NULL;

  while (runner != NULL) {
    lserviceclass = (RHSvClass *)runner->data;

    if (lserviceclass->serviceclass_config->serviceclass_id == search_id) {
      return lserviceclass;
    }

    runner = g_list_next(runner);
  }
  return NULL;
}

int serviceclass_cleanup(RHSvClass *sc)
{
  if (sc == NULL)
    return 0;

  if (sc->serviceclass_config != NULL)
    cleanup_serviceclass_config(sc->serviceclass_config);

  return 0;
}

int register_serviceclass(RHMainServer *ms,
                          const char *serviceclass_cfg_file)
{
  GList *serviceclass_list = ms->serviceclass_list;
  GList *chk = NULL;
  GList *node = NULL;
  FILE  *cfg_file = NULL;
  union rahunas_config *cfg_get = NULL;
  struct rahunas_serviceclass_config *serviceclass_config = NULL;
  RHSvClass *new_serviceclass = NULL;
  RHSvClass *old_serviceclass = NULL;

  union rahunas_config config = {
    .rh_serviceclass.serviceclass_name = NULL,
    .rh_serviceclass.serviceclass_id = 1,
    .rh_serviceclass.description  = NULL,
    .rh_serviceclass.network      = NULL,
    .rh_serviceclass.network_size = 0,
    .rh_serviceclass.fake_arpd    = NULL,
    .rh_serviceclass.fake_arpd_iface = NULL,
    .rh_serviceclass.init_flag    = 0
  };

  cfg_file = fopen(serviceclass_cfg_file, "r");
  if (cfg_file == NULL)
    return -1;

  if (get_config(serviceclass_cfg_file, &config) == 0) {
    old_serviceclass = serviceclass_exists(serviceclass_list,
                                      config.rh_serviceclass.serviceclass_id,
                                      config.rh_serviceclass.serviceclass_name);

    if (old_serviceclass != NULL) {
      if (old_serviceclass->dummy_config != NULL) {
        DP("Cleanup old dummy config");
        rh_free((void **) &old_serviceclass->dummy_config);
      }

      old_serviceclass->dummy_config =
        (struct rahunas_serviceclass_config *) rh_malloc(sizeof(struct rahunas_serviceclass_config));

      if (old_serviceclass->dummy_config == NULL)
        return -1;

      memset(old_serviceclass->dummy_config, 0,
        sizeof(struct rahunas_serviceclass_config));
      memcpy(old_serviceclass->dummy_config, &config,
        sizeof(struct rahunas_serviceclass_config));

      if (strncmp(config.rh_serviceclass.serviceclass_name,
           old_serviceclass->serviceclass_config->serviceclass_name, 32) != 0) {
        old_serviceclass->serviceclass_config->init_flag = VS_RESET;
      } else {
        old_serviceclass->serviceclass_config->init_flag = VS_RELOAD;
      }
      return 1;
    }
  } else {
    return -1;
  }

  serviceclass_config = (struct rahunas_serviceclass_config *) rh_malloc(sizeof(struct rahunas_serviceclass_config));

  if (serviceclass_config == NULL)
    return -1;

  memset(serviceclass_config, 0, sizeof(struct rahunas_serviceclass_config));

  memcpy(serviceclass_config, &config, sizeof(struct rahunas_serviceclass_config));

  new_serviceclass = (RHSvClass *) rh_malloc(sizeof(RHSvClass));

  if (new_serviceclass == NULL)
    return -1;

  memset(new_serviceclass, 0, sizeof(RHSvClass));

  new_serviceclass->serviceclass_config = serviceclass_config;

  new_serviceclass->serviceclass_config->init_flag = VS_INIT;
  ms->serviceclass_list = g_list_append(ms->serviceclass_list, new_serviceclass);
  return 0;
}

int unregister_serviceclass(RHMainServer *ms, int serviceclass_id)
{
  GList *serviceclass_list = ms->serviceclass_list;
  GList *runner = g_list_first(serviceclass_list);
  RHSvClass *lserviceclass = NULL;

  while (runner != NULL) {
    lserviceclass = (RHSvClass *)runner->data;
    if (lserviceclass->serviceclass_config->serviceclass_id == serviceclass_id) {
      serviceclass_cleanup(lserviceclass);

      ms->serviceclass_list = g_list_delete_link(ms->serviceclass_list, runner);
      break;
    } else {
      runner = g_list_next(runner);
    }
  }
  return 0;
}

int unregister_serviceclass_all(RHMainServer *ms)
{
  GList *serviceclass_list = ms->serviceclass_list;
  GList *runner = g_list_first(serviceclass_list);
  GList *deleting = NULL;
  RHSvClass *lserviceclass = NULL;

  while (runner != NULL) {
    lserviceclass = (RHSvClass *)runner->data;
    serviceclass_cleanup(lserviceclass);
    deleting = runner;
    runner = g_list_next(runner);

    cleanup_serviceclass_config(lserviceclass->serviceclass_config);
    ms->serviceclass_list = g_list_delete_link(ms->serviceclass_list, deleting);
  }

  return 0;
}

int walk_through_serviceclass(int (*callback)(void *, void *), RHMainServer *ms)
{
  GList *serviceclass_list = ms->serviceclass_list;
  GList *runner = g_list_first(serviceclass_list);
  RHSvClass *sc = NULL;

  while (runner != NULL) {
    sc = (RHSvClass *)runner->data;

    (*callback)(ms, sc);

    runner = g_list_next(runner);
  }

  return 0;
}

void serviceclass_init(RHMainServer *ms, RHSvClass *sc)
{
  struct rahunas_serviceclass_config *sc_config = NULL;

  if (ms == NULL || sc == NULL)
    return;

  sc_config = sc->serviceclass_config;
  logmsg (RH_LOG_NORMAL, "[%s] Service class init", sc_config->serviceclass_name);

  serviceclass_do_init (ms, sc);

  sc_config->init_flag = SC_DONE;
  DP("Service Class (%s) - Configured", sc->serviceclass_config->serviceclass_name);
}

void serviceclass_reload(RHMainServer *ms, RHSvClass *sc)
{
  if (sc->serviceclass_config->init_flag == SC_DONE) {
    sc->serviceclass_config->init_flag = SC_NONE;
    return;
  }

  while (sc->serviceclass_config->init_flag != SC_DONE) {
    if (sc->serviceclass_config->init_flag == SC_INIT) {
      logmsg(RH_LOG_NORMAL, "[%s] - Service class config init",
             sc->serviceclass_config->serviceclass_name);

      serviceclass_do_init (ms, sc);

      sc->serviceclass_config->init_flag = SC_DONE;
    } else if (sc->serviceclass_config->init_flag == SC_RELOAD) {
      logmsg(RH_LOG_NORMAL, "[%s] - Service class config reload",
             sc->serviceclass_config->serviceclass_name);

      if (sc->dummy_config != NULL) {
        cleanup_serviceclass_config(sc->serviceclass_config);
        memcpy(sc->serviceclass_config, sc->dummy_config,
          sizeof(struct rahunas_serviceclass_config));
      }

      serviceclass_do_init (ms, sc);

      sc->serviceclass_config->init_flag = SC_DONE;
    } else if (sc->serviceclass_config->init_flag == SC_RESET) {
      logmsg(RH_LOG_NORMAL, "[%s] - Service class config reset",
             sc->serviceclass_config->serviceclass_name);

      if (sc->dummy_config != NULL) {
        cleanup_serviceclass_config(sc->serviceclass_config);
        memcpy(sc->serviceclass_config, sc->dummy_config,
          sizeof(struct rahunas_serviceclass_config));
        rh_free((void **) &sc->dummy_config);
      }

      sc->serviceclass_config->init_flag = SC_INIT;
    } else {
      /* Prevent infinite loop */
      sc->serviceclass_config->init_flag = SC_DONE;
    }
  }
}


void serviceclass_unused_cleanup(RHMainServer *ms)
{
  GList *serviceclass_list = ms->serviceclass_list;
  GList *runner = g_list_first(serviceclass_list);
  RHSvClass *lserviceclass = NULL;

  while (runner != NULL) {
    lserviceclass = (RHSvClass *)runner->data;
    if (lserviceclass->serviceclass_config->init_flag == SC_NONE) {
      logmsg(RH_LOG_NORMAL, "[%s] - Service class config removed",
             lserviceclass->serviceclass_config->serviceclass_name);
      unregister_serviceclass(ms, lserviceclass->serviceclass_config->serviceclass_id);

      // Set runner to the first of list due to unregister may delete head
      runner = g_list_first(ms->serviceclass_list);
    } else {
      runner = g_list_next(runner);
    }
  }
}

int serviceclass_do_init (RHMainServer *ms, RHSvClass *sc)
{
  struct rahunas_serviceclass_config *sc_config = sc->serviceclass_config;

  logmsg (RH_LOG_NORMAL, "[%s] Mapping network: %s",
                         sc_config->serviceclass_name,
                         sc_config->network);
  if (sc_config->fake_arpd != NULL) {
    if ((strncasecmp (sc_config->fake_arpd, "yes", strlen ("yes")) == 0) &&
        (sc_config->fake_arpd_iface != NULL)) {
      logmsg(RH_LOG_NORMAL, "[%s] Fake-arpd: Enabled (on %s)",
                            sc_config->serviceclass_name,
                            sc_config->fake_arpd_iface);
    } else {
      logmsg(RH_LOG_NORMAL, "[%s] Fake-arpd: Disabled",
                            sc_config->serviceclass_name);
    }
  }
}
