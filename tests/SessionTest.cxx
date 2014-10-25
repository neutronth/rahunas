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

#include "../src/Session.h"

class SessionTest : public ::testing::Test {
public:
  Session session;
};

TEST_F(SessionTest, CreatedWithUnauthenticatedStatus)
{
  EXPECT_FALSE(session.isAuthenticated ());
}

TEST_F(SessionTest, CreatedAndMarkAuthenticated)
{
  session.markAuthenticated ();
  EXPECT_TRUE(session.isAuthenticated ());
}

TEST_F(SessionTest, CreatedWithNotEmptySessionId)
{
  EXPECT_TRUE(!session.getId ().empty ());
}

TEST_F(SessionTest, CreatedWithSpecificSessionId)
{
  session.setId ("01234567-89ab-cdef-0123-456789abcdef");

  EXPECT_EQ(session.getId (), "01234567-89ab-cdef-0123-456789abcdef");
}

TEST_F(SessionTest, CreatedWithNoClients)
{
  EXPECT_EQ(session.clientCount (), 0);
}

TEST_F(SessionTest, CreatedAndAddIPv4Client)
{
  shared_ptr<Client> c = session.clientAdd (Client::IPv4, "192.168.1.100");

  ASSERT_TRUE (c != nullptr);

  EXPECT_EQ(session.clientCount (), 1);
}

TEST_F(SessionTest, CreatedAndAddIPv6Client)
{
  shared_ptr<Client> c = session.clientAdd (Client::IPv6, "2001::100");

  ASSERT_TRUE (c != nullptr);

  EXPECT_EQ(session.clientCount (), 1);
}

TEST_F(SessionTest, CreatedAndAddBothIPv4IPv6Client)
{
  shared_ptr<Client> c;
  c = session.clientAdd (Client::IPv4, "192.168.1.100");
  ASSERT_TRUE (c != nullptr);
  c = session.clientAdd (Client::IPv6, "2001::100");
  ASSERT_TRUE (c != nullptr);

  EXPECT_EQ(session.clientCount (), 2);
}

TEST_F(SessionTest, CreatedAndAddIPv4ClientNoDuplicate)
{
  shared_ptr<Client> c = session.clientAdd (Client::IPv4, "192.168.1.100");
  ASSERT_TRUE (c != nullptr);

  shared_ptr<Client> c2 = session.clientAdd (Client::IPv4, "192.168.1.100");
  ASSERT_TRUE (c2 == c);

  EXPECT_EQ(session.clientCount (), 1);
}

TEST_F(SessionTest, CreatedAndAddIPv6ClientNoDuplicate)
{
  shared_ptr<Client> c = session.clientAdd (Client::IPv6, "2001::100");
  ASSERT_TRUE (c != nullptr);

  shared_ptr<Client> c2 = session.clientAdd (Client::IPv6, "2001::100");
  ASSERT_TRUE (c2 == c);

  EXPECT_EQ(session.clientCount (), 1);
}

TEST_F(SessionTest, CreatedAndFindIPv4Client)
{
  shared_ptr<Client> c = session.clientAdd (Client::IPv4, "192.168.1.100");
  ASSERT_TRUE (c != nullptr);

  EXPECT_EQ(session.clientFind ("192.168.1.100"), c);
}

TEST_F(SessionTest, CreatedAndFindIPv6Client)
{
  shared_ptr<Client> c = session.clientAdd (Client::IPv6, "2001::100");
  ASSERT_TRUE (c != nullptr);

  EXPECT_EQ(session.clientFind ("2001::100"), c);
}

TEST_F(SessionTest, CreatedAndAddClientsThenRemove)
{
  shared_ptr<Client> c = session.clientAdd (Client::IPv4, "192.168.1.100");
  ASSERT_TRUE(c != nullptr);

  shared_ptr<Client> c2 = session.clientAdd (Client::IPv6, "2001::100");
  ASSERT_TRUE(c2 != nullptr);

  ASSERT_EQ(session.clientCount (), 2);

  ASSERT_TRUE(session.clientRemove ("192.168.1.100"));
  ASSERT_TRUE(session.clientRemove ("2001::100"));

  EXPECT_EQ(session.clientCount (), 0);
}

TEST_F(SessionTest, CreatedAndAddClientsIgnoreRemoveNotExistsClient)
{
  shared_ptr<Client> c = session.clientAdd (Client::IPv4, "192.168.1.100");
  ASSERT_TRUE(c != nullptr);

  shared_ptr<Client> c2 = session.clientAdd (Client::IPv6, "2001::100");
  ASSERT_TRUE(c2 != nullptr);

  ASSERT_EQ(session.clientCount (), 2);

  ASSERT_FALSE(session.clientRemove ("192.168.1.101"));
  ASSERT_FALSE(session.clientRemove ("2001::101"));

  EXPECT_EQ(session.clientCount (), 2);
}
