<?php
/*
  Copyright (c) 2011, Neutron Soutmun <neutron@rahunas.org>
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

class RahuI18N {
  var $langlist;
  var $textdomain;
  var $localepath;

  function RahuI18N ($langlist, $textdomain = "rahunas-weblogin",
                     $localepath = "/usr/share/locale") {
    $this->langlist   = $langlist;
    $this->textdomain = $textdomain;
    $this->localepath  = $localepath;
  }

  public function localeSetup () {
    if (!empty ($_GET['language'])) {
      setcookie ("rh_language", $_GET['language']);
      $selected_lang =& $this->langlist[$_GET["language"]];
    } else if (empty ($_COOKIE["rh_language"])) {
      $accept_lang = $this->getAcceptLanguage ();
      $lang = empty ($accept_lang) ? "English" : $accept_lang;
      setcookie ("rh_language", $lang);
      $selected_lang =& $this->langlist[$lang];
    } else {
      $selected_lang =& $this->langlist[$_COOKIE["rh_language"]];
    }


    if (!empty ($selected_lang['code'])) {
      setlocale (LC_ALL, $selected_lang['code']);
      bindtextdomain ($this->textdomain, $this->localepath);
      textdomain ($this->textdomain);
    }
  }

  public function getLangList ($begintag = "", $endtag = "") {
    $languages = "";
    $link_tpl = $begintag . "<a href='%s'>%s</a>" . $endtag;
    $qs = $this->explodeQueryString ($_SERVER['QUERY_STRING']);

    if (is_array ($this->langlist)) {
      foreach ($this->langlist as $lang=>$data) {
        $q = $qs;
        $q['language'] = $lang;
        $link = $_SERVER['PHP_SELF'] . "?" . $this->implodeQueryString ($q);
        $languages .= sprintf ($link_tpl, $link, $data["name"]);
      }
    }

    return $languages;
  }

  private function getAcceptLanguage () {
    if (!isset ($_SERVER['HTTP_ACCEPT_LANGUAGE'])) {
      return "";
    }

    preg_match_all ('/([a-z]{1,8}(-[a-z]{1,8})?)\s*(;\s*q\s*=\s*(1|0\.[0-9]+))?/i', $_SERVER['HTTP_ACCEPT_LANGUAGE'], $lang_parse);

    if (count ($lang_parse[1])) {
      $langs = array_combine ($lang_parse[1], $lang_parse[4]);

      foreach ($langs as $lang => $val) {
        if ($val === '')
          $langs[$lang] = 1;
      }

      arsort ($langs, SORT_NUMERIC);

      foreach ($langs as $lang => $val) {
        foreach ($this->langlist as $supportlang_k => $supportlang) {
          $code = strtolower ($supportlang['code']);
          $code = str_replace ("_", "-", $code);

          if (strstr ($code, $lang) !== FALSE) {
            return $supportlang_k;
          }
          
        }
      }
    }

    return "";
  }

  private function explodeQueryString ($q) {
    $qs = explode ("&", $q);
    $q_key = array ();
    $q_val = array ();
    if (is_array ($qs)) {
      foreach ($qs as $qq) {
        $sep = explode ("=", $qq);
        $q_key[] = $sep[0];
        $q_val[] = $sep[1];
      }
    }

    $qr = array_combine ($q_key, $q_val);
    return $qr;
  }

  private function implodeQueryString ($q) {
    $qr = array ();
    if (is_array ($q)) {
      foreach ($q as $k=>$v) {
        $qr[] = $k . "=" . $v;
      }
      $qr = implode ("&", $qr);
    }

    return $qr;
  }
}
