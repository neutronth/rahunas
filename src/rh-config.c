/**
 * RahuNAS configuration
 * Author: Suriya Soutmun <darksolar@gmail.com>
 * Date:   2008-11-26
 */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <dirent.h>
#include <errno.h>

#include "rahunasd.h"
#include "rh-config.h"

enum lcfg_status rahunas_visitor(const char *key, void *data, size_t size, 
                                 void *user_data) {
  char *value = strndup(data, size);
  union rahunas_config *config = (union rahunas_config *)user_data;
  char *clone_key = NULL;
  char *sep = NULL; 
  char *main_key = NULL;
  char *sub_key = NULL;
  enum config_type cfg_type;

  if(config == NULL)
    return lcfg_status_error;

  if(value == NULL)
    return lcfg_status_error;

  clone_key = strdup(key);
  if (clone_key == NULL)
    return lcfg_status_error;

  sep = strstr(clone_key, ".");
  main_key = clone_key;
  sub_key = sep + 1;
  *sep = '\0';

  if (strncmp(main_key, "main", 4) == 0) {
    cfg_type = MAIN;
  } else {
    cfg_type = VSERVER;
    if (config->rh_vserver.vserver_name == NULL)
      config->rh_vserver.vserver_name = strdup(main_key);
  }

  switch (cfg_type) {
    case MAIN:
      if (strncmp(sub_key, "conf_dir", 8) == 0) {
        if (config->rh_main.conf_dir != NULL)
          free(config->rh_main.conf_dir);
        config->rh_main.conf_dir = strdup(value);
      } else if (strncmp(sub_key, "bandwidth_shape", 15) == 0) {
        if (strncmp(value, "yes", 3) == 0)
          config->rh_main.bandwidth_shape = 1; 
        else
          config->rh_main.bandwidth_shape = 0;
      } else if (strncmp(sub_key, "polling_interval", 16) == 0) {
        config->rh_main.polling_interval = atoi(value);
      } else if (strncmp(sub_key, "log_file", 8) == 0) {
        if (config->rh_main.log_file != NULL)
          free(config->rh_main.log_file);
        config->rh_main.log_file = strdup(value); 
      }
      break;

    case VSERVER:
      if (strncmp(sub_key, "vserver_id", 10) == 0) {
        config->rh_vserver.vserver_id = atoi(value);
      } else if (strncmp(sub_key, "vserver_ip", 10) == 0) {
        if (config->rh_vserver.vserver_ip != NULL)
          free(config->rh_vserver.vserver_ip);
        config->rh_vserver.vserver_ip = strdup(value); 
      } else if (strncmp(sub_key, "idle_timeout", 12) == 0) {
        config->rh_vserver.idle_timeout = atoi(value);
      }
      break;
  }

  
  rh_free(&clone_key);

  return lcfg_status_ok;
}

int get_config(const char *cfg_file, union rahunas_config *config) {
  lcfg_visitor_function visitor_func = rahunas_visitor;
  struct lcfg *c = lcfg_new(cfg_file);
  
  syslog(LOG_INFO, "Parsing config file: %s", cfg_file);

  if (lcfg_parse(c) != lcfg_status_ok) {
    syslog(LOG_ERR, "config error: %s", lcfg_error_get(c));
    lcfg_delete(c);
    return -1;
  }

  syslog(LOG_INFO, "Processing config file: %s", cfg_file);
  if (lcfg_accept(c, visitor_func, config) != lcfg_status_ok) {
    syslog(LOG_ERR, "config error: %s", lcfg_error_get(c));
    lcfg_delete(c);
    return -1;
  }

  lcfg_delete(c);

  return 0;
}


int get_value(const char *cfg_file, const char *key, void **data, size_t *len)
{
  lcfg_visitor_function visitor_func = rahunas_visitor;
  struct lcfg *c = lcfg_new(cfg_file);
  
  if (lcfg_parse(c) != lcfg_status_ok) {
    syslog(LOG_ERR, "config error: %s", lcfg_error_get(c));
    lcfg_delete(c);
    return -1;
  }

  if (lcfg_value_get(c, key, data, len) != lcfg_status_ok) {
    lcfg_delete(c);
    return -1;
  } 

  lcfg_delete(c);

  return 0;
}

int get_vservers_config(const char *conf_dir, struct main_server *server)
{
  DIR *dp;
  struct dirent *dirp;
  void *data = NULL;
  size_t len;
  char conf_file[200];

  if ((dp = opendir(conf_dir)) == NULL)
    return errno;
  
  while ((dirp = readdir(dp)) != NULL) {
    if (strstr(dirp->d_name, ".conf") == NULL)
      continue;

    memset(conf_file, 0, sizeof(conf_file));

    strncat(conf_file, conf_dir, sizeof(conf_file));
    strncat(conf_file, "/", 1);
    strncat(conf_file, dirp->d_name, sizeof(conf_file));

    syslog(LOG_INFO, "Loading config file: %s", conf_file);
    
    register_vserver(server, conf_file);
  }
  
  closedir(dp);
  return 0;
}


int cleanup_vserver_config(struct rahunas_vserver_config *config)
{
  rh_free(&(config->vserver_ip));
  rh_free(&(config->vserver_name));  
  return 0;
}

int cleanup_mainserver_config(struct rahunas_main_config *config)
{
  rh_free(&(config->conf_dir));  
  rh_free(&(config->log_file));
  return 0;
}
