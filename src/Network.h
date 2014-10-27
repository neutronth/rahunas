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

#ifndef _RH_NETWORK_H
#define _RH_NETWORK_H

#include <string>
#include <sstream>

#include "Client.h"
#include "Netmask.h"

using std::string;
using std::stringstream;
using std::size_t;

class Network {
public:
  enum cidr_max {
    CIDR_IPv4_MAX = 32,
    CIDR_IPv6_MAX = 128
  };

public:
  Network (enum Client::support_family f, string net, string netname);

  enum Client::support_family getFamily () { return family; }

  bool       isValid ()           { return valid;    }
  string&    getName ()           { return netname;  }
  uint8_t    getPrefixLen ()      { return cidr;     }
  IPStorage *getRawNetworkAddr () { return &netaddr; }
  string     getNetworkAddr ();

  bool       isInNetwork (string ip);

private:
  bool valid;
  enum Client::support_family family;
  IPStorage  netaddr;
  uint8_t    cidr;
  string     netname;
};

#endif // _RH_NETWORK_H
