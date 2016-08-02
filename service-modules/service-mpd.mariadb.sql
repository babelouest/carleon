DROP TABLE IF EXISTS `c_service_mpd`;

CREATE TABLE `c_service_mpd` (
  `cmpd_name` varchar(64) NOT NULL UNIQUE,
  `cmpd_description` varchar(128),
  `cmpd_host` varchar(128) NOT NULL,
  `cmpd_port` INT(5),
  `cmpd_password` varchar(128)
);
