
DROP TABLE IF EXISTS `c_profile`;
DROP TABLE IF EXISTS `c_element`;
DROP TABLE IF EXISTS `c_service`;

CREATE TABLE `c_service` (
  `cs_uid` TEXT PRIMARY KEY NOT NULL,
  `cs_enabled` INTEGER DEFAULT 0,
  `cs_name` TEXT NOT NULL UNIQUE,
  `cs_description` TEXT
);

CREATE TABLE `c_element` (
  `cs_uid` TEXT,
  `ce_name` TEXT,
  `ce_tag` TEXT
);

CREATE TABLE `c_profile` (
  `cp_id` INTEGER PRIMARY KEY AUTOINCREMENT,
  `cp_name` TEXT NOT NULL UNIQUE,
  `cp_data` TEXT
);
