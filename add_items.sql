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


-- Exportiere Datenbank-Struktur für world_db
CREATE DATABASE IF NOT EXISTS `world_db` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci */ /*!80016 DEFAULT ENCRYPTION='N' */;
USE `world_db`;

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
	('11', 'Tempotrank', 'Ein neon-grüner Trank, der deine Schritte beschleunigt.', 'Consumable', 'Rare', '{"value": 2.0, "effect": "speed", "duration": 30, "yellow_text": "Erhöht das Lauftempo für 30s."}'),
	('12', 'Stärketrank', 'Ein lila Elixier, das deine Muskeln stählt.', 'Consumable', 'Rare', '{"value": 15, "effect": "strength", "duration": 60, "yellow_text": "Erhöht Stärke für 60s."}'),
	('13', 'Intelligenztrank', 'Ein tiefblauer Trank, der den Geist schärft.', 'Consumable', 'Rare', '{"value": 20, "effect": "intellect", "duration": 60, "yellow_text": "Erhöht Intelligenz für 60s."}'),
	('14', 'Glückstrank', 'Ein seltener goldener Trank, der das Schicksal beeinflusst.', 'Consumable', 'Epic', '{"value": 0.1, "effect": "luck", "duration": 300, "yellow_text": "Erhöht Loot-Chance für 5min."}'),
	('2', 'Heiltrank', 'Ein erfrischendes Gebräu.', 'Consumable', 'Common', '{"value": 50, "effect": "heal", "yellow_text": "Heilt alle HP sofort."}'),
	('3', 'Kurzschwert ', 'Ein einfaches Schwert aus Eisen.', 'Weapon', 'Common', '{"slot": "MainHand", "stats": {"strength": 1}}'),
	('4', 'Magma Rüstung II', 'Eine legendäre Rüstung.', 'Armor', 'Epic', '{"slot": "Chest", "stats": {"armor": 50, "stamina": 20}}'),
	('5', 'Übungs Breitschwert', 'Ein seltendes Schwert aus Titan', 'Weapon', 'Rare', '{"slot": "MainHand", "stats": {"strength": 5}}'),
	('6', 'Wuchtiges Breitschwert', 'Ein monströses Schwert, das nur von den Stärksten geführt werden kann.', 'Weapon', 'Epic', '{"slot": "MainHand", "stats": {"weight": 40, "strength": 15}, "yellow_text": "Zerschmettert feindliche Schilde."}'),
	('7', 'Magischer Stab', 'Ein Stab, durchdrungen von reiner Arkan-Energie.', 'Weapon', 'Rare', '{"slot": "MainHand", "stats": {"intellect": 20, "mana_regen": 5}, "yellow_text": "Erhöht den Zauberschaden massiv."}'),
	('8', 'Hinterhältiger Dolch', 'Eine Klinge, die im Schatten geschmiedet wurde. Perfekt für lautlose Kills.', 'Weapon', 'Rare', '{"slot": "MainHand", "stats": {"agility": 25, "crit_chance": 0.1}, "yellow_text": "Verursacht Giftschaden über Zeit."}'),
	('9', 'Göttliche Barrikade', 'Ein Schild, gesegnet vom Licht. Unzerstörbar und majestätisch.', 'Armor', 'Legendary', '{"slot": "OffHand", "stats": {"armor": 80, "stamina": 40, "block_chance": 0.2}, "yellow_text": "Heiliges Licht schützt den Träger."}');

/*!40103 SET TIME_ZONE=IFNULL(@OLD_TIME_ZONE, 'system') */;
/*!40101 SET SQL_MODE=IFNULL(@OLD_SQL_MODE, '') */;
/*!40014 SET FOREIGN_KEY_CHECKS=IFNULL(@OLD_FOREIGN_KEY_CHECKS, 1) */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40111 SET SQL_NOTES=IFNULL(@OLD_SQL_NOTES, 1) */;
