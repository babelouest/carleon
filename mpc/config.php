<?php
/**
 * Carleon
 * Backend for profile management, MPD devices, cameras and webradios
 * Copyright 2015 Nicolas Mora mail@babelouest.org
 * Licenced under AGPL
 */
require_once '../config/config.php';

$storeFilePath = "$carelonStorePath/mpc.json";

$configMpc = json_decode(file_get_contents($storeFilePath), true);
