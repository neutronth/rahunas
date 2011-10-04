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

  This code is made possible thx to samples made by 
    Michael Bretterklieber <michael@bretterklieber.com> author of 
    the PHP PECL Radius package
*/


class UserDB {
  var $db_name;
  var $username;
  var $password;
  var $server;
  var $port;
  var $db_conn;
  var $pass_type;
  var $message;

  function UserDB($dbname, $username, $password,
                  $server = 'localhost', $port = '5432') {
    $this->db_name  = $dbname;
    $this->username = $username;
    $this->password = $password;
    $this->server   = $server;
    $this->port     = $port;
    $this->pass_type= "SHA1";
    $this->db_conn  = NULL;
  }

  function start() {
    $conn_str = "host=$this->server " .
                "port=$this->port " .
                "dbname=$this->db_name " .
                "user=$this->username " .
                "password=$this->password";
    $this->db_conn = pg_connect ($conn_str);
    if ($this->db_conn)
      return TRUE;
    else
      return FALSE;
  }

  function check_password ($username, $password) {
    $sql = "SELECT attribute, value FROM radcheck WHERE username = '$username'";
    $result = pg_query($this->db_conn, $sql); 

    if (!$result) {
      $this->message = _("Error: Could not get data from database");
      return FALSE;
    }
    $row = pg_fetch_row($result);
    if (!$row) {
      $this->message = _("Invalid username or password");
      return FALSE;
    }
    switch ($row[0]) {
      case "SHA-Password" :
        $checkpass = sha1($password);
        $this->pass_type = "SHA1";
        break;
      case "MD5-Password" :
        $checkpass = md5($password);
        $this->pass_type = "MD5";
        break;
      default:
        $this->pass_type = "ClearText";
        $checkpass = $password;
    }
    if ($row[1] == $checkpass) {
      return TRUE;
    } else {
      $this->message = _("Invalid username or password");
      return FALSE;
    }
  }

  function change_password ($username, $password) {
    switch ($this->pass_type) {
      case "SHA1": $newpass = sha1($password);
        break;
      case "MD5" : $newpass = md5($password);
        break;
      default:
        $newpass = $password;
    }
    $sql = "UPDATE radcheck SET value='$newpass' WHERE username='$username'";
    $result = pg_query($this->db_conn, $sql); 

    if (!$result) {
      $this->message = _("Error: Could not change password");
      return FALSE;
    } 
    return TRUE;
  }

  function get_message() {
    return $this->message;
  }

  function stop() {
    if ($this->db_conn != NULL) {
      pg_close ($this->db_conn);
    }
  }
}
?>
