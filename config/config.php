<?php

$carelonStorePath = '/media/donjon/scripts/angharad/carleon/data';

$carleonUrl = '/carleon/';

$carleonApps = array();

$carleonApps['profiles'] = array('name' => 'profiles',
								'enabled' => true,
								'url' => '/profiles/',
);

$carleonApps['camera'] = array('name' => 'camera',
								'enabled' => true,
								'url' => '/camera/',
);

$carleonApps['mpc'] = array('name' => 'mpc',
								'enabled' => true,
								'url' => '/mpc/',
);

$carleonApps['radio'] = array('name' => 'radio',
								'enabled' => true,
								'url' => '/radio/',
);

function updateConfig($config, $configList, $storeFilePath) {
	for ($i=0; $i < count($configList); $i++) {
		if ($configList[$i]['name'] == $config['name']) {
			$configList[$i] = $config;
      return (file_put_contents($storeFilePath, json_encode($configList, JSON_PRETTY_PRINT)));
		}
	}
}
