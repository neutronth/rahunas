/**
 * RahuNAS configuration header
 * Author: Suriya Soutmun <darksolar@gmail.com>
 * Date:   2008-11-26
 */
#ifndef __RH_CONFIG_H 
#define __RH_CONFIG_H 

#include "../lcfg/lcfg_static.h"
#include "rh-server.h"
#include "rh-serviceclass.h"

#define DEFAULT_LOG RAHUNAS_LOG_DIR "rahunas.log"
#define VSERVER_ID 99
#define IDLE_TIMEOUT 600
#define POLLING 60 
#define BANDWIDTH_SHAPE 0
#define IP_ACCOUNTING 0
#define INTERIM_INTERVAL 600

#define XMLSERVICE_HOST "localhost"
#define XMLSERVICE_PORT 80
#define XMLSERVICE_URL  "/rahunas_service/xmlrpc_service.php"

#define MAX_IFB_IFACE   64

#define CONFIG_FILE RAHUNAS_CONF_DIR "rahunas.conf"
#define DEFAULT_PID RAHUNAS_RUN_DIR "rahunasd.pid"
#define DB_NAME "rahunas"

struct interfaces {
  char dev_internal[32];
  char dev_ifb[32];
  int  hit;
  int  init;
};

struct rahunas_main_config {
  char *conf_dir;
  char *serviceclass_conf_dir;
  char *log_file;
  char *dhcp;
  int  serviceclass;
  int  bandwidth_shape;
  int  bittorrent_download_max;
  int  bittorrent_upload_max;
  int  polling_interval;
  int  ip_accounting;
};

struct rahunas_vserver_config {
  char *vserver_name;
  int  vserver_id;
  int  init_flag;
  char *dev_external;
  char *dev_internal;
  struct interfaces *iface;
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
  int  interim_interval;
};

struct rahunas_serviceclass_config {
  char *serviceclass_name;
  int  serviceclass_id;
  int  init_flag;
  char *description;
  char *network;
  struct in_addr start_addr;
  uint32_t network_size;
  char *fake_arpd;
  char *fake_arpd_iface;
};

union rahunas_config {
  struct rahunas_main_config rh_main;
  struct rahunas_vserver_config rh_vserver;
  struct rahunas_serviceclass_config rh_serviceclass;
};

enum config_type {
  MAIN,
  VSERVER,
  SERVICECLASS
};

enum vserver_config_init_flag {
  VS_NONE,
  VS_INIT,
  VS_RELOAD,
  VS_RESET,
  VS_DONE
};

enum serviceclass_config_init_flag {
  SC_NONE,
  SC_INIT,
  SC_RELOAD,
  SC_RESET,
  SC_DONE
};

extern GList *interfaces_list;

int get_config(const char *cfg_file, union rahunas_config *config);
int get_value(const char *cfg_file, const char *key, void **data, size_t *len);
int get_vservers_config(const char *conf_dir, RHMainServer *server);
int get_serviceclass_config(const char *conf_dir, RHMainServer *server);
int cleanup_vserver_config(struct rahunas_vserver_config *config);
int cleanup_serviceclass_config(struct rahunas_serviceclass_config *config);
int cleanup_mainserver_config(struct rahunas_main_config *config);
enum lcfg_status rahunas_visitor(const char *key, void *data, size_t size, 
                                 void *user_data);

GList *append_interface (GList *inf, 
                         const char *inf_name);
GList *remove_interface (GList *inf,
                         const char *inf_name);
struct interfaces *get_interface (GList *inf,
                                  const char *inf_name);
int    ifb_interface_reserve (void);
void    ifb_interface_release (int ifno);
#endif // __RH_CONFIG_H 
