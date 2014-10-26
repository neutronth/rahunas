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

#include "../src/SessionsPool.h"

class SessionsPoolTest : public ::testing::Test {
public:
  SessionsPool sesspool;
};

TEST_F(SessionsPoolTest, CreatedWithEmptyPool)
{
  EXPECT_EQ(sesspool.size (), 0);
}

TEST_F(SessionsPoolTest, GetNewSession)
{
  ASSERT_EQ(sesspool.size (), 0);
  shared_ptr<Session> s = sesspool.sessionGetByIP (Client::IPv4,
                                                   "192.168.1.100");
  ASSERT_TRUE(s != nullptr);

  EXPECT_EQ(sesspool.size (), 1);
}

TEST_F(SessionsPoolTest, GetExistingSessionByIP)
{
  ASSERT_EQ(sesspool.size (), 0);
  shared_ptr<Session> s = sesspool.sessionGetByIP (Client::IPv4,
                                                   "192.168.1.100");
  ASSERT_TRUE(s != nullptr);
  ASSERT_EQ(sesspool.size (), 1);
  shared_ptr<Session> s2 = sesspool.sessionGetByIP (Client::IPv4,
                                                    "192.168.1.100");
  ASSERT_TRUE(s != nullptr);
  ASSERT_EQ(sesspool.size (), 1);

  EXPECT_EQ(s, s2);
}

TEST_F(SessionsPoolTest, GetExistingSessionById)
{
  ASSERT_EQ(sesspool.size (), 0);
  shared_ptr<Session> s = sesspool.sessionGetByIP (Client::IPv4,
                                                   "192.168.1.100");
  ASSERT_TRUE(s != nullptr);
  ASSERT_EQ(sesspool.size (), 1);

  shared_ptr<Session> s2 = sesspool.sessionGetById (s->getId ());
  ASSERT_TRUE(s != nullptr);
  ASSERT_EQ(sesspool.size (), 1);

  EXPECT_EQ(s, s2);
}

TEST_F(SessionsPoolTest, RemoveSession)
{
  ASSERT_EQ(sesspool.size (), 0);
  shared_ptr<Session> s = sesspool.sessionGetByIP (Client::IPv4,
                                                   "192.168.1.100");
  ASSERT_TRUE(s != nullptr);
  ASSERT_EQ(sesspool.size (), 1);

  sesspool.sessionRemove (s->getId ());

  EXPECT_EQ(sesspool.size (), 0);
  EXPECT_EQ(sesspool.ipCount (), 0);
}
