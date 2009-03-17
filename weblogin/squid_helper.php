<?php
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
