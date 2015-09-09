<?php

function updateConfig($config, $configList, $storeFilePath) {
        for ($i=0; $i < count($configList); $i++) {
                if ($configList[$i]['name'] == $config['name']) {
                        $configList[$i] = $config;
      
                        return (saveJsonConfigFile($storeFilePath, $configList));
                }
        }
}

// Save the json config into the specified file
// Prevent from simultaneous write with a lock file
function saveJsonConfigFile($storeFilePath, $config) {
  $lockFile = sys_get_temp_dir() . '/carleon.lock';
  $fpLock = fopen($lockFile, "w");
  if (!$fpLock) {
    return false;
  }
  
  for ($i=0; $i < 10 && !flock($fpLock, LOCK_EX); $i++) {
    usleep(50);
  }
  if ($i < 10) {
    // Lock succeeded save file, unlock, then return
    $rt = file_put_contents($storeFilePath, json_encode($config, JSON_PRETTY_PRINT));
    flock($fpLock, LOCK_UN);
    if (!$rt) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
}

