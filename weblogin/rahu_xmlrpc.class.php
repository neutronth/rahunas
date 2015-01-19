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
    $data['VServerID'] = $vserver_id;

    $result = $client->startsession(base64_encode(json_encode ($data)));
    $decoded = json_decode (base64_decode ($result), true);
    return $decoded == NULL ? false : $decoded;
  }

  function do_stopsession($vserver_id, $data) {
    $client = $this->getClient();
    $data['VServerID'] = $vserver_id;

    $result = $client->stopsession(base64_encode(json_encode ($data)));
    $decoded = json_decode (base64_decode ($result), true);
    return $decoded == NULL ? false : $decoded;
  }

  function do_getsessioninfo($vserver_id, $ip) {
    $client = $this->getClient();
    $data   = array (
      "VServerID" => $vserver_id,
      "IP" => $ip
    );
    $result = $client->getsessioninfo(base64_encode(json_encode ($data)));

    $decoded = json_decode (base64_decode ($result), true);
    return $decoded == NULL ? false : $decoded;
  }

  function do_roaming($vserver_id, $data) {
    $client = $this->getClient();
    $data['VServerID'] = $vserver_id;
    $result = $client->roaming(base64_encode(json_encode ($data)));

    $decoded = json_decode (base64_decode ($result), true);
    return $decoded == NULL ? false : $decoded;
  }
}
?>
