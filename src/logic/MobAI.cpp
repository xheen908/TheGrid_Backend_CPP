#include "MobAI.hpp"
#include "../GameState.hpp"
#include "../GameLogic.hpp"
#include "../SocketHandlers.hpp"
#include "../Logger.hpp"
#include "../abilities/AbilityManager.hpp"
#include <cmath>
#include <algorithm>
#include <thread>
#include <vector>
#include <atomic>

namespace MobAI {
    void updateMob(Mob& m, long long now, const std::vector<std::shared_ptr<Player>>& players) {
        if (m.hp <= 0) return;

        // Debuff-Check Frequenz reduzieren
        float speedMultiplier = 1.0f;
        bool isRooted = false;
        for (const auto& d : m.debuffs) {
            if (d.type == "Chill") speedMultiplier *= 0.5f;
            if (d.type == "Frozen") isRooted = true;
        }

        // 1. Aggro / Target Selection (Staggered: different mobs check at different times)
        int aggroOffset = 0;
        try { aggroOffset = std::stoi(m.id) % 10 * 50; } catch(...) {}

        if (m.target.empty()) {
            if (now < m.lastAggroCheck + 500 + aggroOffset) return;
            m.lastAggroCheck = now;

            float aggroRangeSq = 25.0f * 25.0f;
            std::shared_ptr<Player> nearest = nullptr;
            float minDSq = aggroRangeSq;

            for (auto const& p : players) {
                if (p->mapName != m.mapName || p->isDisconnected || p->isInvisible || p->hp <= 0) continue;
                
                float dx = m.transform.x - p->lastPos.x;
                float dz = m.transform.z - p->lastPos.z;
                float distSq = dx*dx + dz*dz;
                
                if (distSq < minDSq) {
                    minDSq = distSq;
                    nearest = p;
                }
            }

            if (nearest) {
                m.target = nearest->username;
                Logger::log("[MobAI] Mob " + m.name + " (" + m.id + ") aggroed on " + m.target);
            }
        }


        // 2. Chasing / Combat
        if (!m.target.empty()) {
            auto targetP = GameState::getInstance().getPlayer(m.target);
            if (!targetP || targetP->mapName != m.mapName || targetP->hp <= 0 || targetP->isDisconnected || targetP->isInvisible) {
                m.target = "";
                return;
            }

            float dx = targetP->lastPos.x - m.transform.x;
            float dz = targetP->lastPos.z - m.transform.z;
            float distSq = dx*dx + dz*dz;
            
            // Leashing (Squared distance)
            float hx = m.transform.x - m.home.x;
            float hz = m.transform.z - m.home.z;
            if (hx*hx + hz*hz > 60.0f * 60.0f) {
                m.target = "";
                return;
            }

            if (distSq > 2.5f * 2.5f) { // Chasing
                if (!isRooted) {
                    float speed = 0.4f * speedMultiplier; 
                    float angle = std::atan2(dx, dz);
                    m.transform.x += std::sin(angle) * speed;
                    m.transform.z += std::cos(angle) * speed;
                    m.rotationY = angle;
                }
            } else { // In Attack Range
                m.rotationY = std::atan2(dx, dz);

                if (now - m.lastAttack >= 1500) { 
                    m.lastAttack = now;
                    int dmg = GameLogic::getMobDamage(m.level);
                    
                    {
                        std::lock_guard<std::recursive_mutex> pLock(targetP->pMtx);
                        targetP->hp = std::max(0, targetP->hp - dmg);
                        targetP->lastStatusSync = 0; 
                        AbilityManager::getInstance().interrupt(*targetP);

                        json ctMsg = {{"type", "combat_text"}, {"target_id", "player"}, {"value", dmg}, {"is_crit", false}, {"color", "#FF0000"}};
                        SocketHandlers::sendToPlayer(targetP->username, ctMsg.dump());

                        json attackMsg = {{"type", "mob_attack"}, {"mob_id", m.id}, {"target_id", targetP->charName}};
                        SocketHandlers::broadcastToMap(m.mapName, attackMsg.dump());
                    }
                }
            }
        } else {
            // 3. Return to Home
            float hx = m.home.x - m.transform.x;
            float hz = m.home.z - m.transform.z;
            if (hx*hx + hz*hz > 1.0f) {
                float speed = 0.2f * speedMultiplier;
                float angle = std::atan2(hx, hz);
                m.transform.x += std::sin(angle) * speed;
                m.transform.z += std::cos(angle) * speed;
                m.rotationY = angle;
            }
        }
    }

    // Thread management implementation (simple partition)
    void update(long long now) {
        auto players = GameState::getInstance().getPlayersSnapshot();
        std::lock_guard<std::recursive_mutex> lock(GameState::getInstance().getMtx());
        auto& mobs = GameState::getInstance().getMobs();
        
        // Split mobs into chunks if we wanted true multi-threading here
        // For now, let's keep it robust and sequential but prepared for "thread management"
        for (auto& m : mobs) {
            updateMob(m, now, players);
        }
    }

    void startWorkerThreads() {
        // Implementation for advanced thread management if needed
        Logger::log("[MobAI] AI Worker logic initialized.");
    }

    void stopWorkerThreads() {
        // Cleanup if needed
    }
}
