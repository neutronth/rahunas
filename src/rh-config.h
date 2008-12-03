/**
 * RahuNAS configuration header
 * Author: Suriya Soutmun <darksolar@gmail.com>
 * Date:   2008-11-26
 */
#ifndef __RH_CONFIG_H 
#define __RH_CONFIG_H 

#include "../lcfg/lcfg_static.h"

#define DEFAULT_LOG RAHUNAS_LOG_DIR "rahunas.log"
#define IDLE_THRESHOLD 600
#define POLLING 60 
#define SET_NAME "rahunas_set"
#define XMLSERVICE_HOST "localhost"
#define XMLSERVICE_PORT 8888
#define XMLSERVICE_URL  "/xmlrpc_service.php"

#define CONFIG_FILE RAHUNAS_CONF_DIR "rahunas.conf"
#define DEFAULT_PID RAHUNAS_RUN_DIR "rahunasd.pid"
#define DB_NAME "rahunas"

struct rahunas_config {
  int  polling_interval;
  int  idle_threshold;
  int  xml_serv_port;
  char *xml_serv_host;
  char *xml_serv_url;
  char *set_name;
  char *log_file;
};

int config_init(struct rahunas_config *config);
enum lcfg_status rahunas_visitor(const char *key, void *data, size_t size, 
                                 void *user_data);
#endif // __RH_CONFIG_H 
