<?php
/**
 * Carleon
 * Backend for profile management, MPD devices, cameras and webradios
 * Copyright 2015 Nicolas Mora mail@babelouest.org
 * Licenced under AGPL
 */
require_once '../config/config.php';

$storeFilePath = "$carelonStorePath/profiles.json";
$cookieName = "carleondemo_profile";
$cookieExpires = 60*60*24*10*365; // 10 years
