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
#include "../src/Client.h"

class ClientTest : public ::testing::Test {
public:
 Client client;
};

TEST_F(ClientTest, CreatedWithUnauthenticatedStatus)
{
  EXPECT_FALSE(client.isAuthenticated ());
}

TEST_F(ClientTest, CreatedWithNotEmptyClientId)
{
  EXPECT_TRUE(!client.getId ().empty ());
}

TEST_F(ClientTest, CreatedWithSpecificClientId)
{
  client.setId ("01234567-89ab-cdef-0123-456789abcdef");

  EXPECT_EQ(client.getId (), "01234567-89ab-cdef-0123-456789abcdef");
}

TEST_F(ClientTest, CreatedWithEmptyUsername)
{
  EXPECT_TRUE(client.getUsername ().empty ());
}

TEST_F(ClientTest, CreatedWithIPv4AsDefault)
{
  EXPECT_EQ(client.getClientIP ().getVersion (), 4);
}

TEST_F(ClientTest, CreatedWithIPv4DefaultNoIP)
{
  EXPECT_EQ(client.getClientIP ().getIP (), "0.0.0.0");
}

TEST_F(ClientTest, CreatedWithIPv4SetNewIP)
{
  ASSERT_TRUE(client.getClientIP ().setIP ("192.168.1.100", ClientIP::IPv4));

  EXPECT_EQ(client.getClientIP ().getIP (), "192.168.1.100");
}

TEST_F(ClientTest, CreatedWithIPv6SetNewIP)
{
  ASSERT_TRUE(client.getClientIP ().setIP ("2001::1", ClientIP::IPv6));

  EXPECT_EQ(client.getClientIP ().getIP (), "2001::1");
}

TEST_F(ClientTest, CreatedWithInvalidIPv4RevertToNoIP)
{
  ASSERT_FALSE(client.getClientIP ().setIP ("192.168.1.100.3", ClientIP::IPv4));

  EXPECT_EQ(client.getClientIP ().getIP (), "0.0.0.0");
}

TEST_F(ClientTest, CreatedWithInvalidIPv6RevertToNoIP)
{
  ASSERT_FALSE(client.getClientIP ().setIP ("20112::1", ClientIP::IPv6));

  EXPECT_EQ(client.getClientIP ().getIP (), "::");
}

TEST_F(ClientTest, CreatedWithNoClientExtensions)
{
  EXPECT_TRUE(client.getExtensions () == nullptr);
}

TEST_F(ClientTest, CreatedWithClientExtensionsEnabled)
{
  ASSERT_TRUE(client.startExtensions ());

  EXPECT_TRUE(client.isExtensions ());
}

TEST_F(ClientTest, CreatedWithClientExtensionsAndDefaultMAC)
{
  ASSERT_TRUE(client.startExtensions ());

  EXPECT_EQ(client.getExtensions ()->getMAC (), "00:00:00:00:00:00");
}

TEST_F(ClientTest, CreatedWithClientExtensionsAndSetupMAC)
{
  ASSERT_TRUE(client.startExtensions ());
  ASSERT_TRUE(client.getExtensions ()->setMAC ("aa:bb:cc:dd:ee:ff"));

  EXPECT_EQ(client.getExtensions ()->getMAC (), "aa:bb:cc:dd:ee:ff");
}

TEST_F(ClientTest, CreatedWithClientExtensionsAndNoMaxSpeedUploadDownload)
{
  ASSERT_TRUE(client.startExtensions ());

  EXPECT_EQ(client.getExtensions ()->getMaxSpeedDownload (), 0);
  EXPECT_EQ(client.getExtensions ()->getMaxSpeedUpload (), 0);
}

TEST_F(ClientTest, CreatedWithClientExtensionsAndNoMinSpeedUploadDownload)
{
  ASSERT_TRUE(client.startExtensions ());

  EXPECT_EQ(client.getExtensions ()->getMinSpeedDownload (), 0);
  EXPECT_EQ(client.getExtensions ()->getMinSpeedUpload (), 0);
}

TEST_F(ClientTest, CreatedWithClientExtensionsAndSetupMaxSpeedUploadDownload)
{
  ASSERT_TRUE(client.startExtensions ());
  ASSERT_TRUE(client.getExtensions () != nullptr);

  client.getExtensions ()->setMaxSpeedDownload (5000000);
  client.getExtensions ()->setMaxSpeedUpload (512000);

  EXPECT_EQ(client.getExtensions ()->getMaxSpeedDownload (), 5000000);
  EXPECT_EQ(client.getExtensions ()->getMaxSpeedUpload (), 512000);
}

TEST_F(ClientTest, CreatedWithClientExtensionsAndSetupMinSpeedUploadDownload)
{
  ASSERT_TRUE(client.startExtensions ());
  ASSERT_TRUE(client.getExtensions () != nullptr);

  client.getExtensions ()->setMinSpeedDownload (1000000);
  client.getExtensions ()->setMinSpeedUpload (256000);

  EXPECT_EQ(client.getExtensions ()->getMinSpeedDownload (), 1000000);
  EXPECT_EQ(client.getExtensions ()->getMinSpeedUpload (), 256000);
}
