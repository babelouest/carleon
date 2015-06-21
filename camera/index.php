<?php
/**
 * Carleon
 * Backend for profile management, MPD devices, cameras and webradios
 * Copyright 2015 Nicolas Mora mail@babelouest.org
 * Licenced under AGPL
 */
require_once 'config.php';

if ($carleonApps['camera']['enabled']) {
	if (isset($_GET['camera'])) {
		$camera = null;
		foreach ($configCameras as $oneCamera) {
			if ($oneCamera['name'] == $_GET['camera']) {
				$camera = $oneCamera;
			}
		}
		if ($camera != null) {
			if (isset($_GET['addtag'])) {
				$tag = $_GET['addtag'];
				$camera['tags'][] = $tag;
				updateCamera($camera, $configCameras, $storeFilePath);
			} else if (isset($_GET['removetag'])) {
				$tag = $_GET['removetag'];
				if (($key = array_search($tag, $camera['tags'])) !== false) {
					array_splice($camera['tags'], $key, 1);
					updateCamera($camera, $configCameras, $storeFilePath);
				} else {
					print json_encode(array('result' => 'error', 'reason' => 'Tag not found'));
				}
			} else {
				$photoDir = $camera['scheduled-path'].'/';
				if (isset($_GET['alert'])) {
					$photoDir = $camera['triggered-path'].'/';
					$filesFilter = $camera['triggered-files-filter'];
				} else {
					$filesFilter = $camera['scheduled-files-filter'];
				}
				$fileList = array_reverse(glob($photoDir.$filesFilter));
				$output = array();
				foreach ($fileList as $oneFile) {
					$output[] = end(split('/', $oneFile));
				}
				$curCamera = array();
				$curCamera['name'] = $camera['name'];
				$curCamera['detection'] = isDetectionActive($camera);
				$curCamera['list'] = $output;
				print json_encode(array('result' => 'ok', 'camera' => $curCamera));
			}
		} else {
			print json_encode(array('result' => 'error', 'message' => 'camera not found'));
		}
	} else {
		$cameras = array();
		foreach ($configCameras as $oneCamera) {
			$cameras[] = array('name' => $oneCamera['name'], 'description' => $oneCamera['description'], 'tags' => $oneCamera['tags']);
		}
		print(json_encode(array('result' => 'ok', 'cameras' => $cameras)));
	}
} else {
	print json_encode(array('result' => 'error', 'message' => 'application disabled'));
}

function updateCamera($camera, $configCameras, $storeFilePath) {
	for ($i=0; $i < count($configCameras); $i++) {
		if ($configCameras[$i]['name'] == $camera['name']) {
			$configCameras[$i] = $camera;
      if (file_put_contents($storeFilePath, json_encode($configCameras, JSON_PRETTY_PRINT))) {
        print json_encode(array('result' => 'ok', 'camera' => $camera));
      } else {
        print json_encode(array('result' => 'error', 'reason' => 'Error writing file'));
      }
		}
	}
}

// Return true if the camera has activated detection, false otherwise
function isDetectionActive($camera) {
	$statusPage = @file_get_contents($camera['detection-status']['url']);
	return (strstr($statusPage, $camera['detection-status']['active-string']) != false);
}
