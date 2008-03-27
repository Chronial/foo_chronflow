<?php

$file = file_get_contents("_defaultConfigs/_newDefault.js");
$file = str_replace("\r\n",'\r\n',addslashes($file));

$file = '#define COVER_CONFIG_DEF_CONTENT "'.$file.'"';

file_put_contents("COVER_CONFIG_DEF_CONTENT.h",$file);


$configs = 'static const char* def_cfg_coverConfigs[] = {'."\\\r\n";

$d = dir("_defaultConfigs");
while ($f = $d->read()){
   if ($f[0] == '.' || $f[0] == "_")
      continue;
   $file = file_get_contents("_defaultConfigs/".$f);
   $content = str_replace("\r\n",'\r\n',addslashes($file));
   $title = str_replace(".js","",$f);
   $configs .= '"'.$title.'", "'.$content.'",  '."\\\r\n";
}

$configs .= '""};';

file_put_contents("DEF_CFG_COVERCONFIGS.h",$configs);


?>