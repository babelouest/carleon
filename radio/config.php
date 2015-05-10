<?php
/**
 * Carleon
 * Backend for profile management, MPD devices, cameras and webradios
 * Copyright 2015 Nicolas Mora mail@babelouest.org
 * Licenced under AGPL
 */

require_once '../config/config.php';

$fields = array("title", "artist", "albumartist", "year", "album");
$commands = array("skip", "stop", "start", "status", "remaining");

$storeFilePath = "$carelonStorePath/radio.json";

$configRadio = json_decode(file_get_contents($storeFilePath), true);

?>
