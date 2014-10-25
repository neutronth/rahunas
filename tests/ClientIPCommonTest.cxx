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

class ClientIPCommonTest : public ::testing::Test {
public:
  void SetUp () {
    client = ClientFactory::getClient (Client::IPv4, "192.168.1.100");
  }
public:
  shared_ptr<Client> client;
};

TEST_F(ClientIPCommonTest, CreatedWithNotEmptyClientId)
{
  EXPECT_TRUE(!client->getId ().empty ());
}

TEST_F(ClientIPCommonTest, CreatedWithSpecificClientId)
{
  client->setId ("01234567-89ab-cdef-0123-456789abcdef");

  EXPECT_EQ(client->getId (), "01234567-89ab-cdef-0123-456789abcdef");
}

TEST_F(ClientIPCommonTest, CreatedWithNoClientExtensions)
{
  EXPECT_TRUE(client->getExtensions () == nullptr);
}

TEST_F(ClientIPCommonTest, CreatedWithClientExtensionsEnabled)
{
  ASSERT_TRUE(client->startExtensions ());

  EXPECT_TRUE(client->isExtensions ());
}

TEST_F(ClientIPCommonTest, CreatedWithClientExtensionsAndDefaultMAC)
{
  ASSERT_TRUE(client->startExtensions ());

  EXPECT_EQ(client->getExtensions ()->getMAC (), "00:00:00:00:00:00");
}

TEST_F(ClientIPCommonTest, CreatedWithClientExtensionsAndSetupMAC)
{
  ASSERT_TRUE(client->startExtensions ());
  ASSERT_TRUE(client->getExtensions ()->setMAC ("aa:bb:cc:dd:ee:ff"));

  EXPECT_EQ(client->getExtensions ()->getMAC (), "aa:bb:cc:dd:ee:ff");
}

TEST_F(ClientIPCommonTest, CreatedWithClientExtensionsAndNoMaxSpeedUploadDownload)
{
  ASSERT_TRUE(client->startExtensions ());

  EXPECT_EQ(client->getExtensions ()->getMaxSpeedDownload (), 0);
  EXPECT_EQ(client->getExtensions ()->getMaxSpeedUpload (), 0);
}

TEST_F(ClientIPCommonTest, CreatedWithClientExtensionsAndNoMinSpeedUploadDownload)
{
  ASSERT_TRUE(client->startExtensions ());

  EXPECT_EQ(client->getExtensions ()->getMinSpeedDownload (), 0);
  EXPECT_EQ(client->getExtensions ()->getMinSpeedUpload (), 0);
}

TEST_F(ClientIPCommonTest, CreatedWithClientExtensionsAndSetupMaxSpeedUploadDownload)
{
  ASSERT_TRUE(client->startExtensions ());
  ASSERT_TRUE(client->getExtensions () != nullptr);

  client->getExtensions ()->setMaxSpeedDownload (5000000);
  client->getExtensions ()->setMaxSpeedUpload (512000);

  EXPECT_EQ(client->getExtensions ()->getMaxSpeedDownload (), 5000000);
  EXPECT_EQ(client->getExtensions ()->getMaxSpeedUpload (), 512000);
}

TEST_F(ClientIPCommonTest, CreatedWithClientExtensionsAndSetupMinSpeedUploadDownload)
{
  ASSERT_TRUE(client->startExtensions ());
  ASSERT_TRUE(client->getExtensions () != nullptr);

  client->getExtensions ()->setMinSpeedDownload (1000000);
  client->getExtensions ()->setMinSpeedUpload (256000);

  EXPECT_EQ(client->getExtensions ()->getMinSpeedDownload (), 1000000);
  EXPECT_EQ(client->getExtensions ()->getMinSpeedUpload (), 256000);
}
