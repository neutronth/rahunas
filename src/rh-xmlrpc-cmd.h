/**
 * XML-RPC client command sender header
 * Author: Neutron Soutmun <neo.neutron@gmail.com>
 * Date:   2008-09-02
 */
#ifndef __RH_XMLRPC_CMD_H
#define __RH_XMLRPC_CMD_H
#include "rh-radius.h"
#include "rh-macauthen.h"

int send_xmlrpc_stopacct(RHVServer *vs, uint32_t id, int cause);
int send_xmlrpc_interimupdate(RHVServer *vs, uint32_t id);
int send_xmlrpc_offacct(RHVServer *vs);
int send_xmlrpc_macauthen(MACAuthenElem *elem);

#endif // __RH_XMLRPC_CMD_H
