<?php
/**
 * Carleon
 * Backend for profile management, MPD devices, cameras and webradios
 * Copyright 2015 Nicolas Mora mail@babelouest.org
 * Licenced under AGPL
 */
require_once 'config/config.php';

print json_encode(array('result' => 'ok', 'apps' => $carleonApps));
