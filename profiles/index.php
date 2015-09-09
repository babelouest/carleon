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

/* GET method, list profiles or groups */
/* POST method, add, set or remove a profile */

if ($carleonApps['profiles']['enabled']) {
	$jsonProfiles = json_decode(file_get_contents($storeFilePath), true);

	if ($_SERVER['REQUEST_METHOD'] == 'GET') {
		if (isset($_GET['profiles'])) {
			// return profiles list
			$profiles = array();
			foreach ($jsonProfiles as $pr) {
				$profiles[] = array('id' => $pr['id'], 'display' => $pr['display']);
			}
			print json_encode(array('result' => 'ok', 'profiles' => $profiles));
		} else if (isset($_GET['setprofile'])) {
			// change cookie to store the selected profile
			setcookie($cookieName, $_GET['setprofile'], time()+$cookieExpires);
			printProfile($_GET['setprofile'], $jsonProfiles, $storeFilePath);
		} else if (isset($_GET['profile'])) {
			// return group lists for the selected profile
			$found = false;
			printProfile($_GET['profile'], $jsonProfiles, $storeFilePath);
		} else {
			// return group list for the current profile according to the cookie
			if (isset($_COOKIE[$cookieName])) {
				// return expected profile
				$curPr = $_COOKIE[$cookieName];
				$found = false;
				printProfile($curPr, $jsonProfiles, $storeFilePath);
			} else {
				// return default profile
				printDefaultProfile($jsonProfiles, $storeFilePath);
			}
		}
	} else if ($_SERVER['REQUEST_METHOD'] == 'POST') {
		if (isset($_POST['action']) && isset($_POST['profile'])) {
			if ($_POST['action'] == 'addprofile') {
				$newId = getLastId($jsonProfiles)+1;
				$jsonProfiles[] = array('id' => (string)$newId, 'display' => $_POST['profile'], 'tabs' => array(), 'options' => new stdClass());
				if (saveJsonConfigFile($storeFilePath, $jsonProfiles)) {
					printProfile($newId, $jsonProfiles, $storeFilePath);
				} else {
					print json_encode(array('result' => 'error', 'reason' => 'Error writing file'));
				}
			} else if ($_POST['action'] == 'modifyprofile') {
				if (isset($_POST['profile'])) {
					$profile = json_decode($_POST['profile'], true);
					if (isset($profile['id'])) {
						for ($i=0; $i<count($jsonProfiles); $i++) {
							if ($jsonProfiles[$i]['id'] == $profile['id']) {
								if (isset($profile['default']) && $profile['default']) {
									$newJsonProfiles = setDefaultProfile($profile, $jsonProfiles);
									if ($newJsonProfiles) {
										if (saveJsonConfigFile($storeFilePath, $newJsonProfiles)) {
											printProfile($profile['id'], $newJsonProfiles, $storeFilePath);
										} else {
											print json_encode(array('result' => 'error', 'reason' => 'Error writing file'));
										}
									} else {
										print json_encode(array('result' => 'error', 'reason' => 'Error setting new default profile'));
									}
								} else {
									$jsonProfiles[$i] = $profile;
									if (saveJsonConfigFile($storeFilePath, $jsonProfiles)) {
										printProfile($profile['id'], $jsonProfiles, $storeFilePath);
									} else {
										print json_encode(array('result' => 'error', 'reason' => 'Error writing file'));
									}
								}
							}
						}
					} else {
						print json_encode(array('result' => 'error', 'reason' => 'Profile not found'));
					}
				} else {
					print json_encode(array('result' => 'error', 'reason' => 'No profile specified'));
				}
			} else if ($_POST['action'] == 'removeprofile') {
				if (isset($_POST['profile'])) {
					$found = false;
					for ($i=0; $i<count($jsonProfiles); $i++) {
						if ($jsonProfiles[$i]['id'] == $_POST['profile']) {
							if (isset($jsonProfiles[$i]['default']) && $jsonProfiles[$i]['default'] == 1) {
								print json_encode(array('result' => 'error', 'reason' => 'Can not remove default profile'));
							} else {
								array_splice($jsonProfiles, $i, 1);
								if (saveJsonConfigFile($storeFilePath, $jsonProfiles)) {
									print json_encode(array('result' => 'ok'));
								} else {
									print json_encode(array('result' => 'error', 'reason' => 'Error writing file'));
								}
							}
						}
					}
				} else {
					print json_encode(array('result' => 'error', 'reason' => 'No profile specified'));
				}
			}
		} else {
			print json_encode(array('result' => 'error', 'reason' => 'No action or profile specified'));
		}
	} else {
		print json_encode(array('result' => 'error', 'reason' => 'HTTP method error'));
	}
} else {
	print json_encode(array('result' => 'error', 'message' => 'application disabled'));
}

function printProfile($profile, $jsonProfiles, $storeFilePath) {
  $pr = getProfile($profile, $jsonProfiles);
  if ($pr) {
    $default = (isset($pr['default']))?$pr['default']:0;
    print json_encode(array('result' => 'ok', 'profile' => array('id' => $pr['id'], 'display' => $pr['display'], 'default' => $default, 'tabs' => $pr['tabs'], 'options' => ($pr['options']==null?new stdClass():$pr['options']))));
  } else {
    printDefaultProfile($jsonProfiles, $storeFilePath);
  }
}

function printDefaultProfile($jsonProfiles, $storeFilePath) {
  if (isset($jsonProfiles)) {
    foreach ($jsonProfiles as $pr) {
      if ($pr['default']) {
        print json_encode(array('result' => 'ok', 'profile' => array('id' => $pr['id'], 'display' => $pr['display'], 'default' => $pr['default'], 'tabs' => $pr['tabs'], 'options' => ($pr['options']==null?new stdClass():$pr['options']))));
        return;
      }
    }
  }
  
  // If no default profile is found, create one
  $defaultProfile = array('id' => (string)getLastId($jsonProfiles)+1, 'display' => 'Default profile', 'default' => 1, 'tabs' => array(), 'options' => new stdClass());
  $jsonProfiles[] = $defaultProfile;
  if (saveJsonConfigFile($storeFilePath, $jsonProfiles)) {
    print json_encode(array('result' => 'ok', 'profile' => $defaultProfile));
  } else {
    print json_encode(array('result' => 'error', 'reason' => 'Error writing file'));
  }
}

function getProfile($profile, $jsonProfiles) {
	foreach ($jsonProfiles as $pr) {
		if ($pr['id'] == $profile) {
      return $pr;
		}
	}
  return false;
}

function getLastId($jsonProfiles) {
  $id = 0;
  if (isset($jsonProfiles)) {
    foreach ($jsonProfiles as $pr) {
      if (intval($pr['id']) > $id) {
        $id = intval($pr['id']);
      }
    }
  }
  return $id;
}

function setDefaultProfile($profile, $jsonProfiles) {
  // Look for new default profile and set all other profiles default flag to false
  $found = false;
  for ($i=0; $i<count($jsonProfiles); $i++) {
    $jsonProfiles[$i]['default'] = 0;
    if ($jsonProfiles[$i]['id'] == $profile['id']) {
      $found = true;
      $jsonProfiles[$i] = $profile;
    }
  }
  
  // If new default is found, set default flag to true
  if ($found) {
    return $jsonProfiles;
  } else {
    return false;
  }
}
