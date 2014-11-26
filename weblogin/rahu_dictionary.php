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

define ('RADIUS_RAHUNAS_FAIR_USAGE_POLICY_NO',  0);
define ('RADIUS_RAHUNAS_FAIR_USAGE_POLICY_YES', 1);

$vendors[9999] = array (
  1 => array ("AttributeName" => "RahuNAS-Volume-Quota",
              "AttributeType" => "int"),
  2 => array ("AttributeName" => "RahuNAS-Duration-Quota",
              "AttributeType" => "int"),
  3 => array ("AttributeName" => "RahuNAS-Fair-Usage-Policy",
              "AttributeType" => "int")
);

$vendors[14122] = array ( 
  1 => array ("AttributeName" => "WISPr-Location-ID", 
              "AttributeType" => "string"),
  2 => array ("AttributeName" => "WISPr-Location-Name",
              "AttributeType" => "string"),
  3 => array ("AttributeName" => "WISPr-Logoff-URL",
              "AttributeType" => "string"),
  4 => array ("AttributeName" => "WISPr-Redirection-URL",
              "AttributeType" => "string"),
  5 => array ("AttributeName" => "WISPr-Bandwidth-Min-Up",
              "AttributeType" => "int"),
  6 => array ("AttributeName" => "WISPr-Bandwidth-Min-Down",
              "AttributeType" => "int"),
  7 => array ("AttributeName" => "WISPr-Bandwidth-Max-Up",
              "AttributeType" => "int"),
  8 => array ("AttributeName" => "WISPr-Bandwidth-Max-Down", 
              "AttributeType" => "int"), 
  9 => array ("AttributeName" => "WISPr-Session-Terminate-Time",
              "AttributeType" => "string"),
 10 => array ("AttributeName" => "WISPr-Session-Terminate-End-Of-Day",
              "AttributeType" => "string"),
 11 => array ("AttributeName" => "WISPr-Billing-Class-Of-Service",
              "AttributeType" => "string")
);
?>
