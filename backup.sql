-- --------------------------------------------------------
-- Host:                         127.0.0.1
-- Server-Version:               8.0.45 - MySQL Community Server - GPL
-- Server-Betriebssystem:        Linux
-- HeidiSQL Version:             12.14.0.7165
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
) ENGINE=InnoDB AUTO_INCREMENT=124 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- Exportiere Daten aus Tabelle charakter_db.characters: ~7 rows (ungefähr)
INSERT INTO `characters` (`id`, `user_id`, `char_name`, `character_class`, `map_name`, `pos_x`, `pos_y`, `pos_z`, `rotation`, `level`, `xp`, `hp`, `max_hp`, `mana`, `max_mana`, `last_login`) VALUES
	(103, 2, 'Yheen', 'Mage', 'WorldMap0', -13.3121, 0, 6.65896, 2.75, 7, 1220, 373, 373, 240, 240, '2026-01-30 22:56:43'),
	(104, 2, 'Xheen', 'Mage', 'WorldMap0', 0, 0, 0, 0.24, 68, 54186, 0, 11048, 1460, 1460, '2026-01-31 19:06:05'),
	(118, 3, 'Mage', 'Mage', 'WorldMap0', -0.138051, 0.000154, -7.23019, 0, 15, 4304, 0, 925, 400, 400, '2026-01-31 19:07:10'),
	(119, 3, 'Barbar', 'Barbarian', 'WorldMap0', 0, 0, 0, 0, 1, 0, 125, 125, 120, 120, '2026-01-31 17:04:24'),
	(121, 3, 'Ranger', 'Ranger', 'WorldMap0', 0, 0, 0, 0, 1, 0, 125, 125, 120, 120, '2026-01-31 17:04:53'),
	(122, 3, 'Knight', 'Knight', 'WorldMap0', 0, 0, 0, 0, 1, 0, 125, 125, 120, 120, '2026-01-31 17:05:06'),
	(123, 3, 'Rogue', 'Rogue', 'WorldMap0', -20.0564, 0, 8.42799, 0, 3, 295, 193, 193, 160, 160, '2026-01-31 17:21:29');


-- Exportiere Datenbank-Struktur für world_db
CREATE DATABASE IF NOT EXISTS `world_db` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci */ /*!80016 DEFAULT ENCRYPTION='N' */;
USE `world_db`;

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
) ENGINE=InnoDB AUTO_INCREMENT=11 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- Exportiere Daten aus Tabelle world_db.mobs: ~13 rows (ungefähr)
INSERT INTO `mobs` (`id`, `mob_id`, `map_name`, `name`, `level`, `hp`, `pos_x`, `pos_y`, `pos_z`, `respawn_time`) VALUES
	(5, 'arena_01', 'Arena0', 'Bit-Wächter', 1, 100, 0, 0, -10, 30),
	(6, 'arena_02', 'Arena0', 'Bit-Wächter', 1, 100, -8, 0, 14, 30),
	(7, 'arena_03', 'Arena0', 'Bit-Veteran', 3, 450, -5, 0, -20, 30),
	(8, 'arena_g2_1', 'Arena0', 'Duo-Bit A', 2, 200, -10, 0, -5, 30),
	(9, 'arena_g2_2', 'Arena0', 'Duo-Bit B', 2, 200, -11, 0, -6, 30),
	(10, 'mob_boss', 'Dungeon0', 'System-Kernel', 16, 15000, 17.5038, 5.10011, 0.152788, 30),
	(11, 'poly_cube_1', 'Arena2', 'Neon-Würfel', 5, 500, 0, 2, 5, 15),
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
