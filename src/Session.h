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

#ifndef _RH_SESSION_H
#define _RH_SESSION_H

#include <string>
#include <memory>
#include <vector>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "Client.h"

using std::string;
using std::vector;
using std::shared_ptr;
using namespace boost::uuids;

class Session {
public:
  Session ();

  bool isAuthenticated ()   { return authenticated; }
  void markAuthenticated () { authenticated = true; }

  string getId ();
  void   setId (string s_id);

  unsigned int clientCount () { return clients.size (); }

  shared_ptr<Client> clientAdd  (string ip, enum ClientIP::support_family f);
  shared_ptr<Client> clientMove (string ip, string to_ip,
                                 enum ClientIP::support_family f);
  shared_ptr<Client> clientFind (string ip);


private:
  bool authenticated;
  uuid id;
  vector<shared_ptr<Client>> clients;
};

inline
Session::Session () :
  authenticated (false)
{
  id = boost::uuids::random_generator()();
}

#endif // _RH_SESSION_H
