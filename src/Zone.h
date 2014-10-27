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

#ifndef _RH_ZONE_H
#define _RH_ZONE_H

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#include "Network.h"
#include "Client.h"

using std::string;
using std::vector;
using std::unordered_map;
using std::shared_ptr;

class Zone {
public:
  Zone (string n);

  string& getName () { return name; }
  bool    setName (string n);

  unsigned int netCount () { return networks.size (); }
  bool         netAdd (enum Client::support_family f, string net,
                       string netname);
  bool         netRemoveByNetwork (string net);
  bool         netRemoveByName    (string netname);

  shared_ptr<Network> netFindByNetwork (string net);
  shared_ptr<Network> netFindByName    (string netname);
  shared_ptr<Network> netFindByIP      (string ip);

private:
  string name;
  vector<shared_ptr<Network>> networks;
};

inline
Zone::Zone (string n)
  : name (n)
{
}

inline bool
Zone::setName (string n)
{
  name.assign (n);
  return name == n;
}

#endif // _RH_ZONE_H
