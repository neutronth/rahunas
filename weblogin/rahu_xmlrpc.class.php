<?php
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

  function do_startsession($data) {
    $client = $this->getClient(); 
    $params = implode("|", $data);
    $params .= "|";
    $result = $client->startsession($params);
    return $result;
  }

  function do_stopsession($ip, $mac) {
    $client = $this->getClient(); 
    $params = sprintf("%s|%s", $ip, $mac);
    $result = $client->stopsession($params);
    if (strstr($result, "was removed"))
      return true;
    else
      return false;
  }

  function do_getsessioninfo($ip) {
    $client = $this->getClient(); 
    $params = sprintf("%s", $ip);
    $result = $client->getsessioninfo($ip);
    $ret = explode("|", $result);

    /* $ret[0] - ip
       $ret[1] - username
       $ret[2] - session_id
       $ret[3] - session_start
       $ret[4] - mac_address
       $ret[5] - session_timeout */
    $ip = $ret[0];
    $username = $ret[1];
    $session_id = $ret[2];
    $session_start = $ret[3];
    $mac_address = $ret[4];
    $session_timeout = $ret[5];

    $result = array("ip"=>$ip, 
                    "username"=>$username, 
                    "session_id"=>$session_id,
                    "session_start"=>$session_start,
                    "mac_address"=>$mac_address,
                    "session_timeout"=>$session_timeout);
    return $result;
  }
}

?>
