#include "../GameLogic.hpp"
#include <cmath>
#include <algorithm>

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
    xpRequired = ((xpRequired + 50) / 100) * 100;

    // HP & Mana scaling
    double avgPower;
    if (level <= 30) {
        avgPower = 30.0 + (level - 1) * 7.0 + std::pow((double)level, 2.45) * 0.2;
    } else {
        avgPower = 1100 + (level - 30) * 35.0;
    }

    return { (int)(avgPower * 5.0), (int)(avgPower * 3.3), xpRequired };
}

int GameLogic::getMobMaxHp(int level) {
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
    return (int)(10 + (level * 2.0) + (std::pow((double)level, 1.8) * 0.5));
}

int GameLogic::getMobXPReward(int level) {
    return 45 + (5 * level);
}

void GameLogic::scaleMobToMap(Mob& mob, const std::map<std::string, float>& mapLevelMap, const std::map<std::string, int>& mapPartySizeMap) {
    auto itLevel = mapLevelMap.find(mob.mapName);
    auto itParty = mapPartySizeMap.find(mob.mapName);

    if (itLevel == mapLevelMap.end()) return;

    int maxPlayerLevel = (int)std::floor(itLevel->second);
    int targetLevel = maxPlayerLevel; 
    
    float typeMultiplier = 1.0f;
    if (mob.mobType == "Rare") typeMultiplier = 2.0f;
    else if (mob.mobType == "Elite") typeMultiplier = 4.0f;
    else if (mob.mobType == "Boss") typeMultiplier = 10.0f;

    int maxParty = (itParty != mapPartySizeMap.end()) ? itParty->second : 1;
    float partyMultiplier = 1.0f + (maxParty - 1) * 0.5f;

    double baseScale = (double)getMobMaxHp(mob.dbLevel);
    double targetScale = (double)getMobMaxHp(targetLevel);
    int targetMaxHp = (int)((double)mob.dbMaxHp * (targetScale / baseScale) * typeMultiplier * partyMultiplier);

    if (mob.level != targetLevel || mob.maxHp != targetMaxHp) {
        float ratio = (mob.maxHp > 0) ? (float)mob.hp / mob.maxHp : 1.0f;
        mob.level = targetLevel;
        mob.maxHp = targetMaxHp;
        if (mob.hp > 0) {
            mob.hp = (int)std::floor(mob.maxHp * ratio);
            if (mob.hp <= 0) mob.hp = 1;
        }
    }
}
