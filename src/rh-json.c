/**
 * RahuNAS JSON util
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2014-11-25
 */

#include <glib/gbase64.h>
#include <string.h>
#include "rahunasd.h"
#include "rh-json.h"

const char *get_json_data_string (const char *field, const json_t *root)
{
  const char *ret_data = NULL;
  json_t     *data = NULL;

  data = json_object_get (root, field);
  if (json_is_string (data)) {
    ret_data = json_string_value (data);
    DP ("Params %s - %s", field, ret_data);
  }

  return ret_data;
}

int get_json_data_integer (const char *field, const json_t *root)
{
  int     ret_data = 0;
  json_t *data     = NULL;

  data = json_object_get (root, field);
  if (json_is_number (data)) {
    ret_data = json_integer_value (data);
    DP ("Params %s - %" JSON_INTEGER_FORMAT, field, ret_data);
  }

  return ret_data;
}

static
gchar *create_json_request_reply (int status, const char *type,
                                  const char *fmt, va_list ap)
{
  json_t *r_data  = NULL;
  json_t *r_root  = NULL;
  char   *r       = NULL;
  gchar  *encoded = NULL;

  r_data = json_vpack_ex (NULL, 0, fmt, ap);

  if (r_data) {
    if (strncmp (type, "Rep", 3) == 0) {
      r_root = json_pack ("{siss}", "Status", status, type, "");
      json_object_set (r_root, type, r_data);
    }

    r = json_dumps ((r_root ? r_root : r_data), 0);

    if (r) {
      DP ("Create %s: %s", type, r);
      encoded = g_base64_encode (r, strlen (r));
      free (r);
      DP ("Create %s Encoded: %s", type, encoded);
    }
  }

  json_decref (r_data);
  json_decref (r_root);

  return encoded ? encoded : g_strdup ("{}");
}

gchar *create_json_request (const char *fmt, ...)
{
  va_list ap;
  gchar  *encoded = NULL;

  va_start (ap, fmt);
  encoded = create_json_request_reply (0, "Request", fmt, ap);
  va_end (ap);

  return encoded;
}

gchar *create_json_reply (int status, const char *fmt, ...)
{
  va_list ap;
  gchar  *encoded = NULL;

  va_start (ap, fmt);
  encoded = create_json_request_reply (status, "Reply", fmt, ap);
  va_end (ap);

  return encoded;
}

int get_json_reply_status (const gchar *reply)
{
  int     status = 0;
  gsize   decoded_size = 0;
  gchar  *decoded = NULL;
  json_t *root    = NULL;
  json_error_t error;

  if (!reply)
    return -1;

  decoded = g_base64_decode (reply, &decoded_size);

  if (!decoded)
    return -1;

  root = json_loads (decoded, 0, &error);
  status = get_json_data_integer ("Status", root);
  g_free (decoded);
  json_decref (root);

  return status;
}
