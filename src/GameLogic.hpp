#ifndef GAMELOGIC_HPP
#define GAMELOGIC_HPP

#include "GameState.hpp"
#include <cmath>

struct LevelData {
    int hp;
    int mana;
    int xpToNextLevel;
};

struct DamageResult {
    int damage;
    bool isCrit;
};

class GameLogic {
public:
    static LevelData getLevelData(int level);
    static int getMobMaxHp(int level);
    static int getMobDamage(int level);
    static int getMobXPReward(int level, int baseXP = 50);
    static DamageResult getSpellDamage(int playerLevel, int baseMin, int baseMax);
    static void scaleMobToMap(Mob& mob, const std::map<std::string, float>& mapLevelMap, const std::map<std::string, int>& mapPartySizeMap);
    static void awardXP(Player& player, int amount);
    static int getInventorySize(int level);
    static void checkQuestKill(Player& player, const std::string& mobTypeId);
};

#endif
