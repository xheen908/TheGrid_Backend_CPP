-- ########################################################
-- BATCH INITIALIZATION SCRIPT (REFACTORED)
-- Project: The Grid - Multi-Service Architecture
-- Dbs: auth_db, charakter_db, world_db
-- ########################################################

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- --------------------------------------------------------
-- 1. SETUP AUTH_DB (Accounts & Permissions)
-- --------------------------------------------------------
CREATE DATABASE IF NOT EXISTS `auth_db`;
USE `auth_db`;

CREATE TABLE IF NOT EXISTS `users` (
  `id` INT AUTO_INCREMENT PRIMARY KEY,
  `username` VARCHAR(50) NOT NULL UNIQUE,
  `password_hash` VARCHAR(255) NOT NULL,
  `gm_status` BOOLEAN DEFAULT FALSE,
  `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

TRUNCATE TABLE `users`;
INSERT INTO `users` (`id`, `username`, `password_hash`, `gm_status`) VALUES
(1, 'xheen908', 'password', true),
(2, 'admin', 'admin', true),
(3, 'client', 'client', false);

-- --------------------------------------------------------
-- 2. SETUP CHARAKTER_DB (Dynamic Player Data)
-- --------------------------------------------------------
CREATE DATABASE IF NOT EXISTS `charakter_db`;
USE `charakter_db`;

CREATE TABLE IF NOT EXISTS `characters` (
  `id` INT AUTO_INCREMENT PRIMARY KEY,
  `user_id` INT NOT NULL,
  `char_name` VARCHAR(100) NOT NULL UNIQUE,
  `map_name` VARCHAR(100) DEFAULT 'WorldMap0',
  `pos_x` FLOAT DEFAULT 0.0,
  `pos_y` FLOAT DEFAULT 0.0,
  `pos_z` FLOAT DEFAULT 0.0,
  `rotation` FLOAT DEFAULT 0.0,
  `level` INT DEFAULT 1,
  `xp` INT DEFAULT 0,
  `hp` INT DEFAULT 125,
  `max_hp` INT DEFAULT 125,
  `mana` INT DEFAULT 120,
  `max_mana` INT DEFAULT 120,
  `appearance_data` JSON,
  `last_login` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB;

TRUNCATE TABLE `characters`;
INSERT INTO `characters` (`id`, `user_id`, `char_name`, `map_name`, `pos_x`, `pos_y`, `pos_z`, `rotation`, `level`, `xp`, `hp`, `max_hp`, `mana`, `max_mana`, `appearance_data`) VALUES
(101, 3, 'Wheen', 'WorldMap0', -5.79, 0.0, 5.47, -1.63, 10, 5000, 350, 350, 300, 300, '{"suit_color": "#00FFFF"}'),
(102, 3, 'Fheen', 'WorldMap0', 21.76, 0.0, -2.78, 2.09, 6, 200, 250, 250, 220, 220, '{"suit_color": "#00FFFF"}'),
(103, 2, 'Yheen', 'WorldMap0', -5.07, 0.0, -0.30, 2.75, 1, 0, 125, 125, 120, 120, '{"suit_color": "#00FFFF"}'),
(104, 2, 'Xheen', 'Arena2', 3.44, 0.0, -4.10, 0.24, 60, 0, 1500, 1500, 1200, 1200, '{"suit_color": "#FF0000"}');

-- --------------------------------------------------------
-- 3. SETUP WORLD_DB (Static World Data)
-- --------------------------------------------------------
CREATE DATABASE IF NOT EXISTS `world_db`;
USE `world_db`;

CREATE TABLE IF NOT EXISTS `maps` (
  `id` INT AUTO_INCREMENT PRIMARY KEY,
  `map_name` VARCHAR(100) NOT NULL UNIQUE,
  `display_name` VARCHAR(100)
) ENGINE=InnoDB;

TRUNCATE TABLE `maps`;
INSERT INTO `maps` (`map_name`, `display_name`) VALUES
('WorldMap0', 'Hauptwelt'),
('Dungeon0', 'Finsterer Kerker'),
('Arena0', 'Sandige Arena'),
('Arena1', 'Eisige Arena'),
('Arena2', 'Vulkanische Arena');

-- --------------------------------------------------------
-- 4. SETUP MOBS (NPC Enemy Data)
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS `mobs` (
  `id` INT AUTO_INCREMENT PRIMARY KEY,
  `mob_id` VARCHAR(50) NOT NULL UNIQUE,
  `map_name` VARCHAR(100) NOT NULL,
  `name` VARCHAR(100) NOT NULL,
  `level` INT DEFAULT 1,
  `hp` INT DEFAULT 100,
  `pos_x` FLOAT DEFAULT 0.0,
  `pos_y` FLOAT DEFAULT 0.0,
  `pos_z` FLOAT DEFAULT 0.0,
  `respawn_time` INT DEFAULT 30
) ENGINE=InnoDB;

TRUNCATE TABLE `mobs`;
INSERT INTO `mobs` (`id`, `mob_id`, `map_name`, `name`, `level`, `hp`, `pos_x`, `pos_y`, `pos_z`, `respawn_time`) VALUES
	(1, 'arena_01', 'Arena0', 'Bit-Wächter', 1, 100, 0, 0, -10, 30),
	(2, 'arena_02', 'Arena0', 'Bit-Wächter', 1, 100, 20, 0, 0, 30),
	(3, 'arena_03', 'Arena0', 'Bit-Veteran', 3, 450, -5, 0, -20, 30),
	(4, 'arena_g2_1', 'Arena0', 'Duo-Bit A', 2, 200, -10, 0, -5, 30),
	(5, 'arena_g2_2', 'Arena0', 'Duo-Bit B', 2, 200, -11, 0, -6, 30),
	(6, 'mob_boss', 'Dungeon0', 'System-Kernel', 16, 15000, 3.75083, 3.75083, 0.433696, 30);

-- --------------------------------------------------------
-- 5. USER CREATION & PRIVILEGES
-- --------------------------------------------------------
CREATE USER IF NOT EXISTS 'user_name'@'%' IDENTIFIED WITH mysql_native_password BY 'user_passwort';
GRANT ALL PRIVILEGES ON `auth_db`.* TO 'user_name'@'%';
GRANT ALL PRIVILEGES ON `charakter_db`.* TO 'user_name'@'%';
GRANT ALL PRIVILEGES ON `world_db`.* TO 'user_name'@'%';
FLUSH PRIVILEGES;

SET FOREIGN_KEY_CHECKS = 1;