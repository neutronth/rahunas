<?php
/*
  Copyright (c) 2011-2014, Neutron Soutmun <neutron@rahunas.org>
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
require_once 'Net/IPv4.php';
require_once 'rahu_radius.class.php';
require_once 'rahu_xmlrpc.class.php';
require_once 'rahu_i18n.class.php';
require_once 'rahu_render.class.php';

/* Global declaration */
require_once 'config.php';
require_once 'messages.php';

class RahuClient {
  private $ip;
  private $mac;

  public function __construct ($ip = "") {
    $this->ip  = !empty ($ip) ? $ip : $_SERVER['REMOTE_ADDR'];
    $this->mac = $this->returnMacAddress ();
  }

  public function getIP () {
    return $this->ip;
  }

  public function getMAC () {
    return $this->mac;
  }

  private function returnMacAddress() {
    // This code is under the GNU Public Licence
    // Written by michael_stankiewicz {don't spam} at yahoo {no spam} dot com
    // Tested only on linux, please report bugs

    // WARNING: the commands 'which' and 'arp' should be executable
    // by the apache user; on most linux boxes the default configuration
    // should work fine

    // Get the arp executable path
    $location = "/usr/sbin/arp";

    // Get the remote ip address (the ip address of the client, the browser)
    $remoteIp = $_SERVER['REMOTE_ADDR'];
    $remoteIp = str_replace(".", "\\.", $remoteIp);

    // Execute the arp command and store the output in $arpTable
    $arpTable = shell_exec("$location -n");

    // Split the output so every line is an entry of the $arpSplitted array
    $arpSplitted = split("\n",$arpTable);


    // Cicle the array to find the match with the remote ip address
    foreach ($arpSplitted as $value) {
      // Split every arp line, this is done in case the format of the arp
      // command output is a bit different than expected
      $valueSplitted = split(" ",$value);

      foreach ($valueSplitted as $spLine) {
        $ipFound = false;

        if ( preg_match("/\b$remoteIp\b/",$spLine) ) {
          $ipFound = true;
        }

        // The ip address has been found, now rescan all the string
        // to get the mac address

        if ($ipFound) {
          // Rescan all the string, in case the mac address, in the string
          // returned by arp, comes before the ip address
          // (you know, Murphy's laws)
          reset($valueSplitted);

          foreach ($valueSplitted as $spLine) {
            if (preg_match("/[0-9a-f][0-9a-f][:-][0-9a-f][0-9a-f][:-]".
                           "[0-9a-f][0-9a-f][:-][0-9a-f][0-9a-f][:-]".
                           "[0-9a-f][0-9a-f][:-][0-9a-f][0-9a-f]/i",$spLine)) {
              return $spLine;
            }
          }
        }
      }
    }

    return false;
  }
}

class RahuConfig {
  private $client;
  private $config = array ();

  public function __construct ($rahuclient) {
    $this->client =& $rahuclient;
  }

  public function getConfig () {
    global $config_list;
    if (!is_array($config_list))
      return array ();

    foreach ($config_list as $network=>$config) {
      if (Net_IPv4::ipInNetwork($this->client->getIP (), $network)) {
        $this->config =& $config;
        break;
      }
    }

    return $this->config;
  }
}

abstract class RahuAuthen {
  protected $client;
  protected $xmlrpc;
  protected $i18n;
  protected $state;
  protected $config        = array ();
  protected $authenticated = false;
  protected $failed        = false;
  protected $message       = "";
  protected $message_delay = 1;
  protected $javascript    = "";
  protected $redirecting   = false;
  protected $redirect_url  = "";
  protected $sessioninfo   = array ();
  protected $userinfo      = array ();

  public function __construct () {
    global $rahu_langsupport;

    $this->client = new RahuClient ();

    $this->i18n = new RahuI18N ();
    $this->i18n->localeSetup ();

    $config = new RahuConfig ($this->client);
    $this->config =& $config->getConfig ();
    if (!empty ($this->config)) {
      $this->verifySession ();
    }
  }

  public function start () {
    if (!empty ($_POST)) {
      $this->onSubmit ();
    }

    if ($this->authenticated) {
      $this->onAuthenticated ();
    } else {
      $this->onUnAuthenticated ();
    }
  }

  protected function onAuthenticated () {
    /* Pure Abstract */
  }

  protected function onUnAuthenticated () {
    /* Pure Abstract */
  }

  protected function onSubmit () {
    /* Pure Abstract */
  }

  protected function render () {
    $tpl = new RahuRender ($this->config['UAM_TEMPLATE']);
    $tpl->setState ($this->state);
    $tpl->setRedirect (false);
    $tpl->setBrandTitle ($this->config["NAS_LOGIN_TITLE"]);
    $tpl->setLanguages ($this->i18n->getLanguages ());
    $tpl->setCurrentLanguage ($this->i18n->getCurrentLanguage ());
    $tpl->setUserRequestUrl (@urldecode ($_GET['request_url']));
    $tpl->setUserInfo ($this->userinfo);

    if (!empty ($this->message)) {
      $tpl->setMessage ($this->message, $this->failed ? "danger" : "success");
      $tpl->setState ('message');
      $this->redirecting = true;

      if (empty ($this->redirect_url)) {
        $this->redirect_url = $_SERVER["REQUEST_URI"];
      }
    }

    if ($this->redirecting) {
      $tpl->setRedirect (true, $this->redirect_url, $this->message_delay);
    }

    $tpl->render ();
  }

  private function verifySession () {
    $this->xmlrpc = new rahu_xmlrpc_client ();
    $this->xmlrpc->host = $this->config["RAHUNAS_HOST"];
    $this->xmlrpc->port = $this->config["RAHUNAS_PORT"];

    try {
      $retinfo = $this->xmlrpc->do_getsessioninfo ($this->config["VSERVER_ID"],
                                                   $this->client->getIP ());
      if (is_array ($retinfo) && !empty ($retinfo["session_id"])) {
        $this->sessioninfo = $retinfo;
        $this->authenticated = true;
      }
    } catch (XML_RPC2_FaultException $e) {
      $this->message = get_message('ERR_CONNECT_SERVER');
      $this->failed  = true;
      $this->message_delay = 10;
    } catch (XML_RPC2_CurlExeption $e) {
      $this->message = get_message('ERR_CONNECT_SERVER');
      $this->failed  = true;
      $this->message_delay = 10;
    } catch (Exception $e) {
      $this->message = get_message('ERR_CONNECT_SERVER');
      $this->failed  = true;
      $this->message_delay = 10;
    }
  }
}

class RahuAuthenLogin extends RahuAuthen {
  public function start () {
    $this->state = "login";
    parent::start ();
    $this->render ();
  }

  protected function onAuthenticated () {
    $location_tpl = "%s://%s%s/logout.php?request_url=%s";
    $this->redirect_url =
      sprintf ($location_tpl,
               $this->config['NAS_LOGIN_PROTO'],
               $this->config['NAS_LOGIN_HOST'],
               !empty ($this->config['NAS_LOGIN_PORT']) ?
                 ":" . $this->config['NAS_LOGIN_PORT'] : "",
               empty ($_GET['request_url']) ?
                 urlencode ($this->config['DEFAULT_REDIRECT_URL']) :
                 $_GET['request_url']);

    if (empty ($this->message)) {
      header ("Location: " . $this->redirect_url);
    }
  }

  protected function onUnAuthenticated () {
  }

  protected function onSubmit () {
    if (!empty($_POST['user']) && !empty($_POST['passwd'])) {
      $_POST['user'] = trim($_POST['user']);

      $rauth = new rahu_radius_auth ($_POST['user'], $_POST['passwd'],
                                     $this->config['RADIUS_ENCRYPT']);
      $rauth->host = $this->config["RADIUS_HOST"];
      $rauth->port = $this->config["RADIUS_AUTH_PORT"];
      $rauth->secret = $this->config["RADIUS_SECRET"];
      $rauth->start();

      if ($rauth->isError()) {
        $this->message = get_message('ERR_CONNECT_RADIUS');
        $this->failed  = true;
        $this->message_delay = 5;
      } else if ($rauth->isAccept()) {
        $this->message = get_message('OK_USER_AUTHORIZED');
        $racct = new rahu_radius_acct ($_POST['user']);
        $racct->host = $this->config["RADIUS_HOST"];
        $racct->port = $this->config["RADIUS_ACCT_PORT"];
        $racct->secret = $this->config["RADIUS_SECRET"];
        $racct->nas_identifier = $this->config["NAS_IDENTIFIER"];
        $racct->nas_ip_address = $this->config["NAS_IP_ADDRESS"];
        $racct->nas_port = $this->config["VSERVER_ID"];
        $racct->framed_ip_address  = $this->client->getIP ();
        $racct->calling_station_id = $this->client->getMAC ();
        $racct->gen_session_id();

        $serviceclass_attrib = defined('SERVICECLASS_ATTRIBUTE') ?
                               SERVICECLASS_ATTRIBUTE :
                               "WISPr-Billing-Class-Of-Service";

        try {
          $prepareData = array (
            "IP" => $this->client->getIP (),
            "Username" => $_POST['user'],
            "SessionID" => $racct->session_id,
            "MAC" => $this->client->getMAC (),
            "Session-Timeout" => $rauth->getAttribute('session_timeout'),
            "Bandwidth-Max-Down" => $rauth->getAttribute('WISPr-Bandwidth-Max-Down'),
            "Bandwidth-Max-Up" => $rauth->getAttribute('WISPr-Bandwidth-Max-Up'),
            "Class-Of-Service" => $rauth->getAttribute($serviceclass_attrib),
          );
          $result = $this->xmlrpc->do_startsession($this->config['VSERVER_ID'], $prepareData);
          if (strstr($result,"Client already login")) {
            $this->message = get_message('ERR_ALREADY_LOGIN');
          } else if (strstr($result, "Greeting")) {
            $split = explode ("Mapping ", $result);
            $called_station_id = $split[1];
            if (!empty ($called_station_id))
              $racct->called_station_id = $called_station_id;

            $racct->acctStart();
            $this->authenticated = true;
          } else if (strstr($result, "Invalid IP Address")) {
            $this->message = get_message('ERR_INVALID_IP');
          }
        } catch (XML_RPC2_FaultException $e) {
          $this->message = get_message('ERR_CONNECT_SERVER');
          $this->failed  = true;
          $this->message_delay = 10;
        } catch (Exception $e) {
          $this->message = get_message('ERR_CONNECT_SERVER');
          $this->failed  = true;
          $this->message_delay = 10;
        }
      } else {
        if ($rauth->isLoggedIn()) {
          $this->message = get_message('ERR_MAXIMUM_LOGIN');
          $this->failed  = true;
        } else if ($rauth->isTimeout()) {
          $this->message = get_message('ERR_USER_EXPIRED');
          $this->failed  = true;
        } else {
          $this->message = get_message('ERR_INVALID_USERNAME_OR_PASSWORD');
          $this->failed  = true;
        }
      }
    }
  }
}

class RahuAuthenLogout extends RahuAuthen {
  private $refresh_interval = 300;

  public function start () {
    $this->state = "logout";
    parent::start ();
    $this->render ();
  }

  protected function onUnAuthenticated () {
    $location_tpl = "%s://%s%s/login.php?sss=%d";
    $this->redirect_url =
      sprintf ($location_tpl,
               $this->config['NAS_LOGIN_PROTO'],
               $this->config['NAS_LOGIN_HOST'],
               !empty ($this->config['NAS_LOGIN_PORT']) ?
                 ":" . $this->config['NAS_LOGIN_PORT'] : "",
               time ());

    if (empty ($this->message)) {
      header ("Location: " . $this->redirect_url);
    }
  }

  protected function onAuthenticated () {
    if ($this->sessioninfo["session_timeout"] > 0) {
      header("Refresh: " . $this->refresh_interval . "; url=" . $_SERVER['REQUEST_URI']);
    }

    $session_start   = $this->sessioninfo['session_start'];
    $session_start   = $this->formatDateTime($session_start);
    $session_timeout = $this->sessioninfo['session_timeout'];
    $session_end     = $session_timeout == 0 ? _("Never") :
                       $this->formatDateTime($session_timeout);

    $session_timeout_label = $session_timeout == 0 ?
                               _("Session Time") :_("Session Remain Time");
    $session_timeout_remain = $session_timeout == 0 ?
      $this->formatSeconds(time() - $this->sessioninfo['session_start']) :
      $this->formatSeconds($this->sessioninfo['session_timeout'] - time());

    $request_url = @urldecode ($_GET['request_url']);
    $request_url_text = strlen($request_url) < 20 ?
                          $request_url : substr($request_url, 0, 20) . " ...";
    $request_url_link = sprintf ("<a href='%s' target='_blank'>%s</a>",
                                 $request_url, $request_url_text);

    $download_speed = intval ($this->sessioninfo["download_speed"]);
    $upload_speed = intval ($this->sessioninfo["upload_speed"]);

    if ($download_speed > 0) {
      $download_speed = $this->formatBytes ($download_speed) . "bps";
    } else {
      $download_speed = _("Unlimit");
    }

    if ($upload_speed > 0) {
      $upload_speed = $this->formatBytes ($upload_speed) . "bps";
    } else {
      $upload_speed = _("Unlimit");
    }

    $speed_text = sprintf ("%s / %s", $download_speed, $upload_speed);

    $download_bytes = intval ($this->sessioninfo["download_bytes"]);
    $upload_bytes   = intval ($this->sessioninfo["upload_bytes"]);
    if ($download_bytes > 0 || $upload_bytes > 0) {
      $data_transfer_text = sprintf ("%sB / %sB",
                                     $this->formatBytes ($download_bytes),
                                     $this->formatBytes ($upload_bytes));
    }

    array_push ($this->userinfo,
                array (_("Username") => $this->sessioninfo["username"]));
    array_push ($this->userinfo, array (_("Speed") => $speed_text));

    if ($download_bytes > 0 || $upload_bytes > 0) {
      array_push ($this->userinfo, array (_("Data") => $data_transfer_text));
    }

    array_push ($this->userinfo, array (_("Session Start") => $session_start));
    array_push ($this->userinfo, array (_("Session End") => $session_end));
    array_push ($this->userinfo,
                array ($session_timeout_label => $session_timeout_remain));
    array_push ($this->userinfo,
                array (_("Request URL") => $request_url_link));
  }

  private function formatSeconds($secs) {
    $units = array (
      "day"     => array ("unit" => _("day"),
                          "plural_unit" => _("days"),
                          "div" => 24*3600),
      "hour"    => array ("unit" => _("hour"),
                          "plural_unit" => _("hours"),
                          "div" =>    3600),
      "minute"  => array ("unit" => _("minute"),
                          "plural_unit" => _("minutes"),
                          "div" =>      60),
      "second"  => array ("unit" => _("second"),
                          "plural_unit" => _("seconds"),
                          "div" =>       1),
    );

    if ($secs == 0)
      return "$secs " . $units["second"]["plural_unit"];

    $s = "";

    foreach ($units as $unit) {
      if ($n = intval($secs / $unit["div"])) {
        $s .= "$n " . (abs($n) > 1 ? $unit["plural_unit"] : $unit["unit"]) . ", ";
        $secs -= $n * $unit["div"];
      }
    }

    return substr ($s, 0, -2);
  }

  private function formatBytes ($size, $precision = 2) {
    $base = log($size) / log(1024);
    $suffixes = array('', 'k', 'M', 'G', 'T');

    $ret_size = round(pow(1024, $base - floor($base)), $precision);
    $precision = floor ($ret_size) == $ret_size ? 0 : $precision;

    return number_format ($ret_size, $precision) .  " " .
           $suffixes[floor($base)];
  }

  private function formatDateTime ($datetime) {
    return strftime('%e %B %EY %H:%M:%S', $datetime);
  }

  protected function onSubmit () {
    if (!empty($_POST['do_logout'])) {
      try {
        $result = $this->xmlrpc->do_stopsession($this->config['VSERVER_ID'], $this->client->getIP (), $this->client->getMAC (),
                                                RADIUS_TERM_USER_REQUEST);
        if ($result === true) {
          $this->message = get_message('OK_USER_LOGOUT');
          $this->authenticated = false;
        } else {
          $this->message = get_message('ERR_LOGOUT_FAILED');
          $this->failed  = true;
        }
      } catch (XML_RPC2_FaultException $e) {
        $this->message = get_message('ERR_CONNECT_SERVER');
        $this->failed  = true;
        $this->message_delay = 10;
      } catch (Exception $e) {
        $this->message = get_message('ERR_CONNECT_SERVER');
        $this->failed  = true;
        $this->message_delay = 10;
      }
    }
  }
}
?>
