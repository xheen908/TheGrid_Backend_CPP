#ifndef ABILITYMANAGER_HPP
#define ABILITYMANAGER_HPP

#include <map>
#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "Ability.hpp"

using json = nlohmann::json;
#include "Frostblitz.hpp"
#include "IceBarrier.hpp"
#include "FrostNova.hpp"
#include "ConeOfCold.hpp"
#include "Blizzard.hpp"

class AbilityManager {
public:
    static AbilityManager& getInstance() {
        static AbilityManager instance;
        return instance;
    }

    void registerAbility(std::unique_ptr<Ability> ability) {
        abilities[ability->getName()] = std::move(ability);
    }

    const Ability* getAbility(const std::string& name) const {
        auto it = abilities.find(name);
        if (it != abilities.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    json getAbilitiesJson() const {
        json list = json::array();
        for (auto const& [name, ability] : abilities) {
            list.push_back({
                {"name", ability->getName()},
                {"category", ability->getCategory()},
                {"description", ability->getDescription()},
                {"icon", ability->getIcon()},
                {"cast_time", ability->getCastTime()},
                {"cooldown", ability->getCooldown()},
                {"targeted", ability->isTargeted()}
            });
        }
        return list;
    }

    void startCasting(Player& player, const std::string& spellName, const std::string& targetId, const Vector3& targetPos) {
        bool wasCasting = false;
        std::string currentSpell = "";
        {
            std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
            if (player.isCasting) {
                wasCasting = true;
                currentSpell = player.currentSpell;
            }
        }

        if (wasCasting) {
            // Special Case: Blizzard allows self-interruption (spamming to move reticle/refresh)
            if (currentSpell == "Blizzard") {
                interrupt(player);
            } else {
                // For other spells (Frostblitz), prevent cancelling yourself by spamming the same key
                if (currentSpell == spellName) {
                    return; // Ignore request
                } else {
                    // Changing spell? Allow interrupt.
                    interrupt(player);
                }
            }
        }

        const Ability* ability = getAbility(spellName);
        if (!ability || !ability->canCast(player, targetId)) {
            return;
        }

        {
            std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
            if (player.isMoving && !ability->canCastWhileMoving()) return;
        }

        if (ability->getCastTime() > 0) {
            std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
            player.isCasting = true;
            player.currentSpell = spellName;
            player.currentTargetId = targetId;
            player.currentTargetPos = targetPos; // MUST ADD THIS TO PLAYER STRUCT
            long long now = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            player.castEnd = now + (long long)(ability->getCastTime() * 1000);
            player.lastCastTick = now;
            
            ability->onCastStart(player, targetId, targetPos);
        } else {
            ability->onCastComplete(player, targetId, targetPos);
        }
    }

    void update(Player& player) {
        std::string spellToComplete;
        std::string targetToComplete;
        Vector3 posToComplete;
        bool shouldComplete = false;

        {
            std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
            if (!player.isCasting) return;

            long long now = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();

            // Periodic tick every 1000ms
            if (now >= player.lastCastTick + 1000) {
                const Ability* ability = getAbility(player.currentSpell);
                if (ability) {
                    ability->onCastTick(player, player.currentTargetId, player.currentTargetPos);
                }
                player.lastCastTick = now;
            }

            if (now >= player.castEnd) {
                spellToComplete = player.currentSpell;
                targetToComplete = player.currentTargetId;
                posToComplete = player.currentTargetPos;
                shouldComplete = true;
            }
        }

        if (shouldComplete) {
            const Ability* ability = getAbility(spellToComplete);
            if (ability) {
                ability->onCastComplete(player, targetToComplete, posToComplete);
            }
            std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
            player.isCasting = false;
            player.currentSpell = "";
        }
    }

    void interrupt(Player& player) {
        std::string spellToInterrupt;
        {
            std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
            if (!player.isCasting) return;
            spellToInterrupt = player.currentSpell;
            player.isCasting = false;
            player.currentSpell = "";
            Logger::log("[Ability] Player " + player.username + " cast interrupted.");
        }

        const Ability* ability = getAbility(spellToInterrupt);
        if (ability) {
            ability->onCastInterrupted(player);
        }
    }

private:
    AbilityManager() {
        registerAbility(std::make_unique<Frostblitz>());
        registerAbility(std::make_unique<IceBarrier>());
        registerAbility(std::make_unique<FrostNova>());
        registerAbility(std::make_unique<ConeOfCold>());
        registerAbility(std::make_unique<Blizzard>());
    }

    std::map<std::string, std::unique_ptr<Ability>> abilities;
};

#endif
