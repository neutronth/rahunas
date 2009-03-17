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

require_once 'config.php';
require_once 'rahu_radius.class.php';

// Deny all connections that does not come from the localhost
if ($_SERVER['REMOTE_ADDR'] != "127.0.0.1")
  die();

function do_stopacct($method_name, $params, $app_data)
{
  $ip =& $GLOBALS["ip"];
  $username =& $GLOBALS["username"];
  $session_id =& $GLOBALS["session_id"];
  $session_start =& $GLOBALS["session_start"];
  $mac_address =& $GLOBALS["mac_address"];
  $cause =& $GLOBALS["cause"];

  // parsing[0] - ip
  // parsing[1] - username
  // parsing[2] - session_id
  // parsing[3] - session_start  
  // parsing[4] - mac_address  
  // parsing[5] - cause  
 
  //return $params[0];
  $parsing = explode("|", $params[0]);
  $ip = $parsing[0];
  $username   = $parsing[1];
  $session_id = $parsing[2];
  $session_start = intval($parsing[3]);
  $mac_address = $parsing[4];
  $cause = intval($parsing[5]);

  if (!empty($username) && !empty($session_id)) {
    $GLOBALS["task"] = "do_stopacct";
  } else {
    $GLOBALS["task"] = "";
  }

//  return "Recieve: IP=$ip, Username=$username, Session-ID=$session_id, Session-Start=$session_start, MAC=$mac_address";
  return "[RESULT]";
}

$xmlrpc_server = xmlrpc_server_create();

xmlrpc_server_register_method($xmlrpc_server, "stopacct", "do_stopacct");

$request_xml = $HTTP_RAW_POST_DATA;

$response = xmlrpc_server_call_method($xmlrpc_server, $request_xml, '');

if ($GLOBALS["task"] == "do_stopacct") {
  $ip =& $GLOBALS["ip"];
  $username =& $GLOBALS["username"];
  $session_id =& $GLOBALS["session_id"];
  $session_start =& $GLOBALS["session_start"];
  $mac_address =& $GLOBALS["mac_address"];
  $cause =& $GLOBALS["cause"];

  $racct = new rahu_radius_acct ($username);
  $racct->host = $config["RADIUS_HOST"];
  $racct->port = $config["RADIUS_ACCT_PORT"];
  $racct->secret = $config["RADIUS_SECRET"];
  $racct->nas_identifier = $config["NAS_IDENTIFIER"];
  $racct->nas_ip_address = $config["NAS_IP_ADDRESS"];
  $racct->framed_ip_address  = $ip;
  $racct->calling_station_id = $mac_address;
  $racct->terminate_cause = !empty($cause) ? $cause : RADIUS_TERM_NAS_ERROR;
  $racct->nas_port = $config["NAS_PORT"];
  $racct->session_id    = $session_id;
  $racct->session_start = $session_start;
  if ($racct->acctStop() === true) {
    $response = str_replace ("[RESULT]", "OK", $response);
  } else {
    $response = str_replace ("[RESULT]", "FAIL", $response);
  }
}

print $response;

xmlrpc_server_destroy($xmlrpc_server);
?>
