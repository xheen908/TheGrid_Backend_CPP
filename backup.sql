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
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- Exportiere Daten aus Tabelle auth_db.users: ~4 rows (ungefähr)
INSERT INTO `users` (`id`, `username`, `password_hash`, `gm_status`, `created_at`) VALUES
	(1, 'xheen908', 'password', 1, '2026-01-30 15:45:02'),
	(2, 'admin', 'admin', 1, '2026-01-30 15:45:02'),
	(3, 'client', 'client', 0, '2026-01-30 15:45:02'),
	(5, 'neu', 'neu', 0, '2026-02-04 17:57:36');


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
  `pos_x` float DEFAULT '0.32',
  `pos_y` float DEFAULT '2.37',
  `pos_z` float DEFAULT '-12.08',
  `rotation` float DEFAULT '3.11',
  `level` int DEFAULT '1',
  `xp` int DEFAULT '0',
  `hp` int DEFAULT '125',
  `max_hp` int DEFAULT '125',
  `mana` int DEFAULT '120',
  `max_mana` int DEFAULT '120',
  `last_login` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `char_name` (`char_name`)
) ENGINE=InnoDB AUTO_INCREMENT=131 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- Exportiere Daten aus Tabelle charakter_db.characters: ~7 rows (ungefähr)
INSERT INTO `characters` (`id`, `user_id`, `char_name`, `character_class`, `map_name`, `pos_x`, `pos_y`, `pos_z`, `rotation`, `level`, `xp`, `hp`, `max_hp`, `mana`, `max_mana`, `last_login`) VALUES
	(103, 2, 'Yheen', 'Mage', 'TestMap0', 29.5553, -0.597194, 29.2193, 2.75, 5, 410, 312, 341, 225, 225, '2026-02-04 21:13:41'),
	(104, 2, 'Xheen', 'Mage', 'TestMap0', 35.5311, -0.835729, 29.065, 0.24, 1, 150, 0, 151, 99, 99, '2026-02-04 20:34:52'),
	(118, 3, 'Mage', 'Mage', 'TestMap0', 26.8669, 0.030113, 28.6313, 0, 15, 8264, 1086, 1401, 400, 924, '2026-02-04 20:03:31'),
	(119, 3, 'Barbar', 'Barbarian', 'TestMap0', 17.4238, 4.04682, 31.7685, 0, 1, 0, 125, 151, 99, 99, '2026-02-04 18:00:01'),
	(121, 3, 'Ranger', 'Ranger', 'WorldMap0', 0, 0, 0, 0, 1, 0, 125, 125, 120, 120, '2026-01-31 17:04:53'),
	(122, 3, 'Knight', 'Knight', 'WorldMap0', 0, 0, 0, 0, 1, 0, 125, 125, 120, 120, '2026-01-31 17:05:06'),
	(123, 3, 'Rogue', 'Rogue', 'WorldMap0', -20.0564, 0, 8.42799, 0, 3, 295, 193, 193, 160, 160, '2026-01-31 17:21:29'),
	(127, 5, 'Vienneun', 'Mage', 'TestMap0', 21.7491, 1.90005, 31.0157, 3.11, 1, 150, 151, 151, 99, 99, '2026-02-04 20:09:44'),
	(128, 1, 'Admin', 'Mage', 'WorldMap0', -10.2745, 2.37, -0.984131, 3.11, 1, 0, 125, 151, 99, 99, '2026-02-04 18:02:18'),
	(129, 5, 'VierSieben', 'Rogue', 'TestMap0', 32.7233, -0.365782, 31.292, 3.11, 1, 100, 125, 151, 99, 99, '2026-02-04 20:16:35'),
	(130, 5, 'Neuneu', 'Mage', 'TestMap0', 34.6077, -1.39216, 22.9986, 3.11, 1, 200, 0, 151, 99, 99, '2026-02-04 20:19:02');

-- Exportiere Struktur von Tabelle charakter_db.character_inventory
CREATE TABLE IF NOT EXISTS `character_inventory` (
  `id` int NOT NULL AUTO_INCREMENT,
  `character_id` int NOT NULL,
  `item_id` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `slot_index` int NOT NULL,
  `quantity` int DEFAULT '1',
  `is_equipped` tinyint(1) DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `character_id` (`character_id`),
  CONSTRAINT `character_inventory_ibfk_1` FOREIGN KEY (`character_id`) REFERENCES `characters` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=977 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- Exportiere Daten aus Tabelle charakter_db.character_inventory: ~17 rows (ungefähr)
INSERT INTO `character_inventory` (`id`, `character_id`, `item_id`, `slot_index`, `quantity`, `is_equipped`) VALUES
	(698, 128, '1', 0, 1, 0),
	(913, 118, '2', 0, 20, 0),
	(914, 118, '11', 2, 1, 0),
	(915, 118, '4', 4, 1, 0),
	(916, 118, '7', 3, 1, 0),
	(917, 118, '2', 1, 1, 0),
	(928, 127, '2', 0, 10, 0),
	(937, 104, '1', 0, 1, 0),
	(938, 104, '3', 25, 1, 0),
	(939, 104, '4', 26, 1, 0),
	(940, 104, '10', 5, 1, 0),
	(941, 104, '11', 16, 1, 0),
	(942, 104, '12', 17, 1, 0),
	(943, 104, '13', 4, 1, 0),
	(944, 104, '14', 11, 1, 0),
	(945, 104, '6', 27, 1, 0),
	(946, 104, '7', 24, 1, 0),
	(972, 103, '1', 0, 1, 0),
	(973, 103, '3', 1, 1, 0),
	(974, 103, '6', 2, 1, 0),
	(975, 103, '5', 3, 1, 0),
	(976, 103, '2', 4, 16, 0);


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

-- Exportiere Daten aus Tabelle world_db.game_objects: ~9 rows (ungefähr)
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
	(10, 'WorldMap0', 'portal', 0.018641, 2.37008, -0.938741, 0, -1.57429, 0, '{"color": "#072cfa", "spawn_pos": {"x": 8.19, "y": 6.52, "z": 32.61}, "target_map": "TestMap0", "spawn_rot_y": -1.47}');

-- Exportiere Struktur von Tabelle world_db.item_templates
CREATE TABLE IF NOT EXISTS `item_templates` (
  `item_id` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `name` varchar(100) NOT NULL,
  `description` text,
  `type` enum('Weapon','Armor','Consumable','Quest','Material') NOT NULL DEFAULT 'Material',
  `rarity` enum('Common','Rare','Epic','Legendary') NOT NULL DEFAULT 'Common',
  `component_data` json DEFAULT NULL,
  PRIMARY KEY (`item_id`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- Exportiere Daten aus Tabelle world_db.item_templates: ~14 rows (ungefähr)
INSERT INTO `item_templates` (`item_id`, `name`, `description`, `type`, `rarity`, `component_data`) VALUES
	('1', 'GM Kodex', 'Ein heiliges Relikt für Game Master.', 'Quest', 'Legendary', '{"action": "open_gm_menu", "yellow_text": "Öffnet das GM Menü."}'),
	('10', 'Manatrank', 'Ein pulsierendes blaues Gebräu, das die geistige Energie wiederherstellt.', 'Consumable', 'Common', '{"value": 50, "effect": "mana", "yellow_text": "Stellt 50 Mana wieder her."}'),
	('11', 'Tempotrank', 'Ein neon-grüner Trank, der deine Schritte beschleunigt.', 'Consumable', 'Common', '{"value": 2.0, "effect": "speed", "duration": 30, "yellow_text": "Erhöht das Lauftempo für 30s."}'),
	('12', 'Stärketrank', 'Ein lila Elixier, das deine Muskeln stählt.', 'Consumable', 'Common', '{"value": 15, "effect": "strength", "duration": 60, "yellow_text": "Erhöht Stärke für 60s."}'),
	('13', 'Intelligenztrank', 'Ein tiefblauer Trank, der den Geist schärft.', 'Consumable', 'Common', '{"value": 20, "effect": "intellect", "duration": 60, "yellow_text": "Erhöht Intelligenz für 60s."}'),
	('14', 'Glückstrank', 'Ein seltener goldener Trank, der das Schicksal beeinflusst.', 'Consumable', 'Epic', '{"value": 0.1, "effect": "luck", "duration": 300, "yellow_text": "Erhöht Loot-Chance für 5min."}'),
	('2', 'Heiltrank', 'Ein erfrischendes Gebräu.', 'Consumable', 'Common', '{"value": 50, "effect": "heal", "yellow_text": "Heilt alle HP sofort."}'),
	('3', 'Kurzschwert ', 'Ein einfaches Schwert aus Eisen.', 'Weapon', 'Common', '{"slot": "MainHand", "stats": {"strength": 1}}'),
	('4', 'Magma Rüstung II', 'Eine legendäre Rüstung.', 'Armor', 'Epic', '{"slot": "Chest", "stats": {"armor": 50, "stamina": 20}}'),
	('5', 'Übungs Breitschwert', 'Ein seltendes Schwert aus Titan', 'Weapon', 'Rare', '{"slot": "MainHand", "stats": {"strength": 5}}'),
	('6', 'Wuchtiges Breitschwert', 'Ein monströses Schwert, das nur von den Stärksten geführt werden kann.', 'Weapon', 'Epic', '{"slot": "MainHand", "stats": {"weight": 40, "strength": 15}, "yellow_text": "Zerschmettert feindliche Schilde."}'),
	('7', 'Magischer Stab', 'Ein Stab, durchdrungen von reiner Arkan-Energie.', 'Weapon', 'Rare', '{"slot": "MainHand", "stats": {"intellect": 20, "mana_regen": 5}, "yellow_text": "Erhöht den Zauberschaden massiv."}'),
	('8', 'Hinterhältiger Dolch', 'Eine Klinge, die im Schatten geschmiedet wurde. Perfekt für lautlose Kills.', 'Weapon', 'Rare', '{"slot": "MainHand", "stats": {"agility": 25, "crit_chance": 0.1}, "yellow_text": "Verursacht Giftschaden über Zeit."}'),
	('9', 'Göttliche Barrikade', 'Ein Schild, gesegnet vom Licht. Unzerstörbar und majestätisch.', 'Armor', 'Legendary', '{"slot": "OffHand", "stats": {"armor": 80, "stamina": 40, "block_chance": 0.2}, "yellow_text": "Heiliges Licht schützt den Träger."}');

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
  `model_id` varchar(50) DEFAULT 'neon_sphere',
  `mob_type` enum('Normal','Elite','Rare','Boss') DEFAULT 'Normal',
  `level` int DEFAULT '1',
  `hp` int DEFAULT '100',
  `pos_x` float DEFAULT '0',
  `pos_y` float DEFAULT '0',
  `pos_z` float DEFAULT '0',
  `respawn_time` int DEFAULT '30',
  PRIMARY KEY (`id`),
  UNIQUE KEY `mob_id` (`mob_id`)
) ENGINE=InnoDB AUTO_INCREMENT=22 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- Exportiere Daten aus Tabelle world_db.mobs: ~8 rows (ungefähr)
INSERT INTO `mobs` (`id`, `mob_id`, `map_name`, `name`, `model_id`, `mob_type`, `level`, `hp`, `pos_x`, `pos_y`, `pos_z`, `respawn_time`) VALUES
	(10, '10', 'Dungeon0', 'Neon-Pyramide', 'neon_sphere', 'Boss', 1, 150, 17.5038, 5.10011, 0.152788, 30),
	(11, '11', 'Arena2', 'Neon-Würfel', 'neon_sphere', 'Normal', 1, 150, 0, 2, 5, 15),
	(12, '12', 'Arena2', 'Neon-Kegel', 'neon_sphere', 'Normal', 1, 150, 5, 2, 5, 15),
	(13, '13', 'Arena2', 'Neon-Pyramide', 'neon_sphere', 'Normal', 1, 150, -5, 2, 5, 20),
	(14, '14', 'Arena2', 'Neon-Sphäre', 'neon_sphere', 'Normal', 1, 150, 0, 2, -5, 30),
	(15, '15', 'Arena2', 'Neon-Torus', 'neon_sphere', 'Normal', 1, 150, 5, 2, -5, 45),
	(16, '16', 'Arena2', 'Neon-Kapsel', 'neon_sphere', 'Normal', 1, 150, -5, 2, -5, 20),
	(17, '17', 'Arena2', 'Gigantischer Neon-Ring', 'neon_sphere', 'Normal', 1, 150, 0, 6, 0, 60),
	(18, '18', 'TestMap0', 'Zombie Rogue', 'skeleton_rogue', 'Normal', 1, 150, 56.3967, -0.378409, 18.9367, 60),
	(19, '19', 'TestMap0', 'Zombie Minion', 'skeleton_minion', 'Normal', 1, 150, 52.3967, -0.178409, 18.9367, 60),
	(20, '20', 'TestMap0', 'Zombie Magier', 'skeleton_mage', 'Normal', 1, 150, 65.4555, -1.6628, 24.6979, 60),
	(21, '21', 'TestMap0', 'Zombie Magier', 'skeleton_mage', 'Normal', 1, 150, 67.4555, -2.6628, 26.6979, 60);

/*!40103 SET TIME_ZONE=IFNULL(@OLD_TIME_ZONE, 'system') */;
/*!40101 SET SQL_MODE=IFNULL(@OLD_SQL_MODE, '') */;
/*!40014 SET FOREIGN_KEY_CHECKS=IFNULL(@OLD_FOREIGN_KEY_CHECKS, 1) */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40111 SET SQL_NOTES=IFNULL(@OLD_SQL_NOTES, 1) */;
