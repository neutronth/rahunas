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

#include <arpa/inet.h>

#include "ClientIP.h"

string
ClientIP::getIP ()
{
  char ipstr[INET6_ADDRSTRLEN];

  const char *ret = nullptr;

  if (family == AF_INET)
    ret = inet_ntop (family, &ip, ipstr, INET6_ADDRSTRLEN);
  else
    ret = inet_ntop (family, &ip6, ipstr, INET6_ADDRSTRLEN);

  return ret ? string (ipstr) :
               family == AF_INET ? string ("0.0.0.0") : string ("::");
}

bool
ClientIP::setIP (string ip, enum support_family f)
{
  family = f;

  if (inet_pton (family, ip.c_str (), &ip6) > 0)
    return true;

  return false;
}
