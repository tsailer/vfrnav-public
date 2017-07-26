<?php

if (!isset($_SERVER['PHP_AUTH_USER'])) {
  header('WWW-Authenticate: Basic realm="Image Compressor"');
  header('HTTP/1.0 401 Unauthorized');
  echo '<p>Sorry, login required.</p>';
  exit;
} else if ($_SERVER['PHP_AUTH_USER'] != "user" || $_SERVER['PHP_AUTH_PW'] != "passwd") {
  header('WWW-Authenticate: Basic realm="Image Compressor"');
  header('HTTP/1.0 401 Unauthorized');
  echo '<p>Unauthorized.</p>';
  exit;
}

$source_url = 'http://user:passwd@www.flugwetter.de/scripts/getimg.php?src=' . $_GET["src"];

$info = getimagesize($source_url);

if ($info) {
  if ($info['mime'] == 'image/jpeg') $image = imagecreatefromjpeg($source_url);
  elseif ($info['mime'] == 'image/gif') $image = imagecreatefromgif($source_url);
  elseif ($info['mime'] == 'image/png') $image = imagecreatefrompng($source_url);

  header('Content-type: image/jpeg');

  // transmit file
  imagejpeg($image);
  exit;
}

echo '<p>Image ' . $_GET["src"] . ' not found</p>';

?>
