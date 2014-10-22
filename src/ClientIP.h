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

#ifndef _RH_CLIENT_IP_H
#define _RH_CLIENT_IP_H

#include <string>
#include <cstring>
#include <netinet/in.h>

using std::string;

class ClientIP {
public:
  enum support_family {
    IPv4 = AF_INET,
    IPv6 = AF_INET6
  };

public:
  ClientIP ();

  unsigned short getVersion () { return family == AF_INET6 ? 6 : 4; };
  string         getIP ();
  bool           setIP (string ip, enum support_family f);

private:
  enum support_family family;

  union {
    struct in_addr  ip;
    struct in6_addr ip6;
  };
};

inline
ClientIP::ClientIP () : family (IPv4)
{
  memset (&ip6, 0, sizeof (ip6));
}

#endif // _RH_CLIENT_IP_H
