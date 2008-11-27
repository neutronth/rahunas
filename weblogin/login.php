<?php
session_start();
ob_start();
require_once 'rahu_radius.class.php';
require_once 'rahu_xmlrpc.class.php';
require_once 'getmacaddr.php';
require_once 'config.php';
require_once 'header.php';

if (!empty($_POST['user']) && !empty($_POST['passwd'])) {
  $ip = $_SERVER['REMOTE_ADDR'];
  $forward = false;
  $LogoutURL  = $config['NAS_LOGIN_PROTO'] . "://" . $config['NAS_LOGIN_HOST'];
  $LogoutURL .= !empty($config['NAS_LOGIN_PORT']) ? ":" . $config['NAS_LOGIN_PORT'] : "";
  $LogoutURL .= "/logout.php";

  $RequestURL = empty($_GET['request_url']) ? 
                   $config['DEFAULT_REDIRECT_URL']
                 : urldecode($_GET['request_url']);
  $_SESSION['request_url'] = $RequestURL;
  $message = "";
  $rauth = new rahu_radius_auth ($_POST['user'], $_POST['passwd'], $config['RADIUS_ENCRYPT']);
  $rauth->host = $config["RADIUS_HOST"];
  $rauth->port = $config["RADIUS_AUTH_PORT"];
  $rauth->secret = $config["RADIUS_SECRET"];
  $rauth->start();

  if ($rauth->isError()) {
    $message = "ไม่สามารถเชื่อมต่อ กับเครื่องตรวจสอบสิทธิ์ได้";
  } else if ($rauth->isAccept()) {
    $message = "ผู้ใช้ ผ่านการตรวจสอบสิทธิ์ รอสักครู่ ";
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

    // Verify if the user already login
    $xmlrpc = new rahu_xmlrpc_client();
    $xmlrpc->host = $config["RAHUNAS_HOST"];
    $xmlrpc->port = $config["RAHUNAS_PORT"];

    try {
      $prepareData = array (
        "IP" => $ip,
        "Username" => $_POST['user'],
        "SessionID" => $racct->session_id,
        "MAC" => returnMacAddress(),
        "Session-Timeout" => $rauth->attributes['session_timeout'],
        "Bandwidth-Max-Down" => $rauth->attributes['WISPr-Bandwidth-Max-Down'],
        "Bandwidth-Max-Up" => $rauth->attributes['WISPr-Bandwidth-Max-Up']);
      $result = $xmlrpc->do_startsession($prepareData);
      if (strstr($result,"Client already login")) {
        $message = "ผู้ใช้นี้ ได้ใช้สิทธิ์เข้าใช้งานแล้ว";
        $forward = false;
      } else if (strstr($result, "Greeting")) {
        $racct->acctStart();
      } else if (strstr($result, "Invalid IP Address")) {
        $message = "หมายเลข IP Address ประจำเครื่อง ไม่ถูกต้อง";
        $forward = false;
      }
    } catch (XML_RPC2_FaultException $e) {
      $message = "ผิดพลาด! ไม่สามารถเชื่อมต่อเครื่องแม่ข่ายได้";
      $forward = false;
    } catch (Exception $e) {
      $message = "ผิดพลาด! ไม่สามารถเชื่อมต่อเครื่องแม่ข่ายได้";
      $forward = false;
    }
  } else {
    if ($rauth->isLoggedIn()) {
      $message = "ไม่สามารถเข้าระบบได้ มีการเข้าระบบซ้ำ";
    } else if ($rauth->isTimeout()) {
      $message = "ไม่สามารถเข้าระบบได้ หมดเวลาการใช้งาน";
    } else {
      $message = "ไม่พบผู้ใช้นี้ หรือรหัสผ่านผิด";
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
$loginbox = "<style>" .
            "#rh_login_text { font-weight: bolder; }\n" .
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
            "<table>" .
            "<tr><td id='rh_login_text'>Username</td>" .
            "<td><input type='text' name='user' size='22'></td></tr>" .
            "<tr><td id='rh_login_text'>Password</td>" .
            "<td><input type='password' name='passwd' size='22'></td></tr>" .
            "<tr><td>&nbsp;</td>" .
            "<td><input type='submit' value='Login' id='rh_login_button'>" .
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
  $html_buffer = str_replace("<!-- Title -->", $config["NAS_LOGIN_TITLE"], 
                             $html_buffer);
  $html_buffer = str_replace("<!-- Login -->", $loginbox, $html_buffer);
  $html_buffer = str_replace("<!-- JavaScript -->", $loginscript, $html_buffer);
  $html_buffer = str_replace("<body", 
                             "<body onload='document.login.user.focus();'",
                             $html_buffer);
  print $html_buffer;
}


ob_end_flush();
?>
