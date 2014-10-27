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

#include "../src/ZonesPool.h"

class ZonesPoolTest : public ::testing::Test {
public:
  ZonesPool zonespool;
};

TEST_F(ZonesPoolTest, CreatedWithEmptyPool)
{
  EXPECT_EQ(zonespool.size (), 0);
}

TEST_F(ZonesPoolTest, AddNewZone)
{
  ASSERT_TRUE(zonespool.zoneAdd ("default") != nullptr);

  EXPECT_EQ(zonespool.size (), 1);
}

TEST_F(ZonesPoolTest, AddDuplicateZoneWillBeIgnored)
{
  shared_ptr<Zone> z = zonespool.zoneAdd ("default");
  ASSERT_TRUE(z != nullptr);
  shared_ptr<Zone> z2 = zonespool.zoneAdd ("default");
  ASSERT_TRUE(z2 != nullptr);

  EXPECT_EQ(z, z2);
}

TEST_F(ZonesPoolTest, GetZoneByName)
{
  ASSERT_TRUE(zonespool.zoneAdd ("default") != nullptr);

  shared_ptr<Zone> z = zonespool.zoneGetByName ("default");
  ASSERT_TRUE(z != nullptr);

  EXPECT_EQ(z->getName (), "default");
}

TEST_F(ZonesPoolTest, GetNotExistingZoneByNameReturnNothing)
{
  ASSERT_TRUE(zonespool.zoneAdd ("default") != nullptr);

  shared_ptr<Zone> z = zonespool.zoneGetByName ("default2");

  EXPECT_EQ(z, nullptr);
}

TEST_F(ZonesPoolTest, GetZoneByIP)
{
  shared_ptr<Zone> z = zonespool.zoneAdd ("default");
  ASSERT_TRUE(z != nullptr);

  z->netAdd (Client::IPv4, "192.168.1.0/24", "office");

  EXPECT_EQ(zonespool.zoneGetByIP ("192.168.1.100"), z);
}

TEST_F(ZonesPoolTest, RemoveZone)
{
  shared_ptr<Zone> z = zonespool.zoneAdd ("default");
  ASSERT_TRUE(z != nullptr);
  ASSERT_EQ(zonespool.size (), 1);

  ASSERT_TRUE(zonespool.zoneRemove ("default"));

  EXPECT_EQ(zonespool.size (), 0);
}

TEST_F(ZonesPoolTest, RemoveNotExistingZoneWillBeIgnored)
{
  shared_ptr<Zone> z = zonespool.zoneAdd ("default");
  ASSERT_TRUE(z != nullptr);
  ASSERT_EQ(zonespool.size (), 1);

  ASSERT_FALSE(zonespool.zoneRemove ("default2"));

  EXPECT_EQ(zonespool.size (), 1);
}
