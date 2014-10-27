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

#include "../src/Network.h"

using std::shared_ptr;
using std::make_shared;

class NetworkIPv4Test : public ::testing::Test {
public:
  void SetUp() override {
    network =  make_shared<Network> (Client::IPv4, "192.168.1.0/24", "office");
  }

  shared_ptr<Network> network;
};

class NetworkIPv6Test : public ::testing::Test {
public:
  void SetUp() override {
    network =  make_shared<Network> (Client::IPv6, "2001:99:99::/96", "lobby");
  }

  shared_ptr<Network> network;
};

TEST_F(NetworkIPv4Test, CreatedWithValidNetwork)
{
  EXPECT_TRUE(network->isValid ());
}

TEST_F(NetworkIPv4Test, CreatedWithNetName)
{
  EXPECT_EQ(network->getName (), "office");
}

TEST_F(NetworkIPv4Test, GetCorrectNetworkAddress)
{
  ASSERT_EQ(network->getPrefixLen (), 24);
  EXPECT_EQ(network->getNetworkAddr (), "192.168.1.0");
}

TEST_F(NetworkIPv4Test, GetCorrectRawNetworkAddress)
{
  ASSERT_EQ(network->getFamily (), Client::IPv4);
  EXPECT_EQ(ntohl(network->getRawNetworkAddr ()->ip), 0xc0a80100);
}

TEST_F(NetworkIPv4Test, IPAddressIsInNetwork)
{
  EXPECT_TRUE(network->isInNetwork ("192.168.1.100"));
}

TEST_F(NetworkIPv6Test, CreatedWithValidNetwork)
{
  EXPECT_TRUE(network->isValid ());
}

TEST_F(NetworkIPv6Test, CreatedWithNetName)
{
  EXPECT_EQ(network->getName (), "lobby");
}

TEST_F(NetworkIPv6Test, GetCorrectNetworkAddress)
{
  ASSERT_EQ(network->getPrefixLen (), 96);
  EXPECT_EQ(network->getNetworkAddr (), "2001:99:99::");
}

TEST_F(NetworkIPv6Test, GetCorrectRawNetworkAddress)
{
  ASSERT_EQ(network->getFamily (), Client::IPv6);

  EXPECT_EQ(ntohl(network->getRawNetworkAddr ()->ip6[0]), 0x20010099);
  EXPECT_EQ(ntohl(network->getRawNetworkAddr ()->ip6[1]), 0x00990000);
  EXPECT_EQ(ntohl(network->getRawNetworkAddr ()->ip6[2]), 0x0);
  EXPECT_EQ(ntohl(network->getRawNetworkAddr ()->ip6[3]), 0x0);
}

TEST_F(NetworkIPv6Test, IPAddressIsInNetwork)
{
  EXPECT_TRUE(network->isInNetwork ("2001:99:99::100"));
}
