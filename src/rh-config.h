/**
 * RahuNAS configuration header
 * Author: Suriya Soutmun <darksolar@gmail.com>
 * Date:   2008-11-26
 */
#ifndef __RH_CONFIG_H 
#define __RH_CONFIG_H 

#include "../lcfg/lcfg_static.h"
#include "rh-server.h"

#define DEFAULT_LOG RAHUNAS_LOG_DIR "rahunas.log"
#define VSERVER_ID 99
#define IDLE_TIMEOUT 600
#define POLLING 60 
#define BANDWIDTH_SHAPE 0

#define XMLSERVICE_HOST "localhost"
#define XMLSERVICE_PORT 80
#define XMLSERVICE_URL  "/rahunas_service/xmlrpc_service.php"

#define CONFIG_FILE RAHUNAS_CONF_DIR "rahunas.conf"
#define DEFAULT_PID RAHUNAS_RUN_DIR "rahunasd.pid"
#define DB_NAME "rahunas"

struct rahunas_main_config {
  char *conf_dir;
  char *log_file;
  char *dhcp;
  int  bandwidth_shape;
  int  bittorrent_download_max;
  int  bittorrent_upload_max;
  int  polling_interval;
};

struct rahunas_vserver_config {
  char *vserver_name;
  int  vserver_id;
  char *dev_external;
  char *dev_internal;
  char *vlan;
  char *vlan_raw_dev_external;
  char *vlan_raw_dev_internal;
  char *bridge;
  char *masquerade;
  char *ignore_mac;
  char *vserver_ip;
  char *vserver_fqdn;
  char *vserver_ports_allow;
  char *vserver_ports_intercept;
  char *clients;
  char *excluded;
  int  idle_timeout;
  char *dns;
  char *ssh;
  char *proxy;
  char *proxy_host;
  char *proxy_port;
  char *bittorrent;
  char *bittorrent_allow;
  char *radius_host;
  char *radius_secret;
  char *radius_encrypt;
  char *radius_auth_port;
  char *radius_account_port;
  char *nas_identifier;
  char *nas_port;
  char *nas_login_title;
  char *nas_default_redirect;
  char *nas_default_language;
  char *nas_weblogin_template;
};

union rahunas_config {
  struct rahunas_main_config rh_main;
  struct rahunas_vserver_config rh_vserver;
};

enum config_type {
  MAIN,
  VSERVER
};

int get_config(const char *cfg_file, union rahunas_config *config);
int get_value(const char *cfg_file, const char *key, void **data, size_t *len);
int get_vservers_config(const char *conf_dir, struct main_server *server);
int cleanup_vserver_config(struct rahunas_vserver_config *config);
int cleanup_mainserver_config(struct rahunas_main_config *config);
enum lcfg_status rahunas_visitor(const char *key, void *data, size_t size, 
                                 void *user_data);
#endif // __RH_CONFIG_H 
