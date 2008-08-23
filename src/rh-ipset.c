/**
 * RahuNAS ipset implementation
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-08-20
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include "rh-ipset.h"

extern struct set **set_list;
extern ip_set_id_t max_sets;

int kernel_getsocket(void)
{
  int sockfd = -1;
  int tries = GETSOCK_TRIES;

  while (tries > 0) {
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sockfd < 0) {
      syslog(LOG_ERR, "Failed kernel_getsocket() errno=%d, Retry....", errno);
      sleep(2);
    } else {
      return sockfd;
    }
    tries--;
  }
  
  exit(EXIT_FAILURE);
}

int wrapped_getsockopt(void *data, socklen_t *size)
{
  int res;
  int sockfd = kernel_getsocket();

  if (sockfd < 0)
    return -1;

  /* Send! */
  res = getsockopt(sockfd, SOL_IP, SO_IP_SET, data, size);
  if (res != 0)
    DP("res=%d errno=%d", res, errno);

  shutdown(sockfd, SHUT_RDWR);

  return res;
}

int wrapped_setsockopt(void *data, socklen_t size)
{
  int res;
  int sockfd = kernel_getsocket();

  if (sockfd < 0)
    return -1;

  /* Send! */
  res = setsockopt(sockfd, SOL_IP, SO_IP_SET, data, size);
  if (res != 0)
    DP("res=%d errno=%d", res, errno);

  shutdown(sockfd, SHUT_RDWR);

  return res; 
}

void kernel_getfrom(void *data, socklen_t * size)
{
  int res = wrapped_getsockopt(data, size);

  if (res != 0)
    DP("res=%d errno=%d", res, errno);
}

int kernel_sendto_handleerrno(unsigned op, void *data, socklen_t size)
{
  int res = wrapped_setsockopt(data, size);

  if (res !=0) {
    DP("res=%d errno=%d", res, errno);
    return -1;
  }

  return 0;
}

void kernel_sendto(void *data, size_t size)
{
  int res = wrapped_setsockopt(data, size);

  if (res != 0)
    DP("res=%d errno=%d", res, errno);
}

int kernel_getfrom_handleerrno(void *data, size_t * size)
{
  int res = wrapped_getsockopt(data, size);

  if (res != 0) {
    DP("res=%d errno=%d", res, errno);
    return -1;
  }

  return 0;
}

struct set *set_adt_get(const char *name)
{
  struct ip_set_req_adt_get req_adt_get;
  struct set *set;
  socklen_t size;

  DP("%s", name);

  req_adt_get.op = IP_SET_OP_ADT_GET;
  req_adt_get.version = IP_SET_PROTOCOL_VERSION;
  strcpy(req_adt_get.set.name, name);
  size = sizeof(struct ip_set_req_adt_get);

  kernel_getfrom((void *) &req_adt_get, &size);

  set = rh_malloc(sizeof(struct set));
  strcpy(set->name, name);
  set->index = req_adt_get.set.index;

  return set;
}


void parse_ip(const char *str, ip_set_ip_t *ip)
{
  struct in_addr addr;

  DP("%s", str);

  if (inet_aton(str, &addr) != 0) {
    *ip = ntohl(addr.s_addr);
    return;
  }
}

void parse_mac(const char *mac, unsigned char *ethernet)
{
  unsigned int i = 0;
  if (strlen(mac) != ETH_ALEN * 3 - 1)
    return;

  DP("%s", mac);

  for (i = 0; i < ETH_ALEN; i++) {
    long number;
    char *end;
    
    number = strtol(mac + i * 3, &end, 16);
    
    if (end == mac + i * 3 + 2 && number >= 0 && number <=255)
      ethernet[i] = number;
    else
      return;
  }
  return;
}

char *ip_tostring(ip_set_ip_t ip)
{
  struct in_addr addr;
  addr.s_addr = htonl(ip);

  return inet_ntoa(addr);
}

char *mac_tostring(unsigned char macaddress[ETH_ALEN])
{
  static char mac_string[18] = "";
 
  sprintf(mac_string, "%02X:%02X:%02X:%02X:%02X:%02X", 
          macaddress[0], macaddress[1], macaddress[2],
          macaddress[3], macaddress[4], macaddress[5]);
  return mac_string; 
}

int set_adtip(struct set *rahunas_set, const char *adtip, const char *adtmac, 
              unsigned op)
{
  ip_set_ip_t ip;
  unsigned char mac[ETH_ALEN] = {0,0,0,0,0,0};
  parse_ip(adtip, &ip);  
  parse_mac(adtmac, &mac);

  return set_adtip_nb(rahunas_set, &ip, mac, op);
}

int set_adtip_nb(struct set *rahunas_set, ip_set_ip_t *adtip, 
                     unsigned char adtmac[ETH_ALEN], unsigned op)
{
  struct ip_set_req_adt *req_adt = NULL;
  struct ip_set_req_rahunas req;

  size_t size;
  void *data;
  int res = 0;

  if (rahunas_set == NULL)
    return -1;

  size = sizeof(struct ip_set_req_adt) + sizeof(struct ip_set_req_rahunas);
  data = rh_malloc(size);

  memcpy(&req.ip, adtip, sizeof(ip_set_ip_t));
  memcpy(&req.ethernet, adtmac, ETH_ALEN);

  req_adt = (struct ip_set_req_adt *) data;
  req_adt->op = op;
  req_adt->index = rahunas_set->index;
  memcpy(data + sizeof(struct ip_set_req_adt), &req, 
           sizeof(struct ip_set_req_rahunas));

  if (kernel_sendto_handleerrno(op, data, size) == -1)
 		switch (op) {
		case IP_SET_OP_ADD_IP:
		  DP("%s:%s is already in set", ip_tostring(adtip), mac_tostring(adtmac));
      res = RH_IS_IN_SET;
			break;
		case IP_SET_OP_DEL_IP:
      DP("%s:%s is not in set", ip_tostring(adtip), mac_tostring(adtmac));
      res = RH_IS_NOT_IN_SET; 
			break;
		case IP_SET_OP_TEST_IP:
      DP("%s:%s is in set", ip_tostring(adtip), mac_tostring(adtmac));
      res = RH_IS_IN_SET;
			break;
		default:
			break;
		}
	else
		switch (op) {
		case IP_SET_OP_TEST_IP:
      DP("%s:%s is not in set", ip_tostring(adtip), mac_tostring(adtmac));
      res = RH_IS_NOT_IN_SET;
			break;
		default:
			break;   
    }

  free(data);

  return res;
}


void set_flush(const char *name)
{
  struct ip_set_req_std req;

  req.op = IP_SET_OP_FLUSH;
  req.version = IP_SET_PROTOCOL_VERSION;
  strcpy(req.name, name);

  kernel_sendto(&req, sizeof(struct ip_set_req_std));
}

size_t load_set_list(const char name[IP_SET_MAXNAMELEN],
			    ip_set_id_t *idx,
			    unsigned op, unsigned cmd)
{
	void *data = NULL;
	struct ip_set_req_max_sets req_max_sets;
	struct ip_set_name_list *name_list;
	struct set *set;
	ip_set_id_t i;
	socklen_t size, req_size;
	int repeated = 0, res = 0;

	DP("%s %s", cmd == CMD_MAX_SETS ? "MAX_SETS"
		    : cmd == CMD_LIST_SIZE ? "LIST_SIZE"
		    : "SAVE_SIZE",
		    name);
	
tryagain:
	if (set_list) {
		for (i = 0; i < max_sets; i++)
			if (set_list[i])
				free(set_list[i]);
		free(set_list);
		set_list = NULL;
	}
	/* Get max_sets */
	req_max_sets.op = IP_SET_OP_MAX_SETS;
	req_max_sets.version = IP_SET_PROTOCOL_VERSION;
	strcpy(req_max_sets.set.name, name);
	size = sizeof(req_max_sets);
	kernel_getfrom(&req_max_sets, &size);

	DP("got MAX_SETS: sets %d, max_sets %d",
	   req_max_sets.sets, req_max_sets.max_sets);

	max_sets = req_max_sets.max_sets;
	set_list = rh_malloc(max_sets * sizeof(struct set *));
	memset(set_list, 0, max_sets * sizeof(struct set *));
	*idx = req_max_sets.set.index;

	if (req_max_sets.sets == 0)
		/* No sets in kernel */
		return 0;

	/* Get setnames */
	size = req_size = sizeof(struct ip_set_req_setnames) 
			  + req_max_sets.sets * sizeof(struct ip_set_name_list);
	data = rh_malloc(size);
	((struct ip_set_req_setnames *) data)->op = op;
	((struct ip_set_req_setnames *) data)->index = *idx;

	res = kernel_getfrom_handleerrno(data, &size);

	if (res != 0 || size != req_size) {
		free(data);
		if (repeated++ < LIST_TRIES)
			goto tryagain;

		DP("Tried to get sets from kernel %d times"
			   " and failed. Please try again when the load on"
			   " the sets has gone down.", LIST_TRIES);
    return -1;
	}
		
	/* Load in setnames */
	size = sizeof(struct ip_set_req_setnames);			
	while (size + sizeof(struct ip_set_name_list) <= req_size) {
		name_list = (struct ip_set_name_list *)
			(data + size);
		set = rh_malloc(sizeof(struct set));
		strcpy(set->name, name_list->name);
		set->index = name_list->index;
		set->id = name_list->id;
		set_list[name_list->index] = set;
		size += sizeof(struct ip_set_name_list);
	}
	/* Size to get set members, bindings */
	size = ((struct ip_set_req_setnames *)data)->size;
	free(data);
	
	return size;
}
