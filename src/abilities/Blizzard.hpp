#ifndef BLIZZARD_HPP
#define BLIZZARD_HPP

#include "Ability.hpp"
#include "../SocketHandlers.hpp"
#include "../GameLogic.hpp"
#include "../Logger.hpp"
#include <cmath>

class Blizzard : public Ability {
public:
    std::string getName() const override { return "Blizzard"; }
    std::string getCategory() const override { return "Mage"; }
    std::string getDescription() const override { return "Beschwört einen Blizzard, der Eiszapfen auf das Zielgebiet regnen lässt.\n[color=cyan]Verursacht Flächenschaden und verlangsamt Ziele.[/color]"; }
    std::string getIcon() const override { return "res://Assets/UI/spell_blizzard.jpg"; }
    float getCastTime() const override { return 8.0f; } // Channeled (8s)
    float getCooldown() const override { return 0.0f; }
    bool isTargeted() const override { return false; }
    bool ignoresGCD() const override { return true; }
    bool canCastWhileMoving() const override { return false; }

    void onCastStart(Player& player, const std::string& targetId, const Vector3& targetPos) const override {
        std::string pName, pMap;
        {
            std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
            pName = player.charName;
            pMap = player.mapName;
        }

        json startMsg = {
            {"type", "spell_cast_start"},
            {"caster", pName},
            {"spell", getName()},
            {"duration", getCastTime()},
            {"target_pos", {{"x", targetPos.x}, {"y", targetPos.y}, {"z", targetPos.z}}},
            {"vfx_path", "res://Assets/BinbunVFX/magic_projectiles/effects/mprojectile_javelin/mprojectile_javelin_vfx_02.tscn"}
        };
        SocketHandlers::broadcastToMap(pMap, startMsg.dump());
    }

    void onCastTick(Player& player, const std::string& targetId, const Vector3& targetPos) const override {
        std::string pMap;
        {
            std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
            pMap = player.mapName;
        }

        // Broadcast tick for VFX (spawns projectiles in frontend)
        json tickMsg = {
            {"type", "spell_cast_tick"},
            {"spell", getName()},
            {"target_pos", {{"x", targetPos.x}, {"y", targetPos.y}, {"z", targetPos.z}}},
            {"vfx_path", "res://Assets/BinbunVFX/magic_projectiles/effects/mprojectile_javelin/mprojectile_javelin_vfx_02.tscn"}
        };
        SocketHandlers::broadcastToMap(pMap, tickMsg.dump());

        // Apply Damage & Debuffs
        applyAoEDamage(player, targetPos);
    }

    void onCastComplete(Player& player, const std::string& targetId, const Vector3& targetPos) const override {
        std::string pName, pMap, pUsername;
        {
            std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
            pUsername = player.username;
            pName = player.charName;
            pMap = player.mapName;
            
            long long now = currentTimeMillis();
            player.cooldowns[getName()] = now + (long long)(getCooldown() * 1000);
            player.gcdUntil = now + 1500;
        }

        json finishMsg = {
            {"type", "spell_cast_finish"},
            {"caster", pUsername},
            {"spell", getName()},
            {"target_pos", {{"x", targetPos.x}, {"y", targetPos.y}, {"z", targetPos.z}}}
        };
        SocketHandlers::broadcastToMap(pMap, finishMsg.dump());
    }

    void onCastInterrupted(Player& player) const override {
        std::string pMap;
        {
            std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
            pMap = player.mapName;
            
            long long now = currentTimeMillis();
            player.cooldowns[getName()] = now + (long long)(getCooldown() * 1000);
            player.gcdUntil = now + 1500;
        }

        json interMsg = {
            {"type", "spell_cast_finish"},
            {"caster", player.charName},
            {"spell", "interrupted"}
        };
        SocketHandlers::broadcastToMap(pMap, interMsg.dump());
    }

private:
    void applyAoEDamage(Player& player, const Vector3& targetPos) const {
        float range = 9.6f; // Increased by 20% (was 8.0f)
        float rangeSq = range * range;
        std::string pMap, pUsername;
        int pLevel;
        {
            std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
            pMap = player.mapName;
            pLevel = player.level;
            pUsername = player.username;
        }

        std::lock_guard<std::recursive_mutex> lock(GameState::getInstance().getMtx());
        auto& mobs = GameState::getInstance().getMobs();
        for (auto& m : mobs) {
            if (m.mapName == pMap && m.hp > 0) {
                float dx = m.transform.x - targetPos.x;
                float dz = m.transform.z - targetPos.z;
                float distSq = dx*dx + dz*dz;
                
                if (distSq <= rangeSq) {
                    auto result = GameLogic::getSpellDamage(pLevel, 10, 15);
                    m.hp = std::max(0, m.hp - result.damage);
                    m.target = pUsername;



                    // Apply Chill debuff (Slow effect)
                    bool found = false;
                    long long endTime = currentTimeMillis() + 6000;
                    for (auto& d : m.debuffs) {
                        if (d.type == "Chill") {
                            d.endTime = std::max(d.endTime, endTime);
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        m.debuffs.push_back({"Chill", endTime});
                        // Logger::log("[Blizzard] Applied Chill to " + m.name);
                    }

                    // INDIVIDUAL MESSAGES FOR THAT SWEET SPAM FACTOR
                    json combatMsg = {
                        {"type", "combat_text"},
                        {"target_id", m.id},
                        {"value", result.damage},
                        {"is_crit", result.isCrit},
                        {"color", result.isCrit ? "#FFD700" : "#FFFF00"}
                    };
                    SocketHandlers::broadcastToMap(pMap, combatMsg.dump());

                    if (m.hp <= 0) {
                        m.respawnAt = currentTimeMillis() + (GameState::getInstance().getRespawnRate(pMap) * 1000);
                        int xpReward = GameLogic::getMobXPReward(m.level, m.dbXp);
                        GameLogic::awardXP(player, xpReward);
                        GameLogic::checkQuestKill(player, m.typeId);
                    }
                }
            }
        }
    }
};

#endif
