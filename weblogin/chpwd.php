<?php
/*
  Copyright (c) 2008-2011, Suriya Soutmun <darksolar@gmail.com>
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
require_once 'header.php';
require_once 'locale.php';
require_once 'config.php';
require_once 'messages.php';
require_once 'networkchk.php';
require_once 'user.class.php';
require_once 'getmacaddr.php';
require_once 'radius-dbconfig.php';


$ip = $_SERVER['REMOTE_ADDR'];
$config = get_config_by_network($ip, $config_list);
$vserver_id = $config["VSERVER_ID"];

$forward_uri  = $config['NAS_LOGIN_PROTO'] . "://" . $config['NAS_LOGIN_HOST'];
$forward_uri .= !empty($config['NAS_LOGIN_PORT']) ? ":" . $config['NAS_LOGIN_PORT'] : "";
$forward_uri .= "/login.php?sss=" . time();
?>
<?php
// Login box
$request_uri = $_SERVER['REQUEST_URI'];
$forward = false;
if (!empty($_POST['user']) && !empty($_POST['passwd']) &&
    !empty($_POST['newpass']) && !empty($_POST['cfmpass'])) {
  $user = new UserDB ($radius_db,
                      $radius_login,
                      $radius_password,
                      $radius_server);
  if(!$user->start()) {
     $message =_("Can not connect database");
  }
  if($user->check_password($_POST['user'], $_POST['passwd'])) {
    if ($_POST['newpass'] != $_POST['cfmpass']) {
      $message = _("Confirm password missmatch");
    } else {
      if ($user->change_password($_POST['user'], $_POST['newpass'])) {
        $message = _("Password has been changed");
        $forward = true;
      } else {
        $message = $user->get_message();
      }
    }
  } else {
    $message = $user->get_message();
  }
  $user->stop();
  unset ($user);
} else {
  $message = _("Please insert all information");
}

$username = empty($_POST['user'])?"":$_POST['user'];

$loginbox = "<form name='login' action='$request_uri' method='post'>" .
            "<table>" .
            "<tr><td id='rh_login_text'>" . _("Username") . "</td>" .
            "<td><input type='text' name='user' value='$username' size='22'>" .
            "</td></tr>" .
            "<tr><td id='rh_login_text'>" . _("Password") . "</td>" .
            "<td><input type='password' name='passwd' size='22'></td></tr>" .
            "<tr><td>&nbsp;</td><td>&nbsp;</td></tr>".
            "<tr><td id='rh_login_text'>". _("New Password") . "</td>" .
            "<td><input type='password' name='newpass' size='22'></td></tr>" .
            "<tr><td id='rh_login_text'>". _("Confirm Password") . "</td>" .
            "<td><input type='password' name='cfmpass' size='22'></td></tr>" .
            "<tr><td>&nbsp;</td>" .
            "<td><input type='submit' value='" . _("Change Password") .
            "' id='rh_login_button'></td></tr>" .
            "</table>" .
            "</form>";
$forward_script  = $forward ? "self.location.replace('$forward_uri');" : "";
$waiting_show  = $forward ? "visible_hide(wt, 'show');" 
                          : "visible_hide(wt, 'hide');";
$message_show  = !empty($message) ? "visible_hide(msg, 'show');" 
                                  : "visible_hide(msg, 'hide');";
$hide_wait = !empty($message) ? "setTimeout('hide_wait();', 2000);\n" : "";

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
               "</script>";
$watting_script="";

$waiting  = "<div id='rh_waiting'><img src='loading.gif'></div>";
$loginmsg = "<div id='rh_message'>$message</div>";
$loginbox .= $waiting;
$loginbox .= $loginmsg;
?>

<?php
// Template loading
$tpl_path = "templates/" . $config['UAM_TEMPLATE'] . "/";
$tpl_file = $tpl_path . $config['UAM_TEMPLATE'] . ".html";
$handle = @fopen($tpl_file, "r");
$html_buffer = "";
if ($handle) {  
  $css = "<link rel='stylesheet' type='text/css' href='" . $tpl_path . "rahunas.css'>";
  $loginbox = $css . $loginbox;

  while (!feof($handle)) {
    $html_buffer .= fgets($handle, 4096);
  }
  fclose($handle);

  $html_buffer = str_replace("images/", $tpl_path."images/", $html_buffer);
  $html_buffer = str_replace("<!-- Title -->", $config["NAS_LOGIN_TITLE"], 
                             $html_buffer);
  $html_buffer = str_replace("<!-- Login -->", $loginbox, $html_buffer);
  $html_buffer = str_replace("<!-- JavaScript -->", $loginscript, $html_buffer);
  print $html_buffer;
}

ob_end_flush();
?>
