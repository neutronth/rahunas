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
#include <string>

#include "Network.h"

using std::memset;

Network::Network (enum Client::support_family f, string net, string netname)
  : valid (false), family (f), cidr (0), netname (netname)
{
  size_t cidr_found = net.find_last_of ("/");

  memset (&netaddr, 0, sizeof (IPStorage));

  if (cidr_found != string::npos)
    {
      int cidr_conv;
      stringstream ss (net.substr (cidr_found + 1));
      ss >> cidr_conv;
      cidr = cidr_conv;

      uint8_t cidr_max = family == Client::IPv4 ?
                           CIDR_IPv4_MAX : CIDR_IPv6_MAX;

      if (cidr > 0 && cidr <= cidr_max)
        {
          if (family == Client::IPv4)
            {
              if (inet_pton (family, net.substr (0, cidr_found).c_str (),
                    &netaddr.in) > 0)
                {
                  netaddr.ip &= ip_netmask (cidr);
                  valid = true;
                }
            }
          else
            {
              if (inet_pton (family, net.substr (0, cidr_found).c_str (),
                    &netaddr.in6) > 0)
                {
                  ip_netmask6 (&netaddr, cidr);
                  valid = true;
                }
            }
        }
    }
}

string
Network::getNetworkAddr ()
{
   const char *ret = nullptr;
   char ipstr[INET6_ADDRSTRLEN];

   if (family == Client::IPv4)
     {
       ret = inet_ntop (family, &netaddr.in, ipstr, INET6_ADDRSTRLEN);
     }
   else
     {
       ret = inet_ntop (family, &netaddr.in6, ipstr, INET6_ADDRSTRLEN);
     }

   return ret ? string (ipstr) :
            family == Client::IPv4 ? string("0.0.0.0") : string ("::");
}

bool
Network::isInNetwork (string ip)
{
  IPStorage ipdata;

  if (family == Client::IPv4)
    {
      if (inet_pton (family, ip.c_str (), &ipdata.in) > 0)
        {
          ipdata.ip &= ip_netmask (cidr);
          return netaddr.ip == ipdata.ip;
        }
    }
  else
    {
      if (inet_pton (family, ip.c_str (), &ipdata.in6) > 0)
        ip_netmask6 (&ipdata, cidr);

        return netaddr.ip6[0] == ipdata.ip6[0] &&
               netaddr.ip6[1] == ipdata.ip6[1] &&
               netaddr.ip6[2] == ipdata.ip6[2] &&
               netaddr.ip6[3] == ipdata.ip6[3];
    }

  return false;
}
