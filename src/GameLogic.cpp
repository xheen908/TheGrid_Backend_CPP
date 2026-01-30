#include "GameLogic.hpp"
#include <algorithm>
#include <random>

LevelData GameLogic::getLevelData(int level) {
    return {
        100 + (level * 25) + (level * level * 2),
        100 + (level * 20),
        (int)std::floor(100 * std::pow(level, 1.5))
    };
}

int GameLogic::getMobMaxHp(int level) {
    // f(2) approx 76, f(61) approx 84k
    return 30 + (int)(10.0 * std::pow(level, 2.2));
}

int GameLogic::getMobDamage(int level) {
    // f(2) approx 8, f(61) approx 1300
    return 4 + (int)(1.2 * std::pow(level, 1.7));
}

int GameLogic::getMobXPReward(int level) {
    return (int)std::floor(20 + (level * 15) + (level * level * 0.8));
}

DamageResult GameLogic::getSpellDamage(const Player& player, int baseMin, int baseMax) {
    std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
    static std::mt19937 gen(std::random_device{}());
    static std::mutex genMtx;
    std::lock_guard<std::mutex> lock(genMtx);
    
    std::uniform_real_distribution<> critDis(0, 1);
    std::uniform_int_distribution<> baseDis(baseMin, baseMax);
    
    int chosenBase = baseDis(gen);
    bool isCrit = critDis(gen) < 0.15;
    
    int damage = (int)std::floor(chosenBase + (player.level * 40) + (std::pow(player.level, 2.3) * 1.5));
    if (isCrit) damage = (int)std::floor(damage * 1.8);
    
    return { damage, isCrit };
}

void GameLogic::scaleMobToMap(Mob& mob, const std::map<std::string, float>& mapLevelMap, const std::map<std::string, int>& mapPartySizeMap) {
    auto itLevel = mapLevelMap.find(mob.mapName);
    auto itParty = mapPartySizeMap.find(mob.mapName);

    // If no players on map, keep current or revert to base?
    // Let's keep current level but reset to base if absolutely empty? 
    // Actually, if itLevel is not found, we don't scale.
    if (itLevel == mapLevelMap.end()) return;

    int maxPlayerLevel = (int)std::floor(itLevel->second);
    int targetLevel = maxPlayerLevel + 1;
    if (mob.id == "mob_boss") targetLevel = maxPlayerLevel + 5;

    int maxParty = (itParty != mapPartySizeMap.end()) ? itParty->second : 1;
    float partyMultiplier = 1.0f + (maxParty - 1) * 0.5f;

    // Only update if stats would actually change significantly
    int targetMaxHp = (int)(getMobMaxHp(targetLevel) * partyMultiplier);

    if (mob.level != targetLevel || mob.maxHp != targetMaxHp) {
        float ratio = (mob.maxHp > 0) ? (float)mob.hp / mob.maxHp : 1.0f;
        mob.level = targetLevel;
        mob.maxHp = targetMaxHp;
        
        if (mob.hp > 0) {
            mob.hp = (int)std::floor(mob.maxHp * ratio);
            if (mob.hp <= 0) mob.hp = 1; // Prevent accidental death by rounding
        }
    }
}

#include "SocketHandlers.hpp"
#include "Database.hpp"
#include "Logger.hpp"

void GameLogic::awardXP(Player& player, int amount) {
    {
        std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
        player.xp += amount;
        Logger::log("[XP] " + player.charName + " erhielt " + std::to_string(amount) + " Erfahrung.");
    }

    auto levelData = getLevelData(player.level);
    
    bool leveledUp = false;
    // We don't need a persistent lock here yet, as level is an int, 
    // but let's be safe for the increment
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

        // Broadcast LevelUp effect to everyone on map
        json levelUpMsg = {{"type", "level_up"}, {"username", pName}};
        SocketHandlers::broadcastToMap(pMap, levelUpMsg.dump());
    }

    // Always sync status
    json statusMsg;
    std::string syncUsername;
    {
        std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
        syncUsername = player.username;
        statusMsg = {
            {"type", "player_status"},
            {"username", player.charName},
            {"hp", player.hp},
            {"max_hp", player.maxHp},
            {"mana", player.mana},
            {"max_mana", player.maxMana},
            {"shield", player.shield},
            {"level", player.level},
            {"xp", player.xp},
            {"max_xp", levelData.xpToNextLevel}
        };
    }
    // Sync status senden
    if (!player.isDisconnected) SocketHandlers::sendToPlayer(syncUsername, statusMsg.dump());

    // Immer speichern bei XP-Erhalt (inkl. Level-Up)
    Database::getInstance().savePlayer(player);
}
