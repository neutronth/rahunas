<?php
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

  if (!empty($_GET['language']))
    $_SESSION['language'] = $_GET['language'];

  $selected_lang =& $lang[$_SESSION['language']];

  setlocale(LC_ALL, $selected_lang['code']);
  bindtextdomain('messages', './locale');
  textdomain('messages');
?>
