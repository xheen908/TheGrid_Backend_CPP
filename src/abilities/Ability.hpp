#ifndef ABILITY_HPP
#define ABILITY_HPP

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include "../GameState.hpp"

class Ability {
public:
    virtual ~Ability() = default;

    virtual std::string getName() const = 0;
    virtual float getCastTime() const = 0; // in seconds
    virtual float getCooldown() const = 0; // in seconds
    virtual bool isTargeted() const = 0;
    virtual bool ignoresGCD() const { return false; }
    virtual bool canCastWhileMoving() const { return false; }

    virtual bool canCast(const Player& player, const std::string& targetId) const {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        std::lock_guard<std::recursive_mutex> lock(player.pMtx);
        // GCD Check
        if (!ignoresGCD() && now < player.gcdUntil) return false;

        // Cooldown Check
        auto it = player.cooldowns.find(getName());
        if (it != player.cooldowns.end() && now < it->second) return false;

        return true;
    }

    virtual void onCastStart(Player& player, const std::string& targetId) const = 0;
    virtual void onCastComplete(Player& player, const std::string& targetId) const = 0;
    virtual void onCastInterrupted(Player& player) const = 0;

protected:
    static long long currentTimeMillis() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

#endif
