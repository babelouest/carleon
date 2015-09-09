<?php
/**
 * Carleon
 * Backend for profile management, MPD devices, cameras and webradios
 * Copyright 2015 Nicolas Mora mail@babelouest.org
 * Licenced under AGPL
 */
require_once '../lib/carleon.php';
require_once '../lib/config.php';
require_once 'config.php';

if ($carleonApps['camera']['enabled']) {
	$camera = null;
	if (isset($_GET['camera'])) {
		foreach ($configCameras as $oneCamera) {
			if ($oneCamera['name'] == $_GET['camera']) {
				$camera = $oneCamera;
			}
		}
	} else {
		$name = "";
	}

	if ($camera != null) {
		$url = $camera['stream-url'];
		header('Location: '.$url);
	} else {
		header('Location: no_image.jpg');
	}
} else {
	print json_encode(array('result' => 'error', 'message' => 'application disabled'));
}
