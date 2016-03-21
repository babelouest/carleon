-- Create database and user
-- CREATE DATABASE `carleon_dev`;
-- GRANT ALL PRIVILEGES ON carleon_dev.* TO 'carleon'@'%' identified BY 'carleon';
-- FLUSH PRIVILEGES;
-- USE `carleon_dev`;

DROP TABLE IF EXISTS `c_profile`;
DROP TABLE IF EXISTS `c_element`;
DROP TABLE IF EXISTS `c_service`;

CREATE TABLE `c_service` (
  `cs_uid` varchar(64) PRIMARY KEY NOT NULL,
  `cs_enabled` tinyint(1) DEFAULT 0,
  `cs_name` varchar(64) NOT NULL UNIQUE,
  `cs_description` varchar(512)
);

CREATE TABLE `c_element` (
  `cs_uid` varchar(64),
  `ce_name` varchar(64),
  `ce_tag` blob,
  CONSTRAINT `service_ibfk_1` FOREIGN KEY (`cs_uid`) REFERENCES `c_service` (`cs_uid`)
);

CREATE TABLE `c_profile` (
  `cp_id` int(11) PRIMARY KEY AUTO_INCREMENT,
  `cp_name` varchar(64) NOT NULL UNIQUE,
  `cp_data` BLOB -- profile data, json in a blob for old mysql versions compatibility
);
