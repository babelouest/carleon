<?php
/**
 * Carleon
 * Backend for profile management, MPD devices, cameras and webradios
 * Copyright 2015 Nicolas Mora mail@babelouest.org
 * Licenced under AGPL
 */
require_once './lib/carleon.php';
require_once './lib/config.php';

print json_encode(array('result' => 'ok', 'apps' => $carleonApps));
