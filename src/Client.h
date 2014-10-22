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

#ifndef _RH_CLIENT_H
#define _RH_CLIENT_H

#include <string>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <memory>

#include "ClientIP.h"
#include "ClientExtensions.h"

using std::string;
using std::shared_ptr;
using namespace boost::uuids;

typedef struct ClientInfo ClientInfo;

struct ClientInfo {
  uuid     id;
  string   username;
  ClientIP ip;
  shared_ptr<ClientExtensions> ext;
};

class Client {
public:
  Client ();
  ~Client ();

  bool isAuthenticated () { return authenticated; }
  string getId ();
  void   setId (string id);

  string&   getUsername () { return info.username; }
  ClientIP& getClientIP () { return info.ip; }

  shared_ptr<ClientExtensions> getExtensions () { return info.ext; }
  bool startExtensions () {
    if (!getExtensions ())
      info.ext.reset (new ClientExtensions ());

    return getExtensions () != nullptr;
  }
  bool isExtensions () { return getExtensions () != nullptr; }

private:
  bool       authenticated;
  ClientInfo info;
};

inline
Client::Client () : authenticated (false)
{
  info.id = boost::uuids::random_generator()();
}

inline
Client::~Client ()
{
}

#endif // _RH_CLIENT_H
