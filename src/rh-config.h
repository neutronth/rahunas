/**
 * RahuNASd config file 
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-08-07
 */

#ifndef __RH_CONFIG_H
#define __RH_CONFIG_H

/* Configuration */
#define DEFAULT_LOG "/var/log/rahunas/rahunas.log"
#define DEFAULT_PID "/var/run/rahunasd.pid"

#ifdef RH_DEBUG
#  define IDLE_THRESHOLD 30
#  define POLLING 15 
#else
#  define IDLE_THRESHOLD 600
#  define POLLING 60 
#endif

#define SET_NAME "rahunas_set"
#define XMLSERVICE_HOST	"localhost"
#define XMLSERVICE_PORT	8888
#define XMLSERVICE_URL	"/xmlrpc_service.php"

#endif // __RH_CONFIG_H
