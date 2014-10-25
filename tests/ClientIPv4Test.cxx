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

#include <gtest/gtest.h>
#include <memory>
#include "../src/Client.h"
#include "../src/ClientFactory.h"

using std::shared_ptr;

class ClientIPv4Test : public ::testing::Test {
public:
  void SetUp () {
    client = ClientFactory::getClient (Client::IPv4, "192.168.1.100");
  }
public:
  shared_ptr<Client> client;
};

class ClientInvalidIPv4Test : public ::testing::Test {
public:
  void SetUp () {
    client = ClientFactory::getClient (Client::IPv4, "192.168.1.300");
  }
public:
  shared_ptr<Client> client;
};

TEST_F(ClientIPv4Test, CreatedAndValid)
{
  EXPECT_TRUE(client->isValid ());
}

TEST_F(ClientIPv4Test, CreatedAndIPIsCorrect)
{
  EXPECT_EQ(client->getIP (), "192.168.1.100");
}

TEST_F(ClientInvalidIPv4Test, CreatedAndInvalidNoPtrReturn)
{
  EXPECT_EQ(client, nullptr);
}
