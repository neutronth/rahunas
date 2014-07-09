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
    $this->buffer = str_replace("images/", $this->tplpath."images/",
                                $this->buffer);
    $this->buffer = str_replace("css/", $this->tplpath."css/", $this->buffer);
    $this->buffer = str_replace("js/", $this->tplpath."js/", $this->buffer);
  }

  function setString ($pattern, $replace) {
    $this->buffer = str_replace ($pattern, $replace, $this->buffer);
  }

  function render () {
    echo $this->buffer;
  }
}
?>
