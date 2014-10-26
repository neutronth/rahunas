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
#include <sstream>
#include <boost/uuid/uuid_io.hpp>

#include "SessionsPool.h"

using std::stringstream;
using std::make_shared;
using std::make_pair;

shared_ptr<Session>
SessionsPool::sessionGetByIP (enum Client::support_family f, string ip)
{
  auto s = ip_pool.find (ip);

  if (s == ip_pool.end ())
    {
      shared_ptr<Session> new_s = make_shared<Session> ();

      if (new_s && new_s->clientAdd (f, ip))
        {
          uuid_pool.insert (make_pair (new_s->getRawId (), new_s));
          ip_pool.insert (make_pair (string(ip), new_s));
          return new_s;
        }
    }
  else
    {
      return s->second;
    }

  return nullptr;
}

shared_ptr<Session>
SessionsPool::sessionGetById (string id)
{
  stringstream ss (id);
  uuid rawid;
  ss >> rawid;

  auto s = uuid_pool.find (rawid);

  if (s != uuid_pool.end ())
    {
      return s->second;
    }

  return nullptr;
}

bool
SessionsPool::sessionRemove (string id)
{
  stringstream ss (id);
  uuid rawid;
  ss >> rawid;

  auto s = sessionGetById (id);
  if (s)
    {
      for (auto c : s->getClients ())
        {
          ip_pool.erase (c->getIP ());
        }

      uuid_pool.erase (rawid);
    }

  return true;
}
