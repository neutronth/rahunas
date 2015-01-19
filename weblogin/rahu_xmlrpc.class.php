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

  This code is made possible thx to samples made by 
    the XML_RPC2 package maintainers, 
    Sérgio Carvalho (lead) <sergiosgc@gmail.com>
*/
require_once 'rahu_radius.class.php';
require_once 'XML/RPC2/Client.php';
require_once 'XML/RPC2/Value.php';

class rahu_xmlrpc_client {
  var $host;
  var $port;
  var $options;

  function rahu_xmlrpc_client() {
    $this->options = array('prefix' => '',
                           'debug' => false,
                           'encoding' => 'utf-8');
  }
  
  function getClient() {
    return XML_RPC2_Client::create("http://$this->host:$this->port", 
                                   $this->options); 

  }

  function do_startsession($vserver_id, $data) {
    $client = $this->getClient(); 
    $params = implode("|", $data);
    $params = sprintf("%s|%s", $vserver_id, $params);
    $result = $client->startsession($params);
    return $result;
  }

  function do_stopsession($vserver_id, $ip, $mac, 
                          $cause = RADIUS_TERM_USER_REQUEST) {
    $client = $this->getClient(); 
    $params = sprintf("%s|%s|%s|%s", $vserver_id, $ip, $mac, $cause);
    $result = $client->stopsession($params);
    if (strstr($result, "was removed"))
      return true;
    else
      return false;
  }

  function do_getsessioninfo($vserver_id, $ip) {
    $client = $this->getClient(); 
    $params = sprintf("%s|%s", $vserver_id, $ip);
    $result = $client->getsessioninfo($params);
    $ret = explode("|", $result);

    /* $ret[0] - ip
       $ret[1] - username
       $ret[2] - session_id
       $ret[3] - session_start
       $ret[4] - mac_address
       $ret[5] - session_timeout
       $ret[6] - serviceclass_description
       $ret[7] - download_bytes
       $ret[8] - upload_bytes
       $ret[9] - download_speed
       $ret[10] - upload_speed
    */

    if (count ($ret) == 11) {
      $ip = $ret[0];
      $username = $ret[1];
      $session_id = $ret[2];
      $session_start = $ret[3];
      $mac_address = $ret[4];
      $session_timeout = $ret[5];
      $serviceclass_description = $ret[6];
      $download_bytes = $ret[7];
      $upload_bytes = $ret[8];
      $download_speed   = $ret[9];
      $upload_speed     = $ret[10];

      $result = array("ip"=>$ip,
                      "username"=>$username,
                      "session_id"=>$session_id,
                      "session_start"=>$session_start,
                      "mac_address"=>$mac_address,
                      "session_timeout"=>$session_timeout,
                      "serviceclass_description"=>$serviceclass_description,
                      "download_bytes"=>$download_bytes,
                      "upload_bytes"=>$upload_bytes,
                      "download_speed"=>$download_speed,
                      "upload_speed"=>$upload_speed);
    }

    return $result;
  }

  function do_roaming($vserver_id, $session_id, $ip, $secure_token, $new_ip, $mac) {
    $client = $this->getClient();
    $params = sprintf("%s|%s|%s|%s|%s|%s", $vserver_id, $session_id, $ip,
                      $secure_token, $new_ip, $mac);
    $result = $client->roaming($params);
    $ret = explode("|", $result);

    /* $ret[0] - ip
       $ret[1] - username
       $ret[2] - session_id
       $ret[3] - session_start
       $ret[4] - mac_address
       $ret[5] - session_timeout
       $ret[6] - serviceclass_name
       $ret[7] - bandwidth_max_down
       $ret[8] - bandwidth_max_up
    */

    if (count ($ret) == 9) {
      $ip = $ret[0];
      $username = $ret[1];
      $session_id = $ret[2];
      $session_start = $ret[3];
      $mac_address = $ret[4];
      $session_timeout = $ret[5];
      $serviceclass_name = $ret[6];
      $bandwidth_max_down = $ret[7];
      $bandwidth_max_up = $ret[8];

      $result = array("ip"=>$ip,
                      "username"=>$username,
                      "session_id"=>$session_id,
                      "session_start"=>$session_start,
                      "mac_address"=>$mac_address,
                      "session_timeout"=>$session_timeout,
                      "serviceclass_name"=>$serviceclass_name,
                      "bandwidth_max_down"=>$bandwidth_max_down,
                      "bandwidth_max_up"=>$bandwidth_max_up);
    } else {
      $result = false;
    }

    return $result;
  }
}
?>
