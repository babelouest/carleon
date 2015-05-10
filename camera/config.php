<?php

require_once '../config/config.php';

$storeFilePath = "$carelonStorePath/camera.json";

$configCameras = json_decode(file_get_contents($storeFilePath), true);
