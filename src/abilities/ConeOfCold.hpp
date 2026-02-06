#ifndef CONEOFCOLD_HPP
#define CONEOFCOLD_HPP

#include "Ability.hpp"
#include "../GameLogic.hpp"
#include "../SocketHandlers.hpp"
#include <cmath>
#include <algorithm>

class ConeOfCold : public Ability {
public:
    std::string getName() const override { return "Kältekegel"; }
    std::string getCategory() const override { return "Mage"; }
    std::string getDescription() const override { return "Schaden und Verlangsamung vor dir.\n[color=cyan]Dauer: 6 Sek.[/color]"; }
    std::string getIcon() const override { return "res://Assets/UI/spell_cone_of_cold.jpg"; }
    float getCastTime() const override { return 0.0f; }
    float getCooldown() const override { return 10.0f; }
    bool isTargeted() const override { return false; }
    bool canCastWhileMoving() const override { return true; }

    void onCastStart(Player& player, const std::string& targetId, const Vector3& targetPos) const override {}

    void onCastComplete(Player& player, const std::string& targetId, const Vector3& targetPos) const override {
        float range = 25.0f; // Erhöhte Reichweite (Range-Zauber)
        float coneAngleRad = (120.0f / 2.0f) * (3.14159265359f / 180.0f);
        
        std::string pName, pMap, pUsername;
        Vector3 pPos;
        Rotation pRot;
        int pLevel = 1;
        {
            std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
            pName = player.charName;
            pMap = player.mapName;
            pUsername = player.username;
            pPos = player.lastPos;
            pRot = player.rotation;
            pLevel = player.level;
            player.cooldowns[getName()] = currentTimeMillis() + (long long)(getCooldown() * 1000);
            player.gcdUntil = currentTimeMillis() + 1500;
        }

        Logger::log("[Ability] Kältekegel cast by " + pName + " (Range: " + std::to_string(range) + ")");

        // Visual effect (Frontend hat +PI Korrektur)
        json coneMsg = {
            {"type", "spell_cast_finish"},
            {"caster", pName},
            {"spell", getName()},
            {"mode", "aoe_cone"},
            {"pos", {{"x", pPos.x}, {"y", pPos.y}, {"z", pPos.z}}},
            {"rot", {{"x", pRot.x}, {"y", pRot.y}, {"z", pRot.z}}}
        };
        SocketHandlers::broadcastToMap(pMap, coneMsg.dump());

        struct MobHit {
            std::string id;
            std::string typeId;
            int damage;
            bool isCrit;
            bool died;
            int xpReward;
        };
        std::vector<MobHit> hits;

        {
            std::lock_guard<std::recursive_mutex> lock(GameState::getInstance().getMtx());
            auto& mobs = GameState::getInstance().getMobs();

            // Korrektur: Die Blickrichtung im Backend war um 180 Grad versetzt (wie im Frontend)
            float forwardAngle = pRot.y + 3.14159265359f;

            for (auto& m : mobs) {
                if (m.mapName == pMap && m.hp > 0) {
                    float dx = m.transform.x - pPos.x;
                    float dz = m.transform.z - pPos.z;
                    float dist = std::sqrt(dx*dx + dz*dz);
                    
                    if (dist <= range && dist > 0.1f) {
                        float targetAngle = std::atan2(dx, dz);
                        float angleDiff = std::abs(targetAngle - forwardAngle);
                        
                        // Wrapping
                        while (angleDiff > 3.14159265359f) {
                            angleDiff = std::abs(angleDiff - 2.0f * 3.14159265359f);
                        }
                        
                        if (angleDiff <= coneAngleRad) {
                            auto result = GameLogic::getSpellDamage(pLevel, 30, 50); // Mehr Schaden als Frost Nova
                            m.hp = std::max(0, m.hp - result.damage);
                            m.target = pUsername;

                            // Apply "Chill" debuff (Slow)
                            bool found = false;
                            long long endTime = currentTimeMillis() + 10000;
                            for (auto& d : m.debuffs) {
                                if (d.type == "Chill") {
                                    d.endTime = std::max(d.endTime, endTime);
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) m.debuffs.push_back({"Chill", endTime});

                            MobHit hit;
                            hit.id = m.id;
                            hit.typeId = m.typeId;
                            hit.damage = result.damage;
                            hit.isCrit = result.isCrit;
                            hit.died = false;
                            hit.xpReward = 0;

                            if (m.hp <= 0) {
                                hit.died = true;
                                m.respawnAt = currentTimeMillis() + (GameState::getInstance().getRespawnRate(pMap) * 1000);
                                hit.xpReward = GameLogic::getMobXPReward(m.level, m.dbXp);
                            }
                            hits.push_back(hit);
                        }
                    }
                }
            }
        }

        for (const auto& hit : hits) {
            json ctMsg = {
                {"type", "combat_text"},
                {"target_id", hit.id},
                {"value", hit.damage},
                {"is_crit", hit.isCrit},
                {"color", hit.isCrit ? "#FFD700" : "#00FFFF"}
            };
            SocketHandlers::broadcastToMap(pMap, ctMsg.dump());

            if (hit.died) {
                GameLogic::awardXP(player, hit.xpReward);
                GameLogic::checkQuestKill(player, hit.typeId);
            }
        }
    }

    void onCastInterrupted(Player& player) const override {}
};

#endif
