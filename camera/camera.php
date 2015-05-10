<?php
/**
 * Carleon
 * Backend for profile management, MPD devices, cameras and webradios
 * Copyright 2015 Nicolas Mora mail@babelouest.org
 * Licenced under AGPL
*/
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

	header("Content-type: image/png");
	header("Cache-Control: no-cache, must-revalidate");
	if ($camera != null) {
		$photoDir = $camera['scheduled-path'];
		$curFile = "lastsnap.jpg";
		if (isset($_GET['file'])) {
			$curFile = $_GET['file'];
		}
		if (isset($_GET['alert'])) {
			$photoDir = $camera['triggered-path'];
		}
		if (file_exists("$photoDir/" . $curFile)) {
			$im = imagecreatefromjpeg("$photoDir/" . $curFile);
			if (!isset($_GET['large'])) {
				if (isset($_GET['grid'])) {
					$rsr_scl = imagecreatetruecolor(160, 120);
					imagecopyresized($rsr_scl, $im, 0, 0, 0, 0, 160, 120, imagesx($im), imagesy($im));
				} else {
					$rsr_scl = imagecreatetruecolor(640, 480);
					imagecopyresized($rsr_scl, $im, 0, 0, 0, 0, 640, 480, imagesx($im), imagesy($im));
				}
				imagejpeg($rsr_scl);
				imagedestroy($rsr_scl);
			} else {
				imagejpeg($im);
			}
		} else {
			$im = imagecreatefromjpeg("no_image.jpg");
			imagejpeg($im);
		}
		imagedestroy($im);
	} else {
		$im = imagecreatefromjpeg("no_image.jpg");
		imagejpeg($im);
		imagedestroy($im);
	}
} else {
	print json_encode(array('result' => 'error', 'message' => 'application disabled'));
}
