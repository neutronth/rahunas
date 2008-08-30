/**
 * utility functions header
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-08-31
 */

#include <syslog.h>
#include <glib.h>

#include "rh-utils.h"

void *rh_malloc(size_t size)
{
  void *p;
  
  if (size == 0)
    return NULL;

  if ((p = malloc(size)) == NULL) {
    syslog(LOG_ERR, "RahuNASd: not enough memory");
    exit(EXIT_FAILURE);
  } 

  return p;
}

void rh_free(void **data)
{
  if (*data == NULL)
    return;

  free(*data);
  *data = NULL;
}

const char *rh_string_get_sep(const char *haystack, const char *sep, 
                              unsigned short idx)
{
  char* result = NULL;
  gchar *pStart = NULL;
  gchar *pEnd   = NULL;
  gchar *pLast  = NULL;
  unsigned short current_idx = 0;
  unsigned short isFound = 0;

  if (!haystack) {
    return NULL;
  }

  pEnd = haystack;
  pLast = haystack + strlen(haystack);
  while (!isFound) {
    pStart = pEnd;
    if (pStart == pLast) {
      // Finding the separator fail
      return NULL;
    }

    pEnd = g_strstr_len(pStart, strlen(pStart), sep);  
    pEnd = pEnd == NULL ? pLast : pEnd;

    current_idx++;

    if (current_idx == idx) {
      result = g_strndup(pStart, pEnd - pStart);
      return result;
    } else {
      pEnd++;
    }
  }
}
