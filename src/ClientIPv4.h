/*************************************************************************

  Copyright (C) 2014  Neutron Soutmun <neutron@rahunas.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*************************************************************************/

#ifndef _RH_CLIENT_IPV4_H
#define _RH_CLIENT_IPV4_H

#include <arpa/inet.h>
#include <netinet/in.h>

#include "Client.h"

class ClientIPv4 : public Client {
public:
  ClientIPv4 (string setup_ip);
  virtual ~ClientIPv4 ();

  virtual unsigned short getIPFamily () { return family; }
  virtual string         getIP ();

private:
  enum Client::support_family family;
  struct in_addr ip;
};

inline
ClientIPv4::ClientIPv4 (string setup_ip)
  : family (Client::IPv4)
{
  if (inet_pton (family, setup_ip.c_str (), &ip) > 0)
    valid = true;
}

inline
ClientIPv4::~ClientIPv4 ()
{
}

#endif // _RH_CLIENT_IPV4_H
