<?php
/*
  Copyright (c) 2008-2009, Neutron Soutmun <neo.neutron@gmail.com>
  All rights reserved.

  Redistribution and use in source and binary forms, with or without 
  modification, are permitted provided that the following conditions 
  are met:

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.
  3. The names of the authors may not be used to endorse or promote products 
     derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
  POSSIBILITY OF SUCH DAMAGE.

  This code cannot simply be copied and put under the GNU Public License or 
    any other GPL-like (LGPL, GPL2) License.
*/

function get_message($message) {
  $MSG['ERR_CONNECT_SERVER'] = _("Error! Could not connect to the server");
  $MSG['ERR_CONNECT_RADIUS'] = _("Error! Could not connect to the authenticator server");
  $MSG['ERR_MAXIMUM_LOGIN']  = _("Error! User has been reach the maximum login");
  $MSG['ERR_ALREADY_LOGIN']  = _("Error! User already login");
  $MSG['ERR_USER_EXPIRED']   = _("Error! User expired");
  $MSG['ERR_INVALID_IP']     = _("Error! Invalid IP address");
  $MSG['ERR_INVALID_USERNAME_OR_PASSWORD'] = 
    _("Error! Invalid username or password");
  $MSG['ERR_LOGOUT_FAILED']  = _("Error! Logout failed! please try again");
  $MSG['ERR_PLEASE_LOGIN']   = _("Error! Please login");
  
  $MSG['OK_USER_AUTHORIZED'] = 
    _("Success! User has been authorized, please wait a moment");
  $MSG['OK_USER_LOGOUT'] = _("Success! User has been logout");
  $MSG['ROAMING'] = _("Roaming in progress, User will be moved to current network");

  return $MSG[$message];
}
?>
