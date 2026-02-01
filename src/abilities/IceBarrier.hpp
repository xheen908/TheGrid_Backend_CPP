#ifndef ICEBARRIER_HPP
#define ICEBARRIER_HPP

#include "Ability.hpp"
#include "../SocketHandlers.hpp"

class IceBarrier : public Ability {
public:
    std::string getName() const override { return "Eisbarriere"; }
    float getCastTime() const override { return 0.0f; }
    float getCooldown() const override { return 30.0f; }
    bool isTargeted() const override { return false; }
    bool canCastWhileMoving() const override { return true; }

    void onCastStart(Player& player, const std::string& targetId) const override {}

    void onCastComplete(Player& player, const std::string& targetId) const override {
        std::string pName, pMap, pUsername;
        {
            std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
            player.shield = 2666;
            player.cooldowns[getName()] = currentTimeMillis() + (long long)(getCooldown() * 1000);
            player.gcdUntil = currentTimeMillis() + 1500;

            // Add buff
            long long endTime = currentTimeMillis() + 30000;
            bool found = false;
            for (auto& b : player.buffs) {
                if (b.type == "Eisbarriere") {
                    b.endTime = std::max(b.endTime, endTime);
                    found = true;
                    break;
                }
            }
            if (!found) {
                player.buffs.push_back({"Eisbarriere", endTime});
            }
            
            pName = player.charName;
            pMap = player.mapName;
            pUsername = player.username;
        }

        Logger::log("[Ability] Ice Barrier cast by " + pName);

        // Notify client
        json finishMsg = {
            {"type", "spell_cast_finish"},
            {"caster", pName},
            {"target_id", ""},
            {"spell", getName()}
        };
        SocketHandlers::broadcastToMap(pMap, finishMsg.dump());
        
        // Final status sync via safe method
        // Note: awardXP also does a status sync, but here we just manually trigger one
        // by sending a chat message or similar, but the periodic sync in Server.cpp will handle it anyway.
        // Let's send a system message.
        json sysMsg = {{"type", "chat_receive"}, {"mode", "system"}, {"message", "Eisbarriere aktiviert!"}};
        SocketHandlers::sendToPlayer(pUsername, sysMsg.dump());
    }

    void onCastInterrupted(Player& player) const override {}
};

#endif
