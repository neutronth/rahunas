/**
 * RahuNAS configuration
 * Author: Suriya Soutmun <darksolar@gmail.com>
 * Date:   2008-11-26
 */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "rh-config.h"

void config_default(struct rahunas_config *config) {
  config->polling_interval = POLLING;
  config->idle_threshold = IDLE_THRESHOLD;
  config->xml_serv_port = XMLSERVICE_PORT;
  config->xml_serv_host = strdup(XMLSERVICE_HOST);
  config->xml_serv_url = strdup(XMLSERVICE_URL);
  config->set_name = strdup(SET_NAME);
  config->log_file = strdup(DEFAULT_LOG);
}

enum lcfg_status rahunas_visitor(const char *key, void *data, size_t size, 
                                 void *user_data) {
  struct rahunas_config *config = (struct rahunas_config *)user_data;

  if(config == NULL)
    return lcfg_status_error;

  char *value = strndup(data, size);

  if(value == NULL)
    return lcfg_status_error;

  if(!strcmp(key, "log_file")) {
    config->log_file = value;
  } else if (!strcmp(key, "idle_threshold")) {
    config->idle_threshold = atoi(value);
  } else if (!strcmp(key, "polling")) {
    config->polling_interval = atoi(value);
  } else if (!strcmp(key, "set_name")) {
    config->set_name = value;
  } else if (!strcmp(key, "xml_service_host")) {
    config->xml_serv_host = value;
  } else if (!strcmp(key, "xml_service_port")) {
    config->xml_serv_port = atoi(value);
  } else if (!strcmp(key, "xml_service_url")) {
    config->xml_serv_url = value; 
  }

  return lcfg_status_ok;
}

int config_init(struct rahunas_config *config) {

  config_default(config);

  lcfg_visitor_function visitor_func = rahunas_visitor;
  struct lcfg *c = lcfg_new(CONFIG_FILE);
  
  if (lcfg_parse(c) != lcfg_status_ok) {
    syslog(LOG_ERR, "config error: %s", lcfg_error_get(c));
    lcfg_delete(c);
    return -1;
  }

  if (lcfg_accept(c, visitor_func, config) != lcfg_status_ok) {
    syslog(LOG_ERR, "config error: %s", lcfg_error_get(c));
    lcfg_delete(c);
    return -1;
  }

  if (c != NULL)
    lcfg_delete(c);

  return 0;
}
