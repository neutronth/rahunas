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
require_once 'networkchk.php';

// Deny all connections that does not come from the localhost
if ($_SERVER['REMOTE_ADDR'] != "127.0.0.1")
  die();

function do_stopacct($method_name, $params, $app_data)
{
  $config_list =& $GLOBALS["config_list"];
  $response = "FAIL";

  // parsing[0] - ip
  // parsing[1] - username
  // parsing[2] - session_id
  // parsing[3] - session_start  
  // parsing[4] - mac_address  
  // parsing[5] - cause  
  // parsing[6] - download_bytes
  // parsing[7] - upload_bytes
 
  $parsing = explode("|", $params[0]);
  $ip = $parsing[0];
  $username   = $parsing[1];
  $session_id = $parsing[2];
  $session_start = intval($parsing[3]);
  $mac_address = $parsing[4];
  $cause = intval($parsing[5]);
  $download_bytes = intval($parsing[6]);
  $upload_bytes   = intval($parsing[7]);

  $config = get_config_by_network($ip, $config_list);
  $vserver_id = $config["VSERVER_ID"];

  $racct = new rahu_radius_acct ($username);
  $racct->host = $config["RADIUS_HOST"];
  $racct->port = $config["RADIUS_ACCT_PORT"];
  $racct->secret = $config["RADIUS_SECRET"];
  $racct->nas_identifier = $config["NAS_IDENTIFIER"];
  $racct->nas_ip_address = $config["NAS_IP_ADDRESS"];
  $racct->framed_ip_address  = $ip;
  $racct->calling_station_id = $mac_address;
  $racct->terminate_cause = !empty($cause) ? $cause : RADIUS_TERM_NAS_ERROR;
  $racct->nas_port = $config["VSERVER_ID"];
  $racct->session_id    = $session_id;
  $racct->session_start = $session_start;
  $racct->download_bytes = $download_bytes;
  $racct->upload_bytes   = $upload_bytes;
  if ($racct->acctStop() === true) {
    $response = "OK";
  } else {
    $response = "FAIL";
  }

  return $response;
}

function do_update($method_name, $params, $app_data)
{
  $config_list =& $GLOBALS["config_list"];
  $response = "FAIL";

  // parsing[0] - ip
  // parsing[1] - username
  // parsing[2] - session_id
  // parsing[3] - session_start
  // parsing[4] - mac_address
  // parsing[5] - download_bytes
  // parsing[6] - upload_bytes

  $parsing = explode("|", $params[0]);
  $ip = $parsing[0];
  $username   = $parsing[1];
  $session_id = $parsing[2];
  $session_start = intval($parsing[3]);
  $mac_address = $parsing[4];
  $download_bytes = intval($parsing[5]);
  $upload_bytes   = intval($parsing[6]);

  $config = get_config_by_network($ip, $config_list);
  $vserver_id = $config["VSERVER_ID"];

  $racct = new rahu_radius_acct ($username);
  $racct->host = $config["RADIUS_HOST"];
  $racct->port = $config["RADIUS_ACCT_PORT"];
  $racct->secret = $config["RADIUS_SECRET"];
  $racct->nas_identifier = $config["NAS_IDENTIFIER"];
  $racct->nas_ip_address = $config["NAS_IP_ADDRESS"];
  $racct->framed_ip_address  = $ip;
  $racct->calling_station_id = $mac_address;
  $racct->nas_port = $config["VSERVER_ID"];
  $racct->session_id    = $session_id;
  $racct->session_start = $session_start;
  $racct->download_bytes = $download_bytes;
  $racct->upload_bytes   = $upload_bytes;
  if ($racct->acctUpdate() === true) {
    $response = "OK";
  } else {
    $response = "FAIL";
  }

  return $response;
}

function do_offacct($method_name, $params, $app_data)
{
  $config_list =& $GLOBALS["config_list"];
  $response = "FAIL";

  // parsing[0] - vserver_id

  $parsing = explode("|", $params[0]);
  $vserver_id = $parsing[0];

  $config = array ();

  foreach ($config_list as $network=>$cfg) {
    if ($cfg["VSERVER_ID"] = $vserver_id) {
      $config = $cfg;
      break;
    }
  }

  if (!empty ($config)) {
    $racct = new rahu_radius_acct ();

    $racct->host = $config["RADIUS_HOST"];
    $racct->port = $config["RADIUS_ACCT_PORT"];
    $racct->secret = $config["RADIUS_SECRET"];
    $racct->nas_ip_address = $config["NAS_IP_ADDRESS"];
    $racct->nas_port = $config["VSERVER_ID"];

    if ($racct->acctOff() === true) {
      $response = "OK";
    } else {
      $response = "FAIL";
    }
  } else {
    $response = "FAIL";
  }

  return $response;
}

$xmlrpc_server = xmlrpc_server_create();

xmlrpc_server_register_method($xmlrpc_server, "stopacct", "do_stopacct");
xmlrpc_server_register_method($xmlrpc_server, "offacct", "do_offacct");
xmlrpc_server_register_method($xmlrpc_server, "update", "do_update");

$request_xml = $HTTP_RAW_POST_DATA;

$response = xmlrpc_server_call_method($xmlrpc_server, $request_xml, '');
print $response;

xmlrpc_server_destroy($xmlrpc_server);
?>
