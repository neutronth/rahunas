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

#ifndef _RH_ZONES_POOL_H
#define _RH_ZONES_POOL_H

#include <string>
#include <memory>
#include <vector>

#include "Zone.h"

using std::string;
using std::shared_ptr;
using std::vector;

class ZonesPool {
public:
  ZonesPool ();
  ~ZonesPool ();

  unsigned int size ()    { return zones.size (); }

  shared_ptr<Zone> zoneAdd       (string name);
  shared_ptr<Zone> zoneGetByName (string name);
  shared_ptr<Zone> zoneGetByIP   (string ip);
  bool             zoneRemove    (string name);

private:
  vector<shared_ptr<Zone>> zones;
};

inline
ZonesPool::ZonesPool ()
{
}

inline
ZonesPool::~ZonesPool ()
{
}

#endif // _RH_ZONES_POOL_H
