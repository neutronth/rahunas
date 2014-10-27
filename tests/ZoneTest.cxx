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

#include "../src/Zone.h"

using std::shared_ptr;
using std::make_shared;

class ZoneTest : public ::testing::Test {
public:
  void SetUp() override {
    zone =  make_shared<Zone> ("default");
  }

  shared_ptr<Zone> zone;
};

TEST_F(ZoneTest, CreatedWithZoneName)
{
  EXPECT_EQ(zone->getName (), "default");
}

TEST_F(ZoneTest, CreatedWithZoneRename)
{
  ASSERT_TRUE(zone->setName ("mahasarakham"));
  EXPECT_EQ(zone->getName (), "mahasarakham");
}

TEST_F(ZoneTest, AddANetwork)
{
  ASSERT_EQ(zone->getName (), "default");
  ASSERT_TRUE(zone->netAdd (Client::IPv4, "192.168.1.0/24", "office"));

  EXPECT_EQ(zone->netCount (), 1);
}

TEST_F(ZoneTest, AddANetworkWithNoDuplicate)
{
  ASSERT_EQ(zone->getName (), "default");
  ASSERT_TRUE(zone->netAdd (Client::IPv4, "192.168.1.0/24", "office"));
  ASSERT_FALSE(zone->netAdd (Client::IPv4, "192.168.1.0/24", "office"));

  EXPECT_EQ(zone->netCount (), 1);
}

TEST_F(ZoneTest, AddNetworks)
{
  ASSERT_EQ(zone->getName (), "default");
  ASSERT_TRUE(zone->netAdd (Client::IPv4, "192.168.1.0/24", "office"));
  ASSERT_TRUE(zone->netAdd (Client::IPv6, "2001:99:99::/96", "lobby"));

  EXPECT_EQ(zone->netCount (), 2);
}

TEST_F(ZoneTest, RemoveANetworkByNetwork)
{
  ASSERT_EQ(zone->getName (), "default");
  ASSERT_TRUE(zone->netAdd (Client::IPv4, "192.168.1.0/24", "office"));
  ASSERT_TRUE(zone->netRemoveByNetwork ("192.168.1.0/24"));

  EXPECT_EQ(zone->netCount (), 0);
}

TEST_F(ZoneTest, RemoveANetworkByName)
{
  ASSERT_EQ(zone->getName (), "default");
  ASSERT_TRUE(zone->netAdd (Client::IPv4, "192.168.1.0/24", "office"));
  ASSERT_TRUE(zone->netRemoveByName ("office"));

  EXPECT_EQ(zone->netCount (), 0);
}

TEST_F(ZoneTest, RemoveANotExistingNetworkWillBeIgnored)
{
  ASSERT_EQ(zone->getName (), "default");
  ASSERT_TRUE(zone->netAdd (Client::IPv4, "192.168.1.0/24", "office"));

  ASSERT_FALSE(zone->netRemoveByName ("office2"));
  ASSERT_FALSE(zone->netRemoveByNetwork ("192.168.100.0/24"));

  EXPECT_EQ(zone->netCount (), 1);
}

TEST_F(ZoneTest, FindNetworkByNetwork)
{
  ASSERT_EQ(zone->getName (), "default");
  ASSERT_TRUE(zone->netAdd (Client::IPv4, "192.168.1.0/24", "office"));

  EXPECT_TRUE(zone->netFindByNetwork ("192.168.1.0/24") != nullptr);
}

TEST_F(ZoneTest, FindNetworkByName)
{
  ASSERT_EQ(zone->getName (), "default");
  ASSERT_TRUE(zone->netAdd (Client::IPv4, "192.168.1.0/24", "office"));

  EXPECT_TRUE(zone->netFindByName ("office") != nullptr);
}

TEST_F(ZoneTest, FindNetworkByIP)
{
  ASSERT_EQ(zone->getName (), "default");
  ASSERT_TRUE(zone->netAdd (Client::IPv4, "192.168.1.0/24", "office"));

  EXPECT_TRUE(zone->netFindByIP ("192.168.1.100") != nullptr);
}

TEST_F(ZoneTest, RenameNetworkByAddSameNetworkWithDifferentName)
{
  ASSERT_EQ(zone->getName (), "default");
  ASSERT_TRUE(zone->netAdd (Client::IPv4, "192.168.1.0/24", "office"));
  shared_ptr<Network> n = zone->netFindByName ("office");

  ASSERT_TRUE(zone->netAdd (Client::IPv4, "192.168.1.0/24", "newoffice"));
  shared_ptr<Network> n2 = zone->netFindByName ("newoffice");

  EXPECT_EQ(n, n2);
}
