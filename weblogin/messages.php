<?php
function get_message($message) {
  $MSG['ERR_CONNECT_SERVER'] = _("Error! Could not connect to the server");
  $MSG['ERR_CONNECT_RADIUS'] = _("Error! Could not connect to the authenticator server");
  $MSG['ERR_MAXIMUM_LOGIN']  = _("Error! User has been reach the maximum login");
  $MSG['ERR_ALREADY_LOGIN']  = _("Error! User already login");
  $MSG['ERR_USER_EXPIRED']   = _("Error! User expired");
  $MSG['ERR_INVALID_IP']     = _("Error! Invalid IP address");
  $MSG['ERR_INVALID_USERNAME_OR_PASSWORD'] = 
    _("Error! Invalid username or password");
  $MSG['ERR_LOGOUT_FAILED']  = _("Error! Logout failed! please try again");
  $MSG['ERR_PLEASE_LOGIN']   = _("Error! Please login");
  
  $MSG['OK_USER_AUTHORIZED'] = 
    _("Success! User has been authorized, please wait a moment");
  $MSG['OK_USER_LOGOUT'] = _("Success! User has been logout");

  return $MSG[$message];
}
?>
