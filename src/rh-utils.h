/**
 * utility functions header
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-08-31
 */
#ifndef __RH_UTILS_H
#define __RH_UTILS_H

#include <stdlib.h>

void *rh_malloc(size_t size);
void rh_free(void **data);

const char *rh_string_get_sep(const char *haystack, const char *sep, 
                              unsigned short idx);

#endif // __RH_UTILS_H
