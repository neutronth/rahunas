<?php
/*
  Copyright (c) 2008-2014, Neutron Soutmun <neo.neutron@gmail.com>
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

require_once 'rahu_authen.class.php';
require_once 'rahu_i18n.class.php';

header ("HTTP/1.1 511 Network Authentication Required");

$client = new RahuClient ();
$rahuconfig = new RahuConfig ($client);
$config =& $rahuconfig->getConfig ();
$i18n   = new RahuI18N ();
$i18n->localeSetup ();

$vserver_id  = $config["VSERVER_ID"];
$request_url = "http://" . $_SERVER['HTTP_HOST'] . $_SERVER['REQUEST_URI'];
$forward_tpl = "%s://%s%s/%s&request_url=%s";
$forward_uri = sprintf ($forward_tpl,
                         $config['NAS_LOGIN_PROTO'],
                         $config['NAS_LOGIN_HOST'],
                         !empty($config['NAS_LOGIN_PORT']) ?
                           ":" . $config['NAS_LOGIN_PORT'] : "",
                         "login.php?sss=" . time(),
                         urlencode ($request_url));
?>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<meta http-equiv="Cache-Control" content="no-cache, must-revalidate">
<meta http-equiv="Pragma" content="no-cache">
<meta http-equiv="Expires" content="0">
<meta http-equiv="refresh" content="1; url=<?php echo $forward_uri ?>">
<title>RahuNAS Authentication</title>
<style>
body {
  font-family: sans-serif;
  font-size: 10pt;
}
a {
  text-decoration: none;
}

a:hover {
  font-weight: bolder;
  color: #FF0000;
}

#loadingpic {
  padding-top: 100px;
  padding-bottom: 30px;
}
</style>
</head>
<body>
  <center>
  <div id="loadingpic"><img src="/loading.gif"></div>
  <div><?php echo _("If it is not redirecting within 3 seconds") . "," ._("click"); ?>
    <a href="<?php echo $forward_uri ?>"> <?php echo _("Login Page"); ?></a>
  </div>
  </center>
</body>
</html>
