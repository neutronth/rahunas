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

#include "ZonesPool.h"

shared_ptr<Zone>
ZonesPool::zoneAdd (string name)
{
  for (auto z : zones)
    {
      if (z->getName () == name)
        return z;
    }

  shared_ptr<Zone> new_zone = make_shared<Zone> (name);

  if (new_zone)
    {
      zones.push_back (new_zone);
      return new_zone;
    }

  return nullptr;
}

shared_ptr<Zone>
ZonesPool::zoneGetByName (string name)
{
  for (auto z : zones)
    {
      if (z->getName () == name)
        return z;
    }

  return nullptr;
}

shared_ptr<Zone>
ZonesPool::zoneGetByIP (string ip)
{
  for (auto z : zones)
    {
      if (z->netFindByIP (ip))
        return z;
    }

  return nullptr;
}

bool
ZonesPool::zoneRemove (string name)
{
  for (auto it = zones.begin (); it != zones.end (); ++it)
    {
      if ((*it)->getName () == name)
        {
          zones.erase (it);
          return true;
        }
    }

  return false;
}
