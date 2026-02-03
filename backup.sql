-- --------------------------------------------------------
-- Host:                         127.0.0.1
-- Server-Version:               8.0.44 - MySQL Community Server - GPL
-- Server-Betriebssystem:        Linux
-- HeidiSQL Version:             12.11.0.7065
-- --------------------------------------------------------

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET NAMES utf8 */;
/*!50503 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;


-- Exportiere Datenbank-Struktur für auth_db
CREATE DATABASE IF NOT EXISTS `auth_db` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci */ /*!80016 DEFAULT ENCRYPTION='N' */;
USE `auth_db`;

-- Exportiere Struktur von Tabelle auth_db.users
CREATE TABLE IF NOT EXISTS `users` (
  `id` int NOT NULL AUTO_INCREMENT,
  `username` varchar(50) NOT NULL,
  `password_hash` varchar(255) NOT NULL,
  `gm_status` tinyint(1) DEFAULT '0',
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `username` (`username`)
) ENGINE=InnoDB AUTO_INCREMENT=5 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- Exportiere Daten aus Tabelle auth_db.users: ~3 rows (ungefähr)
INSERT INTO `users` (`id`, `username`, `password_hash`, `gm_status`, `created_at`) VALUES
	(1, 'xheen908', 'password', 1, '2026-01-30 15:45:02'),
	(2, 'admin', 'admin', 1, '2026-01-30 15:45:02'),
	(3, 'client', 'client', 0, '2026-01-30 15:45:02');


-- Exportiere Datenbank-Struktur für charakter_db
CREATE DATABASE IF NOT EXISTS `charakter_db` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci */ /*!80016 DEFAULT ENCRYPTION='N' */;
USE `charakter_db`;

-- Exportiere Struktur von Tabelle charakter_db.characters
CREATE TABLE IF NOT EXISTS `characters` (
  `id` int NOT NULL AUTO_INCREMENT,
  `user_id` int NOT NULL,
  `char_name` varchar(100) NOT NULL,
  `character_class` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci DEFAULT 'Mage',
  `map_name` varchar(100) DEFAULT 'WorldMap0',
  `pos_x` float DEFAULT '0',
  `pos_y` float DEFAULT '0',
  `pos_z` float DEFAULT '0',
  `rotation` float DEFAULT '0',
  `level` int DEFAULT '1',
  `xp` int DEFAULT '0',
  `hp` int DEFAULT '125',
  `max_hp` int DEFAULT '125',
  `mana` int DEFAULT '120',
  `max_mana` int DEFAULT '120',
  `last_login` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `char_name` (`char_name`)
) ENGINE=InnoDB AUTO_INCREMENT=126 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- Exportiere Daten aus Tabelle charakter_db.characters: ~8 rows (ungefähr)
INSERT INTO `characters` (`id`, `user_id`, `char_name`, `character_class`, `map_name`, `pos_x`, `pos_y`, `pos_z`, `rotation`, `level`, `xp`, `hp`, `max_hp`, `mana`, `max_mana`, `last_login`) VALUES
	(103, 2, 'Yheen', 'Mage', 'Arena2', 10.5394, 2.50015, 32.8622, 2.75, 15, 4070, 651, 925, 400, 400, '2026-02-03 15:24:12'),
	(104, 2, 'Xheen', 'Mage', 'WorldMap0', -37.1, 0, -37, 0.24, 79, 5614, 14557, 14557, 1680, 1680, '2026-02-03 02:00:59'),
	(118, 3, 'Mage', 'Mage', 'WorldMap0', -0.138051, 0.000154, -7.23019, 0, 15, 4304, 0, 925, 400, 400, '2026-01-31 19:07:10'),
	(119, 3, 'Barbar', 'Barbarian', 'WorldMap0', 0, 0, 0, 0, 1, 0, 125, 125, 120, 120, '2026-01-31 17:04:24'),
	(121, 3, 'Ranger', 'Ranger', 'WorldMap0', 0, 0, 0, 0, 1, 0, 125, 125, 120, 120, '2026-01-31 17:04:53'),
	(122, 3, 'Knight', 'Knight', 'WorldMap0', 0, 0, 0, 0, 1, 0, 125, 125, 120, 120, '2026-01-31 17:05:06'),
	(123, 3, 'Rogue', 'Rogue', 'WorldMap0', -20.0564, 0, 8.42799, 0, 3, 295, 193, 193, 160, 160, '2026-01-31 17:21:29'),
	(124, 2, 'Theen', 'Knight', 'Arena2', 0.357657, 2.50015, 35.5458, 0, 8, 3176, 266, 428, 260, 260, '2026-02-03 15:18:37'),
	(125, 2, '123', 'Ranger', 'Dungeon0', -38.0083, 0, 0.036614, 0, 1, 165, 125, 125, 120, 120, '2026-02-03 15:27:59');


-- Exportiere Datenbank-Struktur für world_db
CREATE DATABASE IF NOT EXISTS `world_db` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci */ /*!80016 DEFAULT ENCRYPTION='N' */;
USE `world_db`;

-- Exportiere Struktur von Tabelle world_db.game_objects
CREATE TABLE IF NOT EXISTS `game_objects` (
  `id` int NOT NULL AUTO_INCREMENT,
  `map_name` varchar(100) NOT NULL,
  `type` varchar(50) NOT NULL,
  `pos_x` float DEFAULT '0',
  `pos_y` float DEFAULT '0',
  `pos_z` float DEFAULT '0',
  `rot_x` float DEFAULT '0',
  `rot_y` float DEFAULT '0',
  `rot_z` float DEFAULT '0',
  `extra_data` json DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=11 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- Exportiere Daten aus Tabelle world_db.game_objects: ~10 rows (ungefähr)
INSERT INTO `game_objects` (`id`, `map_name`, `type`, `pos_x`, `pos_y`, `pos_z`, `rot_x`, `rot_y`, `rot_z`, `extra_data`) VALUES
	(1, 'WorldMap0', 'portal', -40.9, 0, -41, 0, 0.7858, 0, '{"color": "#DB00DB", "spawn_pos": {"x": -42.5, "y": 0, "z": 0.05}, "target_map": "Dungeon0", "spawn_rot_y": -1.57}'),
	(2, 'WorldMap0', 'portal', 40.8, 0, -41.3, 0, 2.3558, 0, '{"color": "#B25405", "spawn_pos": {"x": 27.904615, "y": 0, "z": 0.036237}, "target_map": "Arena0", "spawn_rot_y": 1.593585}'),
	(3, 'WorldMap0', 'portal', 41.2, 0, 41.7, 0, 3.9208, 0, '{"color": "#00CCFF", "spawn_pos": {"x": 0.036129, "y": 0.000835, "z": -26.506031}, "target_map": "Arena1", "spawn_rot_y": -3.105295}'),
	(4, 'WorldMap0', 'portal', -41.7, 0, 41.4, 0, -0.7792, 0, '{"color": "#00FF00", "spawn_pos": {"x": -0.072281, "y": 2.500153, "z": 36.145241}, "target_map": "Arena2", "spawn_rot_y": 0.013375}'),
	(5, 'Arena0', 'portal', 35, 0, 0, 0, 1.57, 0, '{"color": "#FFFF00", "spawn_pos": {"x": 37.13, "y": 0, "z": -37.64}, "target_map": "WorldMap0", "spawn_rot_y": 2.35}'),
	(6, 'Arena1', 'portal', 0, 0, -32, 0, 0, 0, '{"color": "#FFFF00", "spawn_pos": {"x": 37.32, "y": 0, "z": 36.72}, "target_map": "WorldMap0", "spawn_rot_y": 0.75}'),
	(7, 'Arena2', 'portal', 0, 0, 45, 0, 0, 0, '{"color": "#FFFF00", "spawn_pos": {"x": -37.57, "y": 0, "z": 36.8}, "target_map": "WorldMap0", "spawn_rot_y": -0.796839}'),
	(8, 'Dungeon0', 'portal', -46.3, 0, 0, 0, 1.57, 0, '{"color": "#FFFF00", "spawn_pos": {"x": -37.1, "y": 0, "z": -37}, "target_map": "WorldMap0", "spawn_rot_y": -2.31}'),
	(9, 'TestMap0', 'portal', 5.06597, 6.43977, 30.7688, 0, -2.13045, 0, '{"color": "#072cfa", "spawn_pos": {"x": -6.52, "y": 2.37, "z": -1.15}, "target_map": "WorldMap0", "spawn_rot_y": 1.63}'),
	(10, 'WorldMap0', 'portal', 0.018641, 2.37008, -0.938741, 0, -1.57429, 0, '{"color": "#072cfa", "spawn_pos": {"x": 5.991358, "y": 6.377525, "z": 35.780994}, "target_map": "TestMap0", "spawn_rot_y": -2.91}');

-- Exportiere Struktur von Tabelle world_db.maps
CREATE TABLE IF NOT EXISTS `maps` (
  `id` int NOT NULL AUTO_INCREMENT,
  `map_name` varchar(100) NOT NULL,
  `display_name` varchar(100) DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `map_name` (`map_name`)
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- Exportiere Daten aus Tabelle world_db.maps: ~5 rows (ungefähr)
INSERT INTO `maps` (`id`, `map_name`, `display_name`) VALUES
	(1, 'WorldMap0', 'Hauptwelt'),
	(2, 'Dungeon0', 'Finsterer Kerker'),
	(3, 'Arena0', 'Sandige Arena'),
	(4, 'Arena1', 'Eisige Arena'),
	(5, 'Arena2', 'Vulkanische Arena');

-- Exportiere Struktur von Tabelle world_db.mobs
CREATE TABLE IF NOT EXISTS `mobs` (
  `id` int NOT NULL AUTO_INCREMENT,
  `mob_id` varchar(50) NOT NULL,
  `map_name` varchar(100) NOT NULL,
  `name` varchar(100) NOT NULL,
  `level` int DEFAULT '1',
  `hp` int DEFAULT '100',
  `pos_x` float DEFAULT '0',
  `pos_y` float DEFAULT '0',
  `pos_z` float DEFAULT '0',
  `respawn_time` int DEFAULT '30',
  PRIMARY KEY (`id`),
  UNIQUE KEY `mob_id` (`mob_id`)
) ENGINE=InnoDB AUTO_INCREMENT=18 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- Exportiere Daten aus Tabelle world_db.mobs: ~8 rows (ungefähr)
INSERT INTO `mobs` (`id`, `mob_id`, `map_name`, `name`, `level`, `hp`, `pos_x`, `pos_y`, `pos_z`, `respawn_time`) VALUES
	(10, 'mob_boss', 'Dungeon0', 'Neon-Pyramide', 1, 1, 17.5038, 5.10011, 0.152788, 30),
	(11, 'poly_cube_1', 'Arena2', 'Neon-Würfel', 5, 1, 0, 2, 5, 15),
	(12, 'poly_cone_1', 'Arena2', 'Neon-Kegel', 7, 750, 5, 2, 5, 15),
	(13, 'poly_pyramid_1', 'Arena2', 'Neon-Pyramide', 10, 1000, -5, 2, 5, 20),
	(14, 'poly_sphere_1', 'Arena2', 'Neon-Sphäre', 12, 1500, 0, 2, -5, 30),
	(15, 'poly_torus_1', 'Arena2', 'Neon-Torus', 15, 2500, 5, 2, -5, 45),
	(16, 'poly_capsule_1', 'Arena2', 'Neon-Kapsel', 8, 800, -5, 2, -5, 20),
	(17, 'poly_ring_big', 'Arena2', 'Gigantischer Neon-Ring', 20, 10000, 0, 6, 0, 60);

/*!40103 SET TIME_ZONE=IFNULL(@OLD_TIME_ZONE, 'system') */;
/*!40101 SET SQL_MODE=IFNULL(@OLD_SQL_MODE, '') */;
/*!40014 SET FOREIGN_KEY_CHECKS=IFNULL(@OLD_FOREIGN_KEY_CHECKS, 1) */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40111 SET SQL_NOTES=IFNULL(@OLD_SQL_NOTES, 1) */;
