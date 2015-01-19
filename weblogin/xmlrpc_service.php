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
require_once 'rahu_authen.class.php';

// Deny all connections that does not come from the localhost
if ($_SERVER['REMOTE_ADDR'] != "127.0.0.1")
  die();

define ("JSON_REPLY_OK",   base64_encode(json_encode(array("Status" => 200))));
define ("JSON_REPLY_FAIL", base64_encode(json_encode(array("Status" => 400))));

function rahu_xml_getConfig ($ip) {
  $client = new RahuClient ($ip);
  $rahuconfig = new RahuConfig ($client); 
  return $rahuconfig->getConfig (); 
}

function do_stopacct($method_name, $params, $app_data)
{
  $request = $params[0];
  $info = json_decode(base64_decode($request), true);

  $config =& rahu_xml_getConfig ($info["IP"]);
  $vserver_id = $config["VSERVER_ID"];

  $racct = new rahu_radius_acct ($info["Username"]);
  $racct->host = $config["RADIUS_HOST"];
  $racct->port = $config["RADIUS_ACCT_PORT"];
  $racct->secret = $config["RADIUS_SECRET"];
  $racct->nas_identifier = $config["NAS_IDENTIFIER"];
  $racct->nas_ip_address = $config["NAS_IP_ADDRESS"];
  $racct->framed_ip_address  = $info["IP"];
  $racct->calling_station_id = $info["MAC"];
  $racct->terminate_cause = !empty($info["Cause"]) ? $info["Cause"] : RADIUS_TERM_NAS_ERROR;
  $racct->nas_port = $config["VSERVER_ID"];
  $racct->session_id    = $info["SessionID"];
  $racct->session_start = $info["SessionStart"];
  $racct->download_bytes = $info["DownloadBytes"];
  $racct->upload_bytes   = $info["UploadBytes"];

  if ($racct->acctStop() === true) {
    $response = JSON_REPLY_OK;
  } else {
    $response = JSON_REPLY_FAIL;
  }

  return $response;
}

function do_update($method_name, $params, $app_data)
{
  $request = $params[0];
  $info = json_decode(base64_decode($request), true);

  $config =& rahu_xml_getConfig ($info["IP"]);
  $vserver_id = $config["VSERVER_ID"];

  $racct = new rahu_radius_acct ($info["Username"]);
  $racct->host = $config["RADIUS_HOST"];
  $racct->port = $config["RADIUS_ACCT_PORT"];
  $racct->secret = $config["RADIUS_SECRET"];
  $racct->nas_identifier = $config["NAS_IDENTIFIER"];
  $racct->nas_ip_address = $config["NAS_IP_ADDRESS"];
  $racct->framed_ip_address  = $info["IP"];
  $racct->calling_station_id = $info["MAC"];
  $racct->nas_port = $config["VSERVER_ID"];
  $racct->session_id    = $info["SessionID"];
  $racct->session_start = $info["SessionStart"];
  $racct->download_bytes = $info["DownloadBytes"];
  $racct->upload_bytes   = $info["UploadBytes"];

  if ($racct->acctUpdate() === true) {
    $response = JSON_REPLY_OK;
  } else {
    $response = JSON_REPLY_FAIL;
  }

  return $response;
}

function do_offacct($method_name, $params, $app_data)
{
  $config_list =& $GLOBALS["config_list"];
  $config = array ();

  $request = $params[0];
  $info = json_decode(base64_decode($request), true);

  foreach ($config_list as $network=>$cfg) {
    if (intval ($cfg["VSERVER_ID"]) == $info["VServerID"]) {
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
      $response = JSON_REPLY_OK;
    } else {
      $response = JSON_REPLY_FAIL;
    }
  } else {
    $response = JSON_REPLY_FAIL;
  }

  return $response;
}

function do_macauthen ($method_name, $params, $app_data)
{
  $response = JSON_REPLY_FAIL;
  $request = $params[0];
  $info = json_decode(base64_decode($request), true);

  $auth = new RahuAuthenMAC ($info["IP"], $info["MAC"]);
  $auth->start ();

  if ($auth->isValid ())
    $response = JSON_REPLY_OK;

  return $response;
}


session_start ();

if (!isset ($_SESSION["xmlrpc_server"])) {
  $xmlrpc_server = xmlrpc_server_create();

  xmlrpc_server_register_method($xmlrpc_server, "stopacct", "do_stopacct");
  xmlrpc_server_register_method($xmlrpc_server, "offacct", "do_offacct");
  xmlrpc_server_register_method($xmlrpc_server, "update", "do_update");
  xmlrpc_server_register_method($xmlrpc_server, "macauthen", "do_macauthen");

  $_SESSION["xmlrpc_server"] = $xmlrpc_server;
}

$xmlrpc_server = $_SESSION["xmlrpc_server"];

$request_xml = $HTTP_RAW_POST_DATA;

$response = xmlrpc_server_call_method($xmlrpc_server, $request_xml, '');
print $response;
?>
