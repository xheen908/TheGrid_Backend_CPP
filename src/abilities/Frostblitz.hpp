#ifndef FROSTBLITZ_HPP
#define FROSTBLITZ_HPP

#include "Ability.hpp"
#include "../GameLogic.hpp"
#include "../SocketHandlers.hpp"
#include "../Logger.hpp"

class Frostblitz : public Ability {
public:
    std::string getName() const override { return "Frostblitz"; }
    std::string getCategory() const override { return "Mage"; }
    std::string getDescription() const override { return "Schießt Frost auf das Ziel.\n[color=cyan]Verursacht Frost-Schaden.[/color]"; }
    std::string getIcon() const override { return "res://Assets/UI/spell_frostblitz.jpg"; }
    float getCastTime() const override { return 2.0f; }
    float getCooldown() const override { return 0.0f; }
    bool isTargeted() const override { return true; }
    bool ignoresGCD() const override { return true; }

    void onCastStart(Player& player, const std::string& targetId, const Vector3& targetPos) const override {
        std::string pName, pMap;
        {
            std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
            pName = player.charName;
            pMap = player.mapName;
        }
        
        // Debug: Send a system message so the player knows the server recognized the cast
        json debugMsg = {
            {"type", "chat_receive"},
            {"mode", "system"},
            {"message", "[DEBUG] Zauber Start: " + getName() + " (Caster: '" + pName + "')"}
        };
        SocketHandlers::sendToPlayer(player.username, debugMsg.dump());

        std::string targetDetails = targetId;
        {
            std::lock_guard<std::recursive_mutex> lock(GameState::getInstance().getMtx());
            for (auto const& m : GameState::getInstance().getMobs()) {
                if (m.id == targetId) {
                    targetDetails = m.name + "(" + targetId + ") [" + m.mobType + "]";
                    break;
                }
            }
        }

        Logger::log("[Ability] " + getName() + " cast start by " + pName + " @ " + targetDetails + " on " + pMap);
        json startMsg = {
            {"type", "spell_cast_start"},
            {"caster", pName},
            {"spell", getName()},
            {"duration", (double)getCastTime()}
        };
        SocketHandlers::broadcastToMap(pMap, startMsg.dump());
    }

    void onCastComplete(Player& player, const std::string& targetId, const Vector3& targetPos) const override {
        std::string pName, pMap, pUsername;
        int pLevel = 1;
        {
            std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
            pName = player.charName;
            pMap = player.mapName;
            pUsername = player.username;
            pLevel = player.level;
        }

        int damage = 0;
        bool isCrit = false;
        std::string mobName = "Unknown";
        std::string mobType = "Normal";
        int mobLevel = 0;
        bool mobDied = false;
        int xpReward = 0;
        
        std::string mobTypeId = "";
        {
            std::lock_guard<std::recursive_mutex> lock(GameState::getInstance().getMtx());
            auto& mobs = GameState::getInstance().getMobs();
            
            Mob* targetMob = nullptr;
            for (auto& m : mobs) {
                if (m.id == targetId && m.mapName == pMap && m.hp > 0) {
                    targetMob = &m;
                    break;
                }
            }

            if (!targetMob) {
                Logger::log("[Ability] Frostblitz target not found or dead: " + targetId);
                goto interrupted_label;
            }

            mobName = targetMob->name;
            mobType = targetMob->mobType;
            mobLevel = targetMob->level;
            mobTypeId = targetMob->typeId;

            Logger::log("[Ability] Frostblitz cast complete by " + pName + " @ " + mobName + "(" + targetId + ") [" + mobType + "] on " + pMap);
            
            auto result = GameLogic::getSpellDamage(pLevel, 20, 40);
            damage = result.damage;
            isCrit = result.isCrit;
            targetMob->hp = std::max(0, targetMob->hp - damage);
            targetMob->target = pUsername;
            mobName = targetMob->name;
            mobLevel = targetMob->level;

            Logger::log("[Ability] Frostblitz damage applied: " + std::to_string(damage) + " to " + mobName);

            // Apply debuff
            bool found = false;
            long long endTime = currentTimeMillis() + 8000;
            for (auto& d : targetMob->debuffs) {
                if (d.type == "Chill") {
                    d.endTime = std::max(d.endTime, endTime);
                    found = true;
                    break;
                }
            }
            if (!found) {
                targetMob->debuffs.push_back({"Chill", endTime});
            }

            if (targetMob->hp <= 0) {
                mobDied = true;
                targetMob->respawnAt = currentTimeMillis() + (GameState::getInstance().getRespawnRate(pMap) * 1000);
                xpReward = GameLogic::getMobXPReward(mobLevel, targetMob->dbXp);
                Logger::log("[Ability] Frostblitz target died: " + mobName);
            }
        } // Lock released
        Logger::log("[Ability] Frostblitz lock released for " + pName);

        // Broadcast combat text
        {
            Logger::log("[Ability] Frostblitz broadcasting combat text for " + pName);
            json ctMsg = {
                {"type", "combat_text"},
                {"target_id", targetId},
                {"value", damage},
                {"is_crit", isCrit},
                {"color", isCrit ? "#FFD700" : "#FFFF00"}
            };
            SocketHandlers::broadcastToMap(pMap, ctMsg.dump());

            std::string msg = pName + " trifft " + mobName + " für " + std::to_string(damage) + " Schaden!";
            if (isCrit) msg += " (KRITISCH!)";
            json chatCombatMsg = {
                {"type", "chat_receive"},
                {"mode", "combat"},
                {"message", msg}
            };
            SocketHandlers::broadcastToMap(pMap, chatCombatMsg.dump());
        }

        if (mobDied) {
            Logger::log("[Ability] Frostblitz awarding XP for " + pName);
            GameLogic::awardXP(player, xpReward);
            GameLogic::checkQuestKill(player, mobTypeId);
        }

        {
            json finishMsg = {
                {"type", "spell_cast_finish"},
                {"caster", pName},
                {"target_id", targetId},
                {"spell", getName()}
            };
            SocketHandlers::broadcastToMap(pMap, finishMsg.dump());
        }
        Logger::log("[Ability] Frostblitz processing finished for " + pName);
        return;

interrupted_label:
        onCastInterrupted(player);
    }

    void onCastInterrupted(Player& player) const override {
        std::string pName, pMap;
        {
            std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
            pName = player.charName;
            pMap = player.mapName;
        }
        json cancelMsg = {
            {"type", "spell_cast_finish"},
            {"caster", pName},
            {"spell", "interrupted"}
        };
        SocketHandlers::broadcastToMap(pMap, cancelMsg.dump());
    }
};

#endif
