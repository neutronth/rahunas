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

session_start();
ob_start();
require_once 'rahu_radius.class.php';
require_once 'rahu_xmlrpc.class.php';
require_once 'rahu_i18n.class.php';
require_once 'rahu_langsupport.php';
require_once 'rahu_render.class.php';
require_once 'getmacaddr.php';
require_once 'config.php';
require_once 'header.php';
require_once 'messages.php';
require_once 'networkchk.php';

// Setup I18N
$i18n = new RahuI18N ($rahu_langsupport);
$i18n->localeSetup ();

$ip = $_SERVER['REMOTE_ADDR'];
$config = get_config_by_network($ip, $config_list);
$vserver_id = $config["VSERVER_ID"];

$forward = false;
$LogoutURL  = $config['NAS_LOGIN_PROTO'] . "://" . $config['NAS_LOGIN_HOST'];
$LogoutURL .= !empty($config['NAS_LOGIN_PORT']) ? 
                ":" . $config['NAS_LOGIN_PORT'] : "";
$LogoutURL .= "/logout.php";
$RequestURL = empty($_GET['request_url']) ? 
                $config['DEFAULT_REDIRECT_URL']
                : urldecode($_GET['request_url']);
$_SESSION['request_url'] = $RequestURL;

// Verify if the user already login
$xmlrpc = new rahu_xmlrpc_client();
$xmlrpc->host = $config["RAHUNAS_HOST"];
$xmlrpc->port = $config["RAHUNAS_PORT"];
try {
  $retinfo = $xmlrpc->do_getsessioninfo($vserver_id, $ip);
  if (is_array($retinfo) && !empty($retinfo['session_id'])) {
    $forward = true;
  }
} catch (XML_RPC2_FaultException $e) {
  $message = get_message('ERR_CONNECT_SERVER');
  $forward = false;
} catch (XML_RPC2_CurlExeption $e) {
  $message = get_message('ERR_CONNECT_SERVER');
  $forward = false;
} catch (Exception $e) {
  $message = get_message('ERR_CONNECT_SERVER');
  $forward = false;
}

if (!empty($_POST['user']) && !empty($_POST['passwd'])) {
  $_POST['user'] = trim($_POST['user']);

  $message = "";
  $rauth = new rahu_radius_auth ($_POST['user'], $_POST['passwd'], $config['RADIUS_ENCRYPT']);
  $rauth->host = $config["RADIUS_HOST"];
  $rauth->port = $config["RADIUS_AUTH_PORT"];
  $rauth->secret = $config["RADIUS_SECRET"];
  $rauth->start();

  if ($rauth->isError()) {
    $message = get_message('ERR_CONNECT_RADIUS');
  } else if ($rauth->isAccept()) {
    $message = get_message('OK_USER_AUTHORIZED');
    $forward = true;
    $racct = new rahu_radius_acct ($_POST['user']);
    $racct->host = $config["RADIUS_HOST"];
    $racct->port = $config["RADIUS_ACCT_PORT"];
    $racct->secret = $config["RADIUS_SECRET"];
    $racct->nas_identifier = $config["NAS_IDENTIFIER"];
    $racct->nas_ip_address = $config["NAS_IP_ADDRESS"];
    $racct->nas_port = $config["NAS_PORT"];
    $racct->framed_ip_address  = $_SERVER['REMOTE_ADDR'];
    $racct->calling_station_id = returnMacAddress();
    $racct->gen_session_id();

    $serviceclass_attrib = defined('SERVICECLASS_ATTRIBUTE') ?
                           SERVICECLASS_ATTRIBUTE :
                           "WISPr-Billing-Class-Of-Service";

    try {
      $prepareData = array (
        "IP" => $ip,
        "Username" => $_POST['user'],
        "SessionID" => $racct->session_id,
        "MAC" => returnMacAddress(),
        "Session-Timeout" => $rauth->attributes['session_timeout'],
        "Bandwidth-Max-Down" => $rauth->attributes['WISPr-Bandwidth-Max-Down'],
        "Bandwidth-Max-Up" => $rauth->attributes['WISPr-Bandwidth-Max-Up'],
        "Class-Of-Service" => $rauth->attributes[$serviceclass_attrib],
      );
      $result = $xmlrpc->do_startsession($vserver_id, $prepareData);
      if (strstr($result,"Client already login")) {
        $message = get_message('ERR_ALREADY_LOGIN');
        $forward = false;
      } else if (strstr($result, "Greeting")) {
        $split = explode ("Mapping ", $result);
        $called_station_id = $split[1];
        if (!empty ($called_station_id))
          $racct->called_station_id = $called_station_id;

        $racct->acctStart();
      } else if (strstr($result, "Invalid IP Address")) {
        $message = get_message('ERR_INVALID_IP');
        $forward = false;
      }
    } catch (XML_RPC2_FaultException $e) {
      $message = get_message('ERR_CONNECT_SERVER');
      $forward = false;
    } catch (Exception $e) {
      $message = get_message('ERR_CONNECT_SERVER');
      $forward = false;
    }
  } else {
    if ($rauth->isLoggedIn()) {
      $message = get_message('ERR_MAXIMUM_LOGIN');
    } else if ($rauth->isTimeout()) {
      $message = get_message('ERR_USER_EXPIRED');
    } else {
      $message = get_message('ERR_INVALID_USERNAME_OR_PASSWORD');
    }
  }
}

if ($forward) {
  $_SESSION['firstlogin'] = true;
}
?>

<?php
// Login box
$request_uri = $_SERVER['REQUEST_URI'];

$loginbox = "<form name='login' action='$request_uri' method='post'>" .
            "<table>" .
            "<tr><td id='rh_login_text'>" . _("Username") . "</td>" .
            "<td><input type='text' name='user' size='22'></td></tr>" .
            "<tr><td id='rh_login_text'>" . _("Password") . "</td>" .
            "<td><input type='password' name='passwd' size='22'></td></tr>" .
            "<tr><td>&nbsp;</td>" .
            "<td><input type='submit' value='" . _("Login") . "' id='rh_login_button'>" .
            "</td></tr>" .
            "</table>" .
            "</form>";

$forward_script  = $forward ? "window.open('$RequestURL');" : "";
$forward_script .= $forward ? "self.location.replace('$LogoutURL');" : "";
$waiting_show  = $forward ? "visible_hide(wt, 'show');" 
                          : "visible_hide(wt, 'hide');";
$message_show  = !empty($message) ? "visible_hide(msg, 'show');" 
                                  : "visible_hide(msg, 'hide');";
$hide_wait = !empty($message) ? "setTimeout('hide_wait();', 2000);\n" : "";
$force_forward = $hide_wait == "" && $forward ? 
                   "self.location.replace('$LogoutURL');" : "";

$loginscript = "<script>" .
               "var msg=(document.all);\n" .  
               "var ns4=document.layers;\n" .
               "var ns6=document.getElementById&&!document.all;\n" .
               "var ie4=document.all;\n" .
               "if (ns4)" .
               "  msg=document.rh_message;\n" .
               "else if (ns6)" .
               "  msg=document.getElementById('rh_message').style;\n" .
               "else if (ie4)" .
               "  msg=document.all.rh_message.style;\n\n" .
               "var wt=(document.all);\n" .  
               "if (ns4)" .
               "  wt=document.rh_waiting;\n" .
               "else if (ns6)" .
               "  wt=document.getElementById('rh_waiting').style;\n" .
               "else if (ie4)" .
               "  wt=document.all.rh_waiting.style;\n\n" .
               "function visible_hide(obj, type)\n" .
               "{\n" .
               "  if(type == 'show') {\n" .
               "    if(ns4){obj.visibility='visible';} \n" .
               "    else if (ns6||ie4) obj.display='block';\n".
               "  } else {\n".
               "    if(ns4){obj.visibility='hidden';} \n" .
               "    else if (ns6||ie4) obj.display='none';\n".
               "  }\n".
               "}\n".
               "function hide_wait() {\n".
               "  visible_hide(msg, 'hide');\n".
               "  visible_hide(wt, 'hide');\n".
               "  $forward_script \n" .
               "}\n".
               "  $message_show \n".
               "  $waiting_show \n".
               "  $hide_wait \n".
               "  $force_forward \n".
               "</script>";
$watting_script="";

$waiting  = "<div id='rh_waiting'><img src='loading.gif'></div>";
$loginmsg = "<div id='rh_message'>$message</div>";
$loginbox .= $waiting;
$loginbox .= $loginmsg;
?>

<?php
$tpl = new RahuRender ($config['UAM_TEMPLATE']);
$tpl->setString ("<!-- Title -->", $config["NAS_LOGIN_TITLE"]);
$tpl->setString ("<!-- Login -->", $loginbox);
$tpl->setString ("<!-- JavaScript -->", $loginscript);
$tpl->setString ("<!-- LanguageList -->", $i18n->getLangList("<li>", "</li>"));
$tpl->setString ("<!-- ChangePassword -->", "<li><a href='/chpwd.php'>" .
                    _("Change Password") . "</a></li>");
$tpl->setString ("<body", "<body onload='document.login.user.focus();'");
$tpl->render ();

ob_end_flush();
?>
