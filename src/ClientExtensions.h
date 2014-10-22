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

#ifndef _RH_CLIENT_EXTENSIONS_H
#define _RH_CLIENT_EXTENSIONS_H

#include <string>

class ClientExtensions {
public:
  ClientExtensions ();

  string& getMAC () { return mac; }
  bool    setMAC (string set_mac);

  unsigned int getMaxSpeedDownload () { return max_speed_download; }
  unsigned int getMaxSpeedUpload ()   { return max_speed_upload;   }
  unsigned int getMinSpeedDownload () { return min_speed_download; }
  unsigned int getMinSpeedUpload ()   { return min_speed_upload;   }

  void setMaxSpeedDownload (unsigned int s) { max_speed_download = s; }
  void setMaxSpeedUpload (unsigned int s)   { max_speed_upload = s;   }
  void setMinSpeedDownload (unsigned int s) { min_speed_download = s; }
  void setMinSpeedUpload (unsigned int s)   { min_speed_upload = s;   }

private:
  string mac;
  unsigned int max_speed_download;
  unsigned int max_speed_upload;
  unsigned int min_speed_download;
  unsigned int min_speed_upload;
};

inline
ClientExtensions::ClientExtensions () :
  mac ("00:00:00:00:00:00"),
  max_speed_download (0), max_speed_upload (0),
  min_speed_download (0), min_speed_upload (0)
{
}

inline bool
ClientExtensions::setMAC (string set_mac)
{
  mac.assign (set_mac);
  return set_mac == mac;
}

#endif // _RH_CLIENT_EXTENSIONS
