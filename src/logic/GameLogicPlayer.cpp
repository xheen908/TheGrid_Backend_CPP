#include "../GameLogic.hpp"
#include "SocketHandlers.hpp"
#include "Database.hpp"
#include "Logger.hpp"

void GameLogic::awardXP(Player& player, int amount) {
    {
        std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
        player.xp += amount;
        Logger::log("[XP] " + player.charName + " erhielt " + std::to_string(amount) + " Erfahrung.");
        
        json ctMsg = {
            {"type", "combat_text"}, {"target_id", "player"},
            {"value", "+" + std::to_string(amount) + " XP"},
            {"is_crit", false}, {"color", "#32CD32"}
        };
        SocketHandlers::sendToPlayer(player.username, ctMsg.dump());
    }

    auto levelData = getLevelData(player.level);
    bool leveledUp = false;
    {
        std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
        while (player.xp >= levelData.xpToNextLevel && player.level < 100) {
            player.xp -= levelData.xpToNextLevel;
            player.level++;
            levelData = getLevelData(player.level);
            leveledUp = true;
        }
    }

    if (leveledUp) {
        std::string pName, pMap, pUsername;
        {
            std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
            player.maxHp = levelData.hp;
            player.hp = player.maxHp;
            player.maxMana = levelData.mana;
            player.mana = player.maxMana;
            pName = player.charName;
            pMap = player.mapName;
            pUsername = player.username;
            Logger::log("[LEVEL] " + pName + " ist jetzt Level " + std::to_string(player.level) + "!");
        }
        
        json chatMsg = {{"type", "chat_receive"}, {"mode", "system"}, {"message", "Glückwunsch! Du hast Level " + std::to_string(player.level) + " erreicht!"}};
        if (!player.isDisconnected) SocketHandlers::sendToPlayer(pUsername, chatMsg.dump());

        json levelUpMsg = {{"type", "level_up"}, {"username", pName}};
        SocketHandlers::broadcastToMap(pMap, levelUpMsg.dump());
    }

    json statusMsg;
    std::string syncUsername;
    {
        std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
        syncUsername = player.username;
        statusMsg = {
            {"type", "player_status"}, {"username", player.charName},
            {"hp", player.hp}, {"max_hp", player.maxHp},
            {"mana", player.mana}, {"max_mana", player.maxMana},
            {"shield", player.shield}, {"level", player.level},
            {"xp", player.xp}, {"max_xp", levelData.xpToNextLevel}
        };
    }
    if (!player.isDisconnected) SocketHandlers::sendToPlayer(syncUsername, statusMsg.dump());

    Database::getInstance().savePlayer(player);
}

int GameLogic::getInventorySize(int level) {
    if (level <= 10) return 30;
    if (level <= 20) return 40;
    if (level <= 30) return 60;
    if (level <= 40) return 80;
    return 100;
}
