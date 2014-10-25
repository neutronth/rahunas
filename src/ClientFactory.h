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
#ifndef _RH_CLIENT_FACTORY_H
#define _RH_CLIENT_FACTORY_H

#include "Client.h"
#include "ClientIPv4.h"
#include "ClientIPv6.h"

class ClientFactory {
public:
  static shared_ptr<Client> getClient (enum Client::support_family f,
                                       string setup_ip)
  {
    shared_ptr<Client> c;
    switch (f)
      {
        case Client::IPv4:
        default:
          c = make_shared<ClientIPv4> (setup_ip);
          break;
        case Client::IPv6:
          c = make_shared<ClientIPv6> (setup_ip);
          break;
      }

    return c->isValid () ? c : nullptr;
  }
};

#endif // _RH_CLIENT_FACTORY_H
