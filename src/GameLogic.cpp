#include "GameLogic.hpp"
#include <algorithm>
#include <random>

LevelData GameLogic::getLevelData(int level) {
    // Mob XP Base (Zero-diff reward)
    int mxp = 45 + (5 * level);
    
    // Diff(L) logic
    int diff = 0;
    if (level >= 60) diff = 25;
    else if (level >= 12) diff = 13 + (int)std::floor((level - 12) / 4);
    else if (level == 11) diff = 12;
    else if (level == 10) diff = 11;
    else if (level == 9) diff = 9;
    else if (level == 8) diff = 5;
    else diff = 0;

    int xpRequired = (8 * level + diff) * mxp;
    
    // Nearest 100 rounding (standard for this formula)
    xpRequired = ((xpRequired + 50) / 100) * 100;

    return {
        100 + (level * 25) + (level * level * 2), // HP
        100 + (level * 20),                       // Mana
        xpRequired                                // XP to Next Level
    };
}

int GameLogic::getMobMaxHp(int level) {
    // Balanced to die in ~5 spells: Average Damage * 5
    // Average base damage at L1 is 30 (midpoint of 20-40)
    double base = 30.0;
    double avgDmg;
    if (level <= 30) {
        avgDmg = base + (level - 1) * 7.0 + std::pow((double)level, 2.45) * 0.2;
    } else {
        avgDmg = 1100 + (level - 30) * 35.0;
    }
    return (int)(avgDmg * 5);
}

int GameLogic::getMobDamage(int level) {
    // Soft scale to player HP (L1: ~10, L60: ~500)
    return (int)(10 + (level * 2.0) + (std::pow((double)level, 1.8) * 0.5));
}

int GameLogic::getMobXPReward(int level) {
    // Standard Mob XP Reward for equal level
    return 45 + (5 * level);
}

DamageResult GameLogic::getSpellDamage(int playerLevel, int baseMin, int baseMax) {
    static std::mt19937 gen(std::random_device{}());
    static std::mutex genMtx;
    std::lock_guard<std::mutex> lock(genMtx);
    
    std::uniform_real_distribution<> critDis(0, 1);
    std::uniform_int_distribution<> baseDis(baseMin, baseMax);
    
    int chosenBase = baseDis(gen);
    
    // Calculate dynamic crit chance: 15% base + level-based growth
    // At level 1: ~15%, Level 60: ~43%
    double critChance = 0.15 + (playerLevel - 1) * 0.0048; // Scaliert gegen 43% auf L60
    if (critChance > 0.75) critChance = 0.75; // Cap at 75%
    
    bool isCrit = (critDis(gen) < critChance);
    
    // 1. Calculate the target average for this level
    double avgDmg = 0;
    if (playerLevel <= 30) {
        avgDmg = 30.0 + (playerLevel - 1) * 7.0 + std::pow((double)playerLevel, 2.45) * 0.2;
    } else {
        avgDmg = 1100 + (playerLevel - 30) * 35.0;
    }

    // 2. Calculate variance Multiplier
    double varMult = 1.0 + (playerLevel - 1) * 0.33; 
    
    // 3. Apply variance
    double finalDmg = avgDmg + (chosenBase - 30) * varMult;

    if (isCrit) finalDmg *= 1.8; // Crit multiplier 1.8x

    int finalDamageInt = (int)std::floor(finalDmg);
    
    return { finalDamageInt, isCrit };
}

void GameLogic::scaleMobToMap(Mob& mob, const std::map<std::string, float>& mapLevelMap, const std::map<std::string, int>& mapPartySizeMap) {
    auto itLevel = mapLevelMap.find(mob.mapName);
    auto itParty = mapPartySizeMap.find(mob.mapName);

    // If no players on map, keep current or revert to base?
    // Let's keep current level but reset to base if absolutely empty? 
    // Actually, if itLevel is not found, we don't scale.
    if (itLevel == mapLevelMap.end()) return;

    int maxPlayerLevel = (int)std::floor(itLevel->second);
    int targetLevel = maxPlayerLevel; 
    
    // Explicit Multipliers based on Mob Type
    float typeMultiplier = 1.0f;
    if (mob.mobType == "Rare") typeMultiplier = 2.0f;
    else if (mob.mobType == "Elite") typeMultiplier = 4.0f;
    else if (mob.mobType == "Boss") typeMultiplier = 10.0f;

    int maxParty = (itParty != mapPartySizeMap.end()) ? itParty->second : 1;
    float partyMultiplier = 1.0f + (maxParty - 1) * 0.5f;

    // Dynamic Ratio Scaling:
    // Scale DB_HP based on the ratio of Power(TargetLevel) / Power(DB_Level)
    double baseScale = (double)getMobMaxHp(mob.dbLevel);
    double targetScale = (double)getMobMaxHp(targetLevel);
    int targetMaxHp = (int)((double)mob.dbMaxHp * (targetScale / baseScale) * typeMultiplier * partyMultiplier);

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
        
        // Show floating XP text on player
        json ctMsg = {
            {"type", "combat_text"},
            {"target_id", "player"},
            {"value", "+" + std::to_string(amount) + " XP"},
            {"is_crit", false},
            {"color", "#32CD32"} // LimeGreen for XP
        };
        SocketHandlers::sendToPlayer(player.username, ctMsg.dump());
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
