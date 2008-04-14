<?php

$file = file_get_contents("_defaultConfigs/_newDefault.js");
$file = str_replace("\r\n",'\r\n',addslashes($file));

$file = '#define COVER_CONFIG_DEF_CONTENT "'.$file.'"';

file_put_contents("COVER_CONFIG_DEF_CONTENT.h",$file);


$configs = 'static const char* buildInCoverConfigs[] = {'."\\\r\n";

$count = 0;
$d = dir("_defaultConfigs");
while ($f = $d->read()){
   if ($f[0] == '.' || $f[0] == "_")
      continue;
   $file = file_get_contents("_defaultConfigs/".$f);
   $i = 0;
   while (ord($file[$i]) > 128) {
      $i++;
   }
   $content = substr($file, $i);
   $content = str_replace("\r\n",'\r\n',addslashes($content));
   $content = str_replace("   ","\t",$content);
   $title = str_replace(".js","",$f).' (build-in)';
   $configs .= '"'.$title.'", "'.$content.'",  '."\\\r\n";
   $count++;
}

$configs .= '""};'."\r\n";

$configs .= 'static const int buildInCoverConfigCount = '.$count.";\r\n";

file_put_contents("BUILD_IN_COVERCONFIGS.h",$configs);


?>