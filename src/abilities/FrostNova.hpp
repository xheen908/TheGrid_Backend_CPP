#ifndef FROSTNOVA_HPP
#define FROSTNOVA_HPP

#include "Ability.hpp"
#include "../GameLogic.hpp"
#include "../SocketHandlers.hpp"
#include <cmath>

class FrostNova : public Ability {
public:
    std::string getName() const override { return "Frost Nova"; }
    float getCastTime() const override { return 0.0f; }
    float getCooldown() const override { return 25.0f; }
    bool isTargeted() const override { return false; }
    bool canCastWhileMoving() const override { return true; }

    void onCastStart(Player& player, const std::string& targetId) const override {}

    void onCastComplete(Player& player, const std::string& targetId) const override {
        float range = 10.0f;
        std::string pName, pMap, pUsername;
        Vector3 pPos;
        int pLevel = 1;
        {
            std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
            pName = player.charName;
            pMap = player.mapName;
            pUsername = player.username;
            pPos = player.lastPos;
            pLevel = player.level;
            player.cooldowns[getName()] = currentTimeMillis() + (long long)(getCooldown() * 1000);
            player.gcdUntil = currentTimeMillis() + 1500;
        }

        Logger::log("[Ability] Frost Nova cast by " + pName);

        // Visual effect
        json novaMsg = {
            {"type", "spell_cast_finish"},
            {"caster", pName},
            {"spell", getName()},
            {"mode", "aoe"},
            {"pos", {{"x", pPos.x}, {"y", pPos.y}, {"z", pPos.z}}}
        };
        SocketHandlers::broadcastToMap(pMap, novaMsg.dump());

        struct MobHit {
            std::string id;
            int damage;
            bool isCrit;
            bool died;
            int xpReward;
        };
        std::vector<MobHit> hits;

        {
            std::lock_guard<std::recursive_mutex> lock(GameState::getInstance().getMtx());
            auto& mobs = GameState::getInstance().getMobs();

            for (auto& m : mobs) {
                if (m.mapName == pMap && m.hp > 0) {
                    float dist = std::sqrt(std::pow(m.transform.x - pPos.x, 2) + 
                                          std::pow(m.transform.z - pPos.z, 2));
                    if (dist <= range) {
                        auto result = GameLogic::getSpellDamage(pLevel, 15, 25);
                        m.hp = std::max(0, m.hp - result.damage);
                        m.target = pUsername;

                        // Debuff
                        bool found = false;
                        long long endTime = currentTimeMillis() + 8000;
                        for (auto& d : m.debuffs) {
                            if (d.type == "Frozen") {
                                d.endTime = std::max(d.endTime, endTime);
                                found = true;
                                break;
                            }
                        }
                        if (!found) m.debuffs.push_back({"Frozen", endTime});

                        MobHit hit;
                        hit.id = m.id;
                        hit.damage = result.damage;
                        hit.isCrit = result.isCrit;
                        hit.died = false;
                        hit.xpReward = 0;

                        if (m.hp <= 0) {
                            hit.died = true;
                            m.respawnAt = currentTimeMillis() + (GameState::getInstance().getRespawnRate(pMap) * 1000);
                            hit.xpReward = GameLogic::getMobXPReward(m.level);
                        }
                        hits.push_back(hit);
                    }
                }
            }
        } // Lock released

        // Process broadcasts outside lock
        for (const auto& hit : hits) {
            json ctMsg = {
                {"type", "combat_text"},
                {"target_id", hit.id},
                {"value", hit.damage},
                {"is_crit", hit.isCrit},
                {"color", hit.isCrit ? "#FFD700" : "#FFFF00"}
            };
            SocketHandlers::broadcastToMap(pMap, ctMsg.dump());

            if (hit.died) {
                GameLogic::awardXP(player, hit.xpReward);
            }
        }
    }

    void onCastInterrupted(Player& player) const override {}
};

#endif
