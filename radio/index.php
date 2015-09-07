<?php
/**
 * Carleon
 * Backend for profile management, MPD devices, cameras and webradios
 * Copyright 2015 Nicolas Mora mail@babelouest.org
 * Licenced under AGPL
 */

require_once 'config.php';

if ($carleonApps['radio']['enabled']) {
	// Actions: list, on_air, request
	if (isset($_GET['radio'])) {
		$radio = null;
		for ($i=0; $i < count($configRadio); $i++) {
			if ($_GET['radio'] == $configRadio[$i]['name']) {
				$radio = $configRadio[$i];
			}
		}
		if ($radio != null) {
			if (isset($_GET['addtag'])) {
				$tag = $_GET['addtag'];
				$radio['tags'][] = $tag;
				if (updateConfig($radio, $configRadio, $storeFilePath)) {
					echo json_encode(array('result' => 'ok', 'radio' => radioDescription($radio)));
				} else {
					echo json_encode(array("result" => "error", "message" => "Error while saving radio"));
				}
			} else if (isset($_GET['removetag'])) {
				$tag = $_GET['removetag'];
				if (($key = array_search($tag, $radio['tags'])) !== false) {
					array_splice($radio['tags'], $key, 1);
					if (updateConfig($radio, $configRadio, $storeFilePath)) {
						echo json_encode(array('result' => 'ok', 'radio' => radioDescription($radio)));
					} else {
						echo json_encode(array("result" => "error", "message" => "Error while saving radio"));
					}
				} else {
					echo json_encode(array("result" => "error", "message" => "Tag not found"));
				}
			} elseif ($radio['control'] == true) {
				$socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
				
				if ($socket === false) {
					echo json_encode(array('result' => 'error', 'message' => "socket_create() failed :  " . socket_strerror(socket_last_error()) . "\n"));
				} else {
					$stream					= $radio['url'];
					$name					= $radio['name'];
					$url					= $radio['url'];
					$type					= $radio['type'];
					$host					= $radio['host'];
					$port					= $radio['port'];
					$requestName	= $radio['requestName'];
					
					$result = socket_connect($socket, $host, $port);
					if ($socket === false) {
						echo json_encode(array('result' => 'error', 'message' => "socket_connect() failed : ($result) " . socket_strerror(socket_last_error($socket)) . "\n"));
					} else {
						if (isset($_GET['list'])) {
							$request = "$requestName.metadata\nquit\n";
							$response = array();

							socket_write($socket, $request, strlen($request));
					
							while ($response[] = socket_read($socket, 2048));

							$lines = explode("\n", implode('', $response));
						
							$songs = array();
							$curSong = null;
						
							foreach ($lines as $line) {
								//echo $line." - ".strpos($line, '--- ')."<br>";
								if ((strpos($line, '--- ') === 0) && (strpos($line, ' ---')+strlen(' ---') === strlen($line))) {
									// New song
									if ($curSong != null) {
										$songs[] = $curSong;
									}
									$curSong = array();
									//echo count($songs)."<br>";
								} else if ($line !== 'END' && $line != 'Bye!') {
									$curElt = explode('=', $line);
									if (count($curElt) > 1 && in_array($curElt[0], $fields)) {
										$curSong[$curElt[0]] = trim($curElt[1], " \t\n\r\0\x0B\"");
									}
								}
							}
							if ($curSong != null) {
								$songs[] = $curSong;
							}
							echo json_encode(array('result' => 'ok', 'list' => array_reverse($songs)));
						} elseif (isset($_GET['on_air'])) {
							$request = "request.on_air\n";

							socket_write($socket, $request, strlen($request));

							$onAir = socket_read($socket, 2048, PHP_NORMAL_READ);

							$request = "request.metadata $onAir\nquit\n";

							socket_write($socket, $request, strlen($request));

							$response = array();

							while ($response[] = socket_read($socket, 2048));

							$lines = explode("\n", implode('', $response));
							
							$curSong = null;
							$fallback = array('title' => 'Error getting values', 'artist' => 'Error getting values', 'album' => 'Error getting values', 'year' => 'Error getting values', 'albumartist' => 'Error getting values');
							
							foreach ($lines as $line) {
								if ($line !== 'END' && $line != 'Bye!') {
									$curElt = explode('=', $line);
									if (count($curElt) > 1 && in_array($curElt[0], $fields)) {
										$curSong[$curElt[0]] = trim($curElt[1], " \t\n\r\0\x0B\"");
									}
								}
							}
							echo json_encode(array('result' => 'ok', 'song' => $curSong===null?$fallback:$curSong));
						} elseif (isset($_GET['request'])) {
							if (in_array($_GET['request'], $commands)) {
								$request = $requestName.".".$_GET['request']."\nquit\n";

								socket_write($socket, $request, strlen($request));

								$response = socket_read($socket, 2048);
								echo json_encode(array('result' => 'ok', 'command' => $_GET['request'], 'data' => $response));
							} else {
								echo json_encode(array('result' => 'error', 'message' => 'unknown command', 'command' => $_GET['request']));
							}
						} else {
							echo json_encode(array('result' => 'error', 'message' => 'unknown action', 'action' => $_GET['action']));
						}
						socket_close($socket);
					}
				}
			} else {
				echo json_encode(array('result' => 'error', 'message' => 'This radio doesn\'t allow controls'));
			}
		} else {
			echo json_encode(array('result' => 'error', 'message' => 'Radio not found'));
		}
	} else {
		$output = array();
		foreach ($configRadio as $radio) {
			$output[] = radioDescription($radio);
		}
		echo json_encode(array('result' => 'ok', 'radios' => $output));
	}
} else {
	print json_encode(array('result' => 'error', 'message' => 'application disabled'));
}

function radioDescription($radio) {
	return array('name' => $radio['name'],
									'display' => $radio['description'],
									'url' => $radio['url'],
									'type' => $radio['type'],
									'control' => $radio['control'],
									'tags' => $radio['tags']
	);
}
?>
