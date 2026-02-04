#include "../GameLogic.hpp"
#include <random>
#include <mutex>

DamageResult GameLogic::getSpellDamage(int playerLevel, int baseMin, int baseMax) {
    static std::mt19937 gen(std::random_device{}());
    static std::mutex genMtx;
    std::lock_guard<std::mutex> lock(genMtx);
    
    std::uniform_real_distribution<> critDis(0, 1);
    std::uniform_int_distribution<> baseDis(baseMin, baseMax);
    
    int chosenBase = baseDis(gen);
    double critChance = 0.15 + (playerLevel - 1) * 0.0048;
    if (critChance > 0.75) critChance = 0.75;
    
    bool isCrit = (critDis(gen) < critChance);
    
    double avgDmg = 0;
    if (playerLevel <= 30) {
        avgDmg = 30.0 + (playerLevel - 1) * 7.0 + std::pow((double)playerLevel, 2.45) * 0.2;
    } else {
        avgDmg = 1100 + (playerLevel - 30) * 35.0;
    }

    double varMult = 1.0 + (playerLevel - 1) * 0.33; 
    double finalDmg = avgDmg + (chosenBase - 30) * varMult;

    if (isCrit) finalDmg *= 1.8;

    return { (int)std::floor(finalDmg), isCrit };
}
