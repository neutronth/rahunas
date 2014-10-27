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

#include <memory>

#include "Zone.h"

using std::make_shared;

bool
Zone::netAdd (enum Client::support_family f, string net, string netname)
{
  shared_ptr<Network> new_net = make_shared<Network> (f, net, netname);

  if (!new_net)
    return false;

  for (auto n : networks)
    {
      if (new_net->getNetworkAddr () == n->getNetworkAddr ())
        {
          if (new_net->getName () != n->getName ())
            {
              n->setName (netname);
              return true;
            }

          return false;
        }
    }

  if (new_net && new_net->isValid ())
    {
      networks.push_back (new_net);
      return true;
    }

  return false;
}

bool
Zone::netRemoveByNetwork (string net)
{
  for (auto it = networks.begin (); it != networks.end (); ++it)
    {
      auto n = (*it);
      shared_ptr<Network> new_net = make_shared<Network> (n->getFamily (), net,
                                                          "temp");

      if (new_net && new_net->isValid () &&
          new_net->getNetworkAddr () == n->getNetworkAddr ())
        {
          networks.erase (it);
          return true;
        }
    }

  return false;
}

bool
Zone::netRemoveByName (string netname)
{
  for (auto it = networks.begin (); it != networks.end (); ++it)
    {
      if ((*it)->getName () == netname)
        {
          networks.erase (it);
          return true;
        }
    }

  return false;
}

shared_ptr<Network>
Zone::netFindByNetwork (string net)
{
  for (auto n : networks)
    {
      shared_ptr<Network> f_net = make_shared<Network> (n->getFamily (), net,
                                                        "temp");

      if (f_net->getNetworkAddr () == n->getNetworkAddr ())
        return n;
    }

  return nullptr;
}

shared_ptr<Network>
Zone::netFindByName (string netname)
{
  for (auto n : networks)
    {
      if (n->getName () == netname)
        return n;
    }

  return nullptr;
}

shared_ptr<Network>
Zone::netFindByIP (string ip)
{
  for (auto n : networks)
    {
      if (n->isInNetwork (ip))
        return n;
    }

  return nullptr;
}
