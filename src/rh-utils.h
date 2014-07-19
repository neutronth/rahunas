/**
 * utility functions header
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-08-31
 */
#ifndef __RH_UTILS_H
#define __RH_UTILS_H

#include <stdlib.h>

#define LOG_MAIN_FD  -1

void *rh_malloc(size_t size);
void rh_free(void **data);

gchar *rh_string_get_sep(const char *haystack, const char *sep,
                         unsigned short idx);
int rh_openlog(const char *filename);
int rh_closelog(int fd);
int rh_logselect(int fd);
int rh_writepid(const char *pidfile, int pid); 
int logmsg(int priority, const char *msg, ...);

uint32_t iptoid(struct rahunas_map *map, const char *ip);
const char *idtoip(struct rahunas_map *map, uint32_t id);

int rh_cmd_exec(const char *cmd, char *const args[], char *const envs[],
                char *result_buffer, int buffer_size);

#endif // __RH_UTILS_H
