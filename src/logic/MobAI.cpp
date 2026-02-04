#include "MobAI.hpp"
#include "../GameState.hpp"
#include "../GameLogic.hpp"
#include "../SocketHandlers.hpp"
#include "../Logger.hpp"
#include <cmath>
#include <algorithm>
#include <thread>
#include <vector>
#include <atomic>

namespace MobAI {
    void updateMob(Mob& m, long long now, const std::vector<std::shared_ptr<Player>>& players) {
        if (m.hp <= 0) return;

        // 0. Debuff Management & Speed Modifiers
        float speedMultiplier = 1.0f;
        bool isRooted = false;

        m.debuffs.erase(std::remove_if(m.debuffs.begin(), m.debuffs.end(),
            [now](const Debuff& d) { return now >= d.endTime; }),
            m.debuffs.end());

        for (const auto& d : m.debuffs) {
            if (d.type == "Chill") speedMultiplier *= 0.5f;
            if (d.type == "Frozen") isRooted = true;
        }

        // 1. Aggro / Target Selection
        if (m.target.empty()) {
            float aggroRange = 25.0f;
            std::shared_ptr<Player> nearest = nullptr;
            float minDist = aggroRange;

            int playersOnMap = 0;
            for (auto const& p : players) {
                if (p->mapName == m.mapName) playersOnMap++;
                if (p->mapName != m.mapName || p->isDisconnected || p->isInvisible || p->hp <= 0) continue;
                
                float dist = std::sqrt(std::pow(m.transform.x - p->lastPos.x, 2) + 
                                      std::pow(m.transform.z - p->lastPos.z, 2));
                if (dist < minDist) {
                    minDist = dist;
                    nearest = p;
                }
            }

            if (nearest) {
                m.target = nearest->username;
                Logger::log("[MobAI] Mob " + m.name + " (" + m.id + ") aggroed on " + m.target + " (dist: " + std::to_string(minDist) + ")");
            } else if (now % 5000 < 100 && playersOnMap > 0) { // Log every ~5s if players are around but not aggroed
                Logger::log("[MobAI] Mob " + m.name + " (" + m.id + ") idle. Players on map: " + std::to_string(playersOnMap) + ", nearest dist: " + std::to_string(minDist));
            }
        }

        // 2. Chasing / Combat
        if (!m.target.empty()) {
            auto targetP = GameState::getInstance().getPlayer(m.target);
            if (!targetP || targetP->mapName != m.mapName || targetP->hp <= 0 || targetP->isDisconnected || targetP->isInvisible) {
                Logger::log("[MobAI] Mob " + m.name + " (" + m.id + ") lost target " + m.target);
                m.target = "";
                return;
            }

            float dist = std::sqrt(std::pow(m.transform.x - targetP->lastPos.x, 2) + 
                                  std::pow(m.transform.z - targetP->lastPos.z, 2));
            
            // Leashing (Max distance from home)
            float distFromHome = std::sqrt(std::pow(m.transform.x - m.home.x, 2) + 
                                          std::pow(m.transform.z - m.home.z, 2));
            if (distFromHome > 60.0f) {
                Logger::log("[MobAI] Mob " + m.name + " (" + m.id + ") leashed (dist from home: " + std::to_string(distFromHome) + ")");
                m.target = ""; // Reset target and return home
                return;
            }

            if (dist > 2.5f) { // Chasing
                if (!isRooted) {
                    float speed = 0.4f * speedMultiplier; 
                    float dx = targetP->lastPos.x - m.transform.x;
                    float dz = targetP->lastPos.z - m.transform.z;
                    float angle = std::atan2(dx, dz);
                    m.transform.x += std::sin(angle) * speed;
                    m.transform.z += std::cos(angle) * speed;
                    m.rotation = angle;
                }
            } else { // In Attack Range
                // Always face target when in range
                m.rotation = std::atan2(targetP->lastPos.x - m.transform.x, targetP->lastPos.z - m.transform.z);

                if (now - m.lastAttack >= 1500) { // Attack speed 1.5s
                    m.lastAttack = now;
                    int dmg = GameLogic::getMobDamage(m.level);
                    
                    {
                        std::lock_guard<std::recursive_mutex> pLock(targetP->pMtx);
                        targetP->hp = std::max(0, targetP->hp - dmg);
                        targetP->lastStatusSync = 0; // Force immediate status sync on next tick

                        // 1. Send Combat Text to hit player
                        json ctMsg = {
                            {"type", "combat_text"},
                            {"target_id", "player"},
                            {"value", dmg},
                            {"is_crit", false},
                            {"color", "#FF0000"}
                        };
                        SocketHandlers::sendToPlayer(targetP->username, ctMsg.dump());

                        // 2. Broadcast Attack Animation to everyone
                        json attackMsg = {
                            {"type", "mob_attack"},
                            {"mob_id", m.id},
                            {"target_id", targetP->charName}
                        };
                        SocketHandlers::broadcastToMap(m.mapName, attackMsg.dump());
                    }
                    Logger::log("[Combat] Mob " + m.name + " (" + m.id + ") hit " + targetP->charName + " for " + std::to_string(dmg));
                }
            }
        } else {
            // 3. Return to Home / Idle Wander
            float distFromHome = std::sqrt(std::pow(m.transform.x - m.home.x, 2) + 
                                          std::pow(m.transform.z - m.home.z, 2));
            if (distFromHome > 1.0f) {
                float speed = 0.2f * speedMultiplier;
                float dx = m.home.x - m.transform.x;
                float dz = m.home.z - m.transform.z;
                float angle = std::atan2(dx, dz);
                m.transform.x += std::sin(angle) * speed;
                m.transform.z += std::cos(angle) * speed;
                m.rotation = angle;
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
