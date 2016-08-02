DROP TABLE IF EXISTS `c_service_mpd`;

CREATE TABLE `c_service_mpd` (
  `cmpd_name` TEXT PRIMARY KEY NOT NULL,
  `cmpd_description` TEXT,
  `cmpd_host` TEXT NOT NULL,
  `cmpd_port` INTEGER,
  `cmpd_password` TEXT
);
