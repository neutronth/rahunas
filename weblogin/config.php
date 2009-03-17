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

$config = array(
  "RADIUS_HOST" => "localhost",
  "RADIUS_SECRET" => "testing123",
  "RADIUS_ENCRYPT" => "CHAP_MD5",
  "RADIUS_AUTH_PORT" => 0,
  "RADIUS_ACCT_PORT" => 0,
  "RAHUNAS_HOST" => "localhost",
  "RAHUNAS_PORT" => "8123",
  "NAS_IDENTIFIER" => "RahuNAS-01",
  "NAS_IP_ADDRESS" => "172.30.0.1",
  "NAS_LOGIN_HOST" => "172.30.0.1",
  "NAS_LOGIN_PORT" => "8443",
  "NAS_LOGIN_PROTO" => "https",
  "NAS_PORT" => 1,
  "NAS_LOGIN_TITLE" => "RahuNAS Network",
  "DEFAULT_REDIRECT_URL" => "http://www.kku.ac.th",
  "DEFAULT_LANGUAGE" => "Thai",
  "UAM_TEMPLATE" => "rahunas"
);
?>
