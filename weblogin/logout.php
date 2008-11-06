<?php
session_start();
ob_start();
require_once 'rahu_radius.class.php';
require_once 'rahu_xmlrpc.class.php';
require_once 'getmacaddr.php';
require_once 'config.php';
require_once 'header.php';

$current_url = $_SERVER['REQUEST_URI'];
$interval = 60;
$auto_refresh = false;

if ($auto_refresh) {
  header("Refresh: $interval; url=$current_url");
}

$forward_uri  = $config['NAS_LOGIN_PROTO'] . "://" . $config['NAS_LOGIN_HOST'];
$forward_uri .= !empty($config['NAS_LOGIN_PORT']) ? ":" . $config['NAS_LOGIN_PORT'] : "";
$forward_uri .= "/login.php?sss=" . time();

$request_url = $_SESSION['request_url'];

$ip = $_SERVER['REMOTE_ADDR'];
$xmlrpc = new rahu_xmlrpc_client();
$xmlrpc->host = $config["RAHUNAS_HOST"];
$xmlrpc->port = $config["RAHUNAS_PORT"];
$valid = false;
$isinfo = false;
$isstopacct = false;
$info = array();
$retinfo = $xmlrpc->do_getsessioninfo($ip);
if (is_array($retinfo)) {
  // Send stop accounting to Radius
  $ip =& $retinfo["ip"];
  $username =& $retinfo["username"];
  $session_id =& $retinfo["session_id"];
	$session_start =& $retinfo["session_start"];
  $mac_address =& $retinfo["mac_address"];
  $isinfo = true;
} else {
  $valid = false;
}

if (!empty($_POST['do_logout'])) {
  if ($isinfo) {
	  $result = $xmlrpc->do_stopsession($ip, returnMacAddress());
	  if ($result === true) {
      $valid = false;
		  $message = "ทำการ 'Logout' สำเร็จ";
      $isstopacct = true;
   	} else {
      $valid = false;
		  $message = "ไม่สามารถ 'เลิกใช้งาน' ได้ในขณะนี้ กรุณาลองใหม่อีกครั้ง";
		  $show_info = true;
	  }
  }

  if ($isstopacct) {
    // Send account stop to radius
    $racct = new rahu_radius_acct ($username);
    $racct->host = $config["RADIUS_HOST"];
    $racct->port = $config["RADIUS_ACCT_PORT"];
    $racct->secret = $config["RADIUS_SECRET"];
    $racct->nas_identifier = $config["NAS_IDENTIFIER"];
    $racct->nas_ip_address = $config["NAS_IP_ADDRESS"];
    $racct->nas_port = $config["NAS_PORT"];
    $racct->framed_ip_address  = $ip;
    $racct->calling_station_id = $mac_address;
    $racct->terminate_cause = RADIUS_TERM_USER_REQUEST;
    $racct->session_id    = $session_id;
    $racct->session_start = $session_start;
    $racct->acctStop();
  }
} else {
  $show_info = true;
}

if ($show_info) {
	$result = $xmlrpc->do_getsessioninfo($ip);
	if (is_array($result)) {
	  if (!empty($result['session_id'])) {
      $valid = true;
			$info = $result;
		} else {
      $message = "คุณยังไม่ได้เข้าใช้งานในระบบ";
		}
	}
}
?>

<?php
// Login box
$valid_text = !$valid ? "" : "" . 
"  <table id='bg'>" .
"  <tr>" .
"	  <td align='right'><b>Username:</b></td>" .
"	  <td>". $info['username']."</td>" .
"	</tr>" .
"  <tr>" .
"	  <td align='right'><b>Session Start:</b></td>" .
"		<td>". date('j F Y H:i', $info['session_start']) . "</td>" .
"	</tr>" .
"  <tr>" .
"	  <td align='right'><b>Session Time:</b></td>" .
"		<td>" . (time() - $info['session_start']) . " seconds</td>" .
"	</tr>" .
"" .
"  <tr>" .
"	  <td align='right'><b>User expired:</b></td>" .
"		<td>". ($info['session_timeout'] == 0 ? "Never" : date('j F Y H:i', $info['session_timeout'])) . "</td>" .
"	</tr>" .
"" .
"  <tr>" .
"	  <td align='right'><b>Request URL:</b></td>" .
"		<td><a href='$request_url' target='_new'>$request_url</a></td>" .
"	</tr>" .
"</table>".
"<table>".
" <tr>" .
"	  <td>&nbsp;<input type='hidden' name='do_logout' value='yes'></td>" .
"	  <td><input type='button' value='Go! Go! Go!' id='rh_goto_button' onClick='window.open(\"".$request_url."\");'></td>" .
"	  <td><input type='submit' value='Logout' id='rh_logout_button'></td>" .
"	</tr>" .
"</table>";
$request_uri = $_SERVER['REQUEST_URI'];
$loginbox = "<style>" .
            "#bg { background: #ffffff; width: 80%;}".
            "#rh_login_text { color: #000000; font-weight: bolder; }\n" .
            "#waiting { " .
            " position: absolute; ".
            " top: -215px;".
            " left: 170px;".
            "}".
            "#message { " .
            "  color: #000000;" .
            "  font-weight: bolder; ".
            "  padding: 2px;" .
            "  width: 80%;" .
            "  text-align: center;" .
            "  background: #FFFF99;" .
            "}\n" .
            "#rh_login_button {" .
            "  color: #000000;" .
            "  padding: 3px 10px 3px 10px;" .
            "  cursor: pointer;" .
            "}\n" .
            "</style>" . 
            "<form name='login' action='$request_uri' method='post'>" .
            "  $valid_text ". 
            "</form>";

$forward_script  = $valid == false ? "self.location.replace('$forward_uri');" : "";

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
               "  msg=document.message;\n" .
               "else if (ns6)" .
	             "  msg=document.getElementById('message').style;\n" .
               "else if (ie4)" .
               "  msg=document.all.message.style;\n\n" .
               "var wt=(document.all);\n" .  
               "if (ns4)" .
               "  wt=document.waiting;\n" .
               "else if (ns6)" .
               "  wt=document.getElementById('waiting').style;\n" .
               "else if (ie4)" .
               "  wt=document.all.waiting.style;\n\n" .
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

$waiting  = "<div id='waiting'><img src='loading.gif'></div>";
$loginmsg = "<div id='message'>$message</div>";
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
  while (!feof($handle)) {
    $html_buffer .= fgets($handle, 4096);
  }
  fclose($handle);

  $html_buffer = str_replace("images/", $tpl_path."images/", $html_buffer);
  $html_buffer = str_replace("<!-- Login -->", $loginbox, $html_buffer);
  $html_buffer = str_replace("<!-- JavaScript -->", $loginscript, $html_buffer);
  print $html_buffer;
}


ob_end_flush();
?>
