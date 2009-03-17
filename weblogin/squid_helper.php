<?php
/*
  Copyright (c) 2009, Neutron Soutmun <neo.neutron@gmail.com>
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
include_once 'config.php';
require_once 'rahu_xmlrpc.class.php';

define(CACHE_TIME, 120);

if (! defined(STDIN)) {
  define("STDIN", fopen("php://stdin", "r"));
}

$user_list = array();

while (!feof(STDIN)) {
  $arg = trim(fgets(STDIN));
  $srcip = rawurldecode($arg);

  // Check cache
  if (!empty($user_list[$srcip]['username']) && 
    (time() - $user_list[$srcip]['timestamp']) < CACHE_TIME) {
    fwrite(STDOUT, "OK user=". $user_list[$srcip]['username']  ."\n");
    continue;
  }

  $xmlrpc = new rahu_xmlrpc_client();
  $xmlrpc->host = $config["RAHUNAS_HOST"];
  $xmlrpc->port = $config["RAHUNAS_PORT"];
  try {
    $retinfo = $xmlrpc->do_getsessioninfo($srcip);
    if (is_array($retinfo) && !empty($retinfo['session_id'])) {
      $user_list[$srcip]['username'] = $retinfo['username'];      
      $user_list[$srcip]['timestamp'] = time();      
      fwrite(STDOUT, "OK user=". $retinfo['username']  ."\n");
    } else {
      fwrite(STDOUT, "ERR\n");
    }

  } catch (XML_RPC2_FaultException $e) {
      fwrite(STDOUT, "ERR\n");
  } catch (Exception $e) {
      fwrite(STDOUT, "ERR\n");
  }
  unset($xmlrpc);
}
?>
