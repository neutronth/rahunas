/**
 * RahuNAS JSON util
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2014-11-25
 */

#ifndef __RH_JSON_H
#define __RH_JSON_H

#include <glib.h>
#include <jansson.h>

const char *get_json_data_string (const char *field, const json_t *root);
int         get_json_data_integer (const char *field, const json_t *root);
gchar      *create_json_request (const char *fmt, ...);
gchar      *create_json_reply (int status, const char *fmt, ...);
int         get_json_reply_status (const gchar *reply);

#endif // __RH_JSON_H
