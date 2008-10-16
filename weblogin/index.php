<?php
require_once 'config.php';
require_once 'header.php';

$forward_uri = $config['NAS_LOGIN_PROTO'] . "://" .
               $config['NAS_LOGIN_HOST'] .
							 "/login.php?sss=" . time();
?>
<html>
<head>
<meta http-equiv="Cache-Control" content="no-cache, must-revalidate">
<meta http-equiv="Pragma" content="no-cache">
<meta http-equiv="Expires" content="0">
<title>RahuNAS - Authentication System</title>
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
</style>
</head>
<body>
<center>
<div><img src="/loading.gif"></div>
<div>If it is not redirecting within 3 seconds, click <a href="<?php echo $forward_uri ?>">Login Page</a></div>
</center>

<script language="JavaScript">
function redirecting() 
{
  redirect_uri = "<?php echo $forward_uri ?>&request_url=" + escape(location);
  location.replace(redirect_uri);
}
setTimeout("redirecting();", 1000);
</script>
</body>
</html>
