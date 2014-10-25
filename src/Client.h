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
#include <sstream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <memory>
#include <netinet/in.h>

#include "ClientExtensions.h"

using std::string;
using std::stringstream;
using std::shared_ptr;
using std::make_shared;
using namespace boost::uuids;

class Client {
public:
  enum support_family {
    IPv4 = AF_INET,
    IPv6 = AF_INET6
  };

public:
  Client ();
  virtual ~Client ();

  string getId ();
  void   setId (string id);

  virtual unsigned short getIPFamily () = 0;
  virtual string         getIP () = 0;

  shared_ptr<ClientExtensions> getExtensions () { return ext; }
  bool startExtensions ();
  bool isExtensions () { return getExtensions () != nullptr; }

  bool isValid () { return valid; }

protected:
  bool valid;
  uuid id;
  shared_ptr<ClientExtensions> ext;
};

inline
Client::Client ()
  : valid (false)
{
  id = boost::uuids::random_generator()();
}

inline
Client::~Client ()
{
}

inline
bool Client::startExtensions ()
{
  if (!getExtensions ())
    ext.reset (new ClientExtensions ());

  return getExtensions () != nullptr;
}

inline
string Client::getId ()
{
  stringstream ss;
  ss << id;

  return ss.str ();
}

inline
void Client::setId (string s_id)
{
  stringstream ss (s_id);
  ss >> id;
}


#endif // _RH_CLIENT_H
