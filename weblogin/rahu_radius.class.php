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
    Michael Bretterklieber <michael@bretterklieber.com> author of 
    the PHP PECL Radius package
*/

require_once 'Auth/RADIUS.php';
require_once 'Crypt/CHAP.php';
require_once 'rahu_dictionary.php';

define ("RADIUS_ACCT_INPUT_GIGAWORDS", 52);
define ("RADIUS_ACCT_OUTPUT_GIGAWORDS", 53);
define ("RADIUS_GIGAWORD", 4294967296); /* 2^32 */

class rahu_radius_auth {
  var $type;
  var $username;
  var $password;
  var $host;
  var $port;
  var $secret;
  var $result;
  var $error;
  var $attributes;
  var $rawAttributes;
  var $rawVendorAttributes;
  var $LoggedIn;
  var $Timeout;

  function rahu_radius_auth($username, $password, $type = 'CHAP_MD5') {
    $this->type = $type;
    $this->username = $username;
    $this->password = $password;
    $this->error = 0;
    $this->LoggedIn = false;
  }

  function getRawAttribute ($attr) {
    return array_key_exists ($attr, $this->rawAttributes) ?
             $this->rawAttributes[$attr] : "";
  }

  function getAttribute ($attr) {
    return array_key_exists ($attr, $this->attributes) ?
             $this->attributes[$attr] : "";
  }

  function start() {
    global $vendors;

    $type =& $this->type;
    $username =& $this->username;
    $password =& $this->password;

    if ($type == "Call-Check") {
      $classname = 'Auth_RADIUS_PAP';
    } else {
      $classname = 'Auth_RADIUS_' . $type;
    }

    $rauth = new $classname ($username, $password);
    $rauth->addServer($this->host, $this->port, $this->secret);
    $rauth->username = $username;
    
    // Disable the standard attributes
    $rauth->useStandardAttributes = 0;

    switch ($type) {
      case 'CHAP_MD5':
      case 'MSCHAPv1':
        $classname = $type == 'MSCHAPv1' ? 'Crypt_CHAP_MSv1' : 'Crypt_CHAP_MD5';
        $crpt = new $classname;
        $crpt->password = $password;
        $rauth->challenge = $crpt->challenge;
        $rauth->chapid = $crpt->chapid;
        $rauth->response = $crpt->challengeResponse ();
        $rauth->flags = 1;
        break;
    
      case 'MSCHAPv2':
        $crpt = new Crypt_CHAP_MSv2;
        $crpt->username = $username;
        $crpt->password = $password;
        $rauth->challenge = $crpt->authChallenge;
        $rauth->peerChallenge = $crpt->peerChallenge;
        $rauth->chapid = $crpt->chapid;
        $rauth->response = $crpt->challengeResponse;
        break;
    
      default:
        $rauth->password = $password;
        break;
    }
    
    
    if (!$rauth->start ()) {
      $this->error = 1;
      return -1;
    }

    if ($type == "Call-Check") {
      $rauth->putAttribute (RADIUS_CALLING_STATION_ID, $this->username);
      $rauth->putAttribute (RADIUS_SERVICE_TYPE, 10);
    }
    
    $this->result = $rauth->send ();
    if (PEAR::isError ($this->result)) {
      $this->error = 1;
      return -1;
    }    

    // get attributes, even if auth failed
    if ($rauth->getAttributes ()) {
      $this->attributes = $rauth->attributes;
      $this->rawAttributes = $rauth->rawAttributes;
      $this->rawVendorAttributes = $rauth->rawVendorAttributes;

      // Extract the vendor attributes
      foreach ($this->rawVendorAttributes as $ven_id=>$data) {
        foreach ($data as $attr_id => $val) {
          $get_helper = "radius_cvt_" . 
                        $vendors[$ven_id][$attr_id]["AttributeType"];
          if (function_exists ($get_helper)) {
            $this->attributes[$vendors[$ven_id][$attr_id]["AttributeName"]] =
              $get_helper($val);
          }
        }
      }
      
      if (!empty($this->attributes['reply_message'])) {
        if (strstr($this->attributes['reply_message'], "logged in"))
          $this->LoggedIn = true;
        else if (strstr($this->attributes['reply_message'], "Your maximum") ||
                 strstr($this->attributes['reply_message'], "Password Has Expired"))
          $this->Timeout = true;
      }
    }
    
    $rauth->close ();
  }

  function isError() {
    return $this->error == 1;
  }

  function isAccept() {
    if ($this->error)
      return false;

    return $this->result === true;
  }

  function isLoggedIn() {
    return $this->LoggedIn;
  }
  
  function isTimeout() {
    return $this->Timeout;
  }
}

class rahu_radius_acct {
  var $username;
  var $host;
  var $port;
  var $secret;
  var $framed_ip_address;
  var $calling_station_id;
  var $called_station_id;
  var $terminate_cause;
  var $nas_identifier;
  var $nas_ip_address;
  var $nas_port;
  var $session_id;
  var $session_start;
  var $result;
  var $error;

  function rahu_radius_acct ($username = "") {
    $this->username = $username;
    $this->error = 0;
  }

  function gen_session_id() {
    $randno1 = rand(0,65535);
    $randno2 = rand(0,65535);
    $randno3 = rand(0,65535);
    $randno4 = rand(0,65535);
    $randno = sprintf("%s%s%s%s",
                str_pad(dechex($randno1), 4, "0", STR_PAD_LEFT),
                str_pad(dechex($randno2), 4, "0", STR_PAD_LEFT),
                str_pad(dechex($randno3), 4, "0", STR_PAD_LEFT),
                str_pad(dechex($randno4), 4, "0", STR_PAD_LEFT));
    $this->session_id = $randno;

    return $this->session_id;
  }

  function get_session_time() {
    return time() - $this->session_start;
  }

  function acct($accttype, $param=NULL) {
    if ($accttype == "Off") {
      $classname = "Auth_RADIUS_Acct";
      $this->session_id = "";
    } else {
      $classname = "Auth_RADIUS_Acct_" .$accttype;
    }

    $racct = new $classname;
    $racct->addServer($this->host, $this->port, $this->secret);

    if (!empty ($this->username)) {
      $racct->username = $this->username;
    }

    $racct->authentic = RADIUS_AUTH_LOCAL;

    if (empty($this->session_id)) {
      switch($accttype) {
        case "Start":
          $this->session_id = $this->gen_session_id();
          break;
        case "Stop":
          /* Fall-through */
        case "Update":
          /* Do not permit the requests that have no session ID */
          return -1;
          break;
        case "Off":
          $racct->status_type = RADIUS_ACCOUNTING_OFF;
          break;
      }
    }

    $racct->useStandardAttributes = 0;

    if (!empty ($this->session_id)) {
      $racct->session_id = $this->session_id;
      $racct->session_time = $this->get_session_time();
    }

    $status = $racct->start();
    if(PEAR::isError($status)) {
      $this->error = 1;
      return -1;
    }

    if (!empty ($this->session_id)) {
      $racct->putAttribute(RADIUS_NAS_PORT_TYPE, RADIUS_ETHERNET);
      $racct->putAttribute(RADIUS_USER_NAME, $this->username);
      $racct->putAttribute(RADIUS_FRAMED_IP_ADDRESS,
                               ip2long($this->framed_ip_address));
      $racct->putAttribute(RADIUS_CALLING_STATION_ID, $this->calling_station_id);
      $racct->putAttribute(RADIUS_CALLED_STATION_ID, $this->called_station_id);
    }

    $racct->putAttribute(RADIUS_NAS_IDENTIFIER, $this->nas_identifier);
    $racct->putAttribute(RADIUS_NAS_PORT, intval($this->nas_port));

    switch($accttype) {
      case "Start":
        break;
      case "Stop":
        $racct->putAttribute(RADIUS_ACCT_TERMINATE_CAUSE, 
                             $this->terminate_cause);
        /* Fall-through */
      case "Update":
        $racct->putAttribute(RADIUS_ACCT_SESSION_TIME, $racct->session_time);
        $racct->putAttribute(RADIUS_ACCT_INPUT_GIGAWORDS,
                             intval($this->download_bytes / RADIUS_GIGAWORD));
        $racct->putAttribute(RADIUS_ACCT_OUTPUT_GIGAWORDS,
                             intval($this->upload_bytes / RADIUS_GIGAWORD));
        $racct->putAttribute(RADIUS_ACCT_INPUT_OCTETS,
                             $this->download_bytes % RADIUS_GIGAWORD);
        $racct->putAttribute(RADIUS_ACCT_OUTPUT_OCTETS,
                             $this->upload_bytes % RADIUS_GIGAWORD);
        break;
      case "Off":
        $racct->putAttribute(RADIUS_ACCT_TERMINATE_CAUSE,
                             RADIUS_TERM_NAS_REQUEST);
        break;
    }
    
    $this->result = $racct->send();
    if(PEAR::isError($this->result)) {
      $this->error = 1;
      return -1;
    }
    
    $racct->close();
    return $this->result;
  }

  function acctStart() {
    return $this->acct("Start"); 
  }

  function acctUpdate() {
    return $this->acct("Update");
  }

  function acctStop() {
    return $this->acct("Stop");
  }

  function acctOff() {
    return $this->acct("Off");
  }
}

?>
