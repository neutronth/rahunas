<?php
/*
  Copyright (c) 2011-2014, Neutron Soutmun <neutron@rahunas.org>
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

require_once ("libs/Browscap/Browscap.php");
require_once ("libs/Smarty/Smarty.class.php");

use phpbrowscap\Browscap;

class RahuRender {
  private $template;
  private $tplpath;
  private $tplfile;
  private $buffer;
  private $engine;
  private $engine_config = array( 
    "compile_dir" => "data/template/compile",
    "cache_dir" => "data/template/cache"
  );
                           

  function RahuRender ($template) {
    $tpl_ok = false;

    $bc = new Browscap ("data/browscap-cache/");
    $bc->doAutoUpdate = false;
    $browser = $bc->getBrowser ();

    /* Fallback to plain version template for old and unsupported browser */
    $template_file = "index";
    if (($browser->Browser == "IE" && $browser->Version < 8) ||
         !$browser->JavaScript)
      $template_file = "index-plain";

    $this->template = $template;
    $this->tplpath = "templates/" . $template . "/";
    $this->tplfile = $template_file . ".html";

    $this->engine = new Smarty ();
    $this->engine->setTemplateDir ($this->tplpath);
    $this->engine->setCompileDir ($this->engine_config["compile_dir"]);
    $this->engine->setCacheDir ($this->engine_config["cache_dir"]);

    $this->engine->assign ("template_path", $this->tplpath);

    $this->engine->assign ("label_language", _("Language"));
    $this->engine->assign ("label_form_username", _("Username"));
    $this->engine->assign ("label_form_password", _("Password"));
    $this->engine->assign ("label_button_login", _("Login"));
    $this->engine->assign ("label_button_logout", _("Logout"));
    $this->engine->assign ("label_button_ok", _("OK"));

    $this->setupSlideImages ();
  }

  public function setBrandTitle ($title) {
    $this->engine->assign ("brand_title", $title);
  }

  public function setLanguages ($languages) {
    $this->engine->assign ("languages", $languages);
  }

  public function setCurrentLanguage ($language) {
    $this->engine->assign ("current_language", $language);
  }

  public function setState ($state) {
    $this->engine->assign ("state", $state);
  }

  public function setMessage ($text, $type) {
    $message = array ("text" => $text, "type" => $type);
    $this->engine->assign ("message", $message);
  }

  public function setRedirect ($flag, $url = "", $delay = 1) {
    $redirect = array ("flag" => $flag, "url" => $url, "delay" => $delay);
    $this->engine->assign ("redirect", $redirect);
  }

  public function setUserRequestUrl ($url) {
    $this->engine->assign ("user_request_url", $url);
  }

  public function setUserInfo ($info) {
    $this->engine->assign ("user_info", $info);
  }

  public function setHelpLink ($link) {
    $this->engine->assign ("help_link", $link);
  }

  function setupSlideImages () {
    $this->engine->assign ("slide_images", array ());
    $images_dir = $this->tplpath . "images/photo/";
    $images = array ();
    if (is_dir ($images_dir)) {
      if ($dh = opendir ($images_dir)) {
        while (($file = readdir ($dh)) !== false) {
          $images_path = $images_dir . $file;
          $image_size = @getimagesize ($images_path);

          if ($image_size) {
            array_push ($images, $images_path);
          }
        }
      }
    }

    $this->engine->assign ("slide_images", $images);
  }

  function render () {
    header("Cache-Control: no-cache, must-revalidate");
    header("Expires: 0");
    header("Pragma: no-cache");

    $this->engine->display ($this->tplfile);
  }
}
?>
