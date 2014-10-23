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

#include <sstream>
#include <boost/uuid/uuid_io.hpp>

#include "Session.h"

using std::stringstream;

string
Session::getId ()
{
  stringstream ss;
  ss << id;

  return ss.str ();
}

void
Session::setId (string s_id)
{
  stringstream ss (s_id);
  ss >> id;
}

shared_ptr<Client>
Session::clientAdd (string ip, enum ClientIP::support_family f)
{
  shared_ptr<Client> c;
  c = clientFind (ip);

  if (c)
    return c;

  c.reset(new Client ());
  if (!c->getClientIP ().setIP (ip, f))
    return nullptr;

  clients.push_back (c);

  return c;
}

shared_ptr<Client>
Session::clientMove (string ip, string to_ip, enum ClientIP::support_family f)
{
  shared_ptr<Client> c;
  c = clientFind (ip);

  if (c)
    c->getClientIP().setIP (to_ip, f);

  return c;
}

shared_ptr<Client>
Session::clientFind (string ip)
{
  for (shared_ptr<Client> c : clients)
    {
      if (c->getClientIP ().getIP () == ip)
        return c;
    }

  return nullptr;
}
