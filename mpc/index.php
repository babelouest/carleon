<?php
/**
 * Carleon
 * Backend for profile management, MPD devices, cameras and webradios
 * Copyright 2015 Nicolas Mora mail@babelouest.org
 * Licenced under AGPL
 */

require("./re-mpd.class.php");
require("./config.php");

if ($carleonApps['mpc']['enabled']) {
	$output = array();
	if(isset($_GET["server"])) {
		$server = getServer($configMpc, $_GET["server"]);
		if ($server == null) {
			$output = array("result" => "error", "message" => "Server not found");
		} else {
			if (isset($_GET['addtag'])) {
				$tag = $_GET['addtag'];
				$server['tags'][] = $tag;
				updateMpc($server, $configMpc, $storeFilePath);
        $myMpd = new Mpd($server["host"], $server["port"], $server["password"] == "" ? null : $server["password"]);
        $output = getInfos($myMpd, $server);
			} else if (isset($_GET['removetag'])) {
				$tag = $_GET['removetag'];
				if (($key = array_search($tag, $server['tags'])) !== false) {
					array_splice($server['tags'], $key, 1);
					updateMpc($server, $configMpc, $storeFilePath);
					$myMpd = new Mpd($server["host"], $server["port"], $server["password"] == "" ? null : $server["password"]);
					$output = getInfos($myMpd, $server);
				} else {
					$output = array("result" => "error", "message" => "Tag not found");
				}
			} else if (isset($_GET["status"])) {
				if ($server == null) {
					$output = array("result" => "error", "message" => "Server not found");
				} else {
					$myMpd = new Mpd($server["host"], $server["port"], $server["password"] == "" ? null : $server["password"]);
					$output = getInfos($myMpd, $server);
				}
			} elseif (isset($_GET["stop"])) {
				if ($server == null) {
					$output = array("result" => "error", "message" => "Server not found");
				} else {
					$myMpd = new Mpd($server["host"], $server["port"], $server["password"] == "" ? null : $server["password"]);
					if (!$myMpd->connected) {
						$output = array("result" => "error", "message" => "Could not connect to server");
					} else {
						$result = $myMpd->Stop();
						$output = getInfos($myMpd, $server);
					}
				}
			} elseif (isset($_GET["play"])) {
				if ($server == null) {
					$output = array("result" => "error", "message" => "Server not found");
				} else {
					$myMpd = new Mpd($server["host"], $server["port"], $server["password"] == "" ? null : $server["password"]);
					if (!$myMpd->connected) {
						$output = array("result" => "error", "message" => "Could not connect to server");
					} else {
						$result = $myMpd->Play();
						$output = getInfos($myMpd, $server);
					}
				}
			} elseif (isset($_GET["volume"])) {
				if ($server == null) {
					$output = array("result" => "error", "message" => "Server not found");
				} elseif (!is_numeric($_GET["volume"])) {
					$output = array("result" => "error", "message" => "volume is not a numeric value");
				} else {
					$myMpd = new Mpd($server["host"], $server["port"], $server["password"] == "" ? null : $server["password"]);
					if (!$myMpd->connected) {
						$output = array("result" => "error", "message" => "Could not connect to server");
					} else {
						$result = $myMpd->SetVolume(intval($_GET["volume"]));
						$output = getInfos($myMpd, $server);
					}
				}
			}
		}
	} else {
		$servers = array();
		foreach ($configMpc as $oneServer) {
			$servers[] = array("name" => $oneServer["name"], "display" => $oneServer["display"], "tags" => $oneServer["tags"]);
		}
		$output = array('result' => 'ok', 'mpcs' => $servers);
	}
	echo json_encode($output);
} else {
	print json_encode(array('result' => 'error', 'message' => 'application disabled'));
}

function getServer($servers, $name) {
    foreach ($servers as $oneServer) {
        if ($oneServer["name"] == $name) {
            return $oneServer;
        }
    }
    return null;
}

function getInfos($myMpd, $server) {
	if (!$myMpd->connected) {
		return array("result" => "error", "message" => "Could not connect to server");
	} elseif (($myMpd->state == MPD_STATE_PLAYING) || ($myMpd->state == MPD_STATE_PAUSED)) {
		$track = $myMpd->playlist[$myMpd->currentTrackID];
		
		$state = $myMpd->state == MPD_STATE_PLAYING?"playing":"paused";
		$title = isset($track['Title']) ? $track['Title'] : "";
		$artist = isset($track['Artist']) ? $track['Artist'] : "";
		$album = isset($track['Album']) ? $track['Album'] : "";
		$date = isset($track['Date']) ? $track['Date'] : "";
			
	} else {
		$state = "stopped";
		$title = "";
		$artist = "";
		$album = "";
		$date = "";
	}
	$output = array("result" => "ok", 
						"mpc" => array(
									"name" => $server['name'],
									"state" => $state,
									"title" => $title,
									"artist" => $artist,
									"album" => $album,
									"date" => $date,
									"volume" => $myMpd->volume,
									"tags" => $server['tags']
									)
						);
	return $output;
}

function updateMpc($mpc, $configMpc, $storeFilePath) {
	for ($i=0; $i < count($configMpc); $i++) {
		if ($configMpc[$i]['name'] == $mpc['name']) {
			$configMpc[$i] = $mpc;
      return (saveJsonConfigFile($storeFilePath, $configMpc));
		}
	}
}
