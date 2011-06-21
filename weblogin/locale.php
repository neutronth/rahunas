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

require_once "config.php";
require_once 'networkchk.php';

$ip = $_SERVER['REMOTE_ADDR'];
$config = get_config_by_network($ip, $config_list);
$vserver_id = $config["VSERVER_ID"];

function explode_querystring($query_string) {
  // Explode the query string into array
  $query_string = explode("&", $query_string);
  $query_key = array();
  $query_val = array();
  if (is_array($query_string)) {
    foreach ($query_string as $each_query) {
      $sep_query = explode("=", $each_query);
      $query_key[] = $sep_query[0];
      $query_val[] = $sep_query[1];
    }
  }
  $query = array_combine($query_key, $query_val);
  return $query;
}

function implode_querystring($query) { 
  // Combine array into query string
  $query_string = array();
  if (is_array($query)) {
    foreach ($query as $key=>$val) {
      $query_string[] = $key . "=" . $val; 
    } 
    
    $query_string = implode("&", $query_string);
  }
  return $query_string;
}

$main_query = explode_querystring($_SERVER['QUERY_STRING']);

// Languages list
$lang = array();
$lang['Thai']['name'] = 'ไทย';
$lang['Thai']['code'] = 'th_TH.UTF-8';
$lang['English']['name'] = 'English';
$lang['English']['code'] = 'en_US.UTF-8';

$lang_list = array();
$lang_link_template = "<a href='%s'>%s</a>";
foreach ($lang as $key=>$eachlang) {
  $query = $main_query;
  $query['language'] = $key;
  $link = $_SERVER['PHP_SELF'] . "?" . implode_querystring($query);
  $lang_list[] = sprintf($lang_link_template, $link, $eachlang['name']); 
}

echo "<div id='rh_language'>Language: " . implode(" | ", $lang_list) . "</div>";

if (empty($_SESSION['language']))
  $_SESSION['language'] = $config['DEFAULT_LANGUAGE'];

if (!empty($_GET['language']))
  $_SESSION['language'] = $_GET['language'];

$selected_lang =& $lang[$_SESSION['language']];

setlocale(LC_ALL, $selected_lang['code']);
bindtextdomain('messages', './locale');
textdomain('messages');
?>
