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

#ifndef _RH_SESSIONS_POOL_H
#define _RH_SESSIONS_POOL_H

#include <string>
#include <unordered_map>
#include <memory>
#include <boost/functional/hash.hpp>

#include "Session.h"
#include "Client.h"

using std::string;
using std::unordered_map;
using std::shared_ptr;
using boost::hash;
using namespace boost::uuids;

class SessionsPool {
public:
  SessionsPool ();
  ~SessionsPool ();

  unsigned int size ()    { return uuid_pool.size (); }
  unsigned int ipCount () { return ip_pool.size ();   }

  shared_ptr<Session> sessionGetByIP (enum Client::support_family f, string ip);
  shared_ptr<Session> sessionGetById (string id);
  bool                sessionRemove  (string id);

private:
  unordered_map<uuid, shared_ptr<Session>, hash<uuid>> uuid_pool;
  unordered_map<string, shared_ptr<Session>> ip_pool;
};

inline
SessionsPool::SessionsPool ()
{
}

inline
SessionsPool::~SessionsPool ()
{
}

#endif // _RH_SESSIONS_POOL_H
