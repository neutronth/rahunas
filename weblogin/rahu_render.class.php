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

require_once ("libs/Browscap.php");

use phpbrowscap\Browscap;

class RahuRender {
  private $template;
  private $tplpath;
  private $tplfile;
  private $buffer;

  function RahuRender ($template) {
    $tpl_ok = false;

    $bc = new Browscap ("browscap-cache/");
    $bc->doAutoUpdate = false;
    $browser = $bc->getBrowser ();

    /* Fallback to plain version template for old and unsupported browser */
    $template_file = $template;
    if (($browser->Browser == "IE" && $browser->Version < 8) ||
         !$browser->JavaScript)
      $template_file = $template . "-plain";

    $this->template = $template;
    $this->tplpath = "templates/" . $template . "/";
    $this->tplfile = $this->tplpath . $template_file . ".html";

    $handle = @fopen ($this->tplfile, "r");
    $this->buffer = "";
    if ($handle) {
      $tpl_ok = true;

      while (!feof ($handle)) {
        $this->buffer .= fgets ($handle, 4096);
      }
    }
    fclose ($handle);

    if (!$tpl_ok)
      die ("Could not load template file!");
   
    /* Replace pre-defined dir with template path */
    $this->setString ("images/", $this->tplpath . "images/");
    $this->setString ("css/", $this->tplpath . "css/");
    $this->setString ("js/", $this->tplpath . "js/");

    $this->setupSlideImages ();
  }

  function setString ($pattern, $replace) {
    $this->buffer = str_replace ($pattern, $replace, $this->buffer);
  }

  function setupSlideImages () {
    if (strstr ($this->buffer, "<!-- Slide Images Container -->") == false) {
      return;
    }

    $images_dir = $this->tplpath . "images/";
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

    if (!empty ($images)) {
      $indicators = "";
      $slides = "";
      $indicator_tpl =
        "<li data-target='#carousel-rahunas' data-slide-to='##id##' " .
        "##class##></li>";
      $slide_tpl = "<div class='item##active##'><img src='##image##'></div>";
      $carousel_tpl =
        "<ol class='carousel-indicators' style='bottom: 70px;'>" .
        "  ##indicators##" .
        "</ol>" .
        "<div class='carousel-inner'>##slides##</div>" .
        "<a class='left carousel-control' href='#carousel-rahunas' " .
          "data-slide='prev'>" .
        "  <span class='glyphicon glyphicon-chevron-left'></span>" .
        "</a>" .
        "<a class='right carousel-control' href='#carousel-rahunas' ".
          "data-slide='next'>" .
        "  <span class='glyphicon glyphicon-chevron-right'></span>" .
        "</a>";

      $i = 0;
      foreach ($images as $image) {
        $active = $i == 0 ? " active" : "";
        $class  = $i == 0 ? "class='active'" : "";
        $cur_indicator = $indicator_tpl;
        $cur_slide     = $slide_tpl;
        $cur_indicator = str_replace ("##id##", $i , $cur_indicator);
        $cur_indicator = str_replace ("##class##", $class , $cur_indicator);
        $cur_slide     = str_replace ("##active##", $active, $cur_slide);
        $cur_slide     = str_replace ("##image##", $image, $cur_slide);

        $indicators .= $cur_indicator;
        $slides .= $cur_slide;

        $i++;
      }

      $carousel = str_replace ("##indicators##", $indicators, $carousel_tpl);
      $carousel = str_replace ("##slides##", $slides, $carousel);

      $this->buffer = str_replace ("<!-- Slide Images Container -->",
                                   $carousel, $this->buffer);
    }
  }

  function render () {
    echo $this->buffer;
  }
}
?>
