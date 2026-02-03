#include "Server.hpp"
#include "GameLogic.hpp"
#include "Database.hpp"
#include "SocketHandlers.hpp"
#include "ChatHandlers.hpp"
#include "GMCommands.hpp"
#include "Logger.hpp"
#include "abilities/AbilityManager.hpp"
#include <iostream>
#include <chrono>
#include <cmath>
#include <thread>
#include <algorithm>
#include <functional>

static uWS::Loop* worldLoop = nullptr;

static long long currentTimeMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

// ==========================================
// WORLD SERVER IMPLEMENTATION
// ==========================================
WorldServer::WorldServer() : running(false) {}

void WorldServer::start(int port) {
    running = true;
    uWS::App app;
    worldLoop = (uWS::Loop*)uWS::Loop::get();

    app.ws<PerSocketData>("/ws", {
        .compression = uWS::SHARED_COMPRESSOR,
        .maxPayloadLength = 16 * 1024 * 1024,
        .idleTimeout = 60,
        .open = [](auto *ws) {
            Logger::log("[WS] Client connected to WorldServer.");
        },
        .message = [this](auto *ws, std::string_view message, uWS::OpCode opCode) {
            try {
                auto j = json::parse(message);
                std::string type = j.value("type", "");
                auto data = ws->getUserData();

                if (type == "authenticate") {
                    std::string token = j.value("token", "");
                    int characterId = j.value("character_id", -1);
                    std::string username = token.starts_with("session_") ? token.substr(8) : "UnknownPlayer";
                    
                    data->username = username;
                    data->isAuthenticated = true;

                    auto player = std::make_shared<Player>();
                    if (Database::getInstance().loadCharacter(characterId, *player)) {
                        data->charName = player->charName;
                        data->isGM = player->isGM;
                    } else {
                        // Fallback
                        player->charName = username;
                        player->username = username;
                        player->level = 1;
                        player->mapName = "WorldMap0";
                        player->lastPos = {0, 0, 0};
                        data->charName = username;
                    }
                    
                    player->ws = ws;
                    GameState::getInstance().addPlayer(data->username, player);

                    data->mapName = player->mapName; 

                    ws->send(json{
                        {"type", "authenticated"}, 
                        {"char_name", player->charName},
                        {"char_class", player->characterClass},
                        {"level", player->level},
                        {"xp", player->xp},
                        {"max_xp", GameLogic::getLevelData(player->level).xpToNextLevel},
                        {"map_name", player->mapName},
                        {"is_gm", player->isGM},
                        {"position", {{"x", player->lastPos.x}, {"y", player->lastPos.y}, {"z", player->lastPos.z}}}
                    }.dump(), uWS::OpCode::TEXT);

                    // Send game objects for current map
                    auto objects = GameState::getInstance().getGameObjects(player->mapName);
                    json objectsMsg = {{"type", "game_objects_init"}, {"objects", json::array()}};
                    for (auto const& obj : objects) {
                        objectsMsg["objects"].push_back({
                            {"id", obj.id},
                            {"type", obj.type},
                            {"position", {{"x", obj.position.x}, {"y", obj.position.y}, {"z", obj.position.z}}},
                            {"rotation", {{"x", obj.rotation.x}, {"y", obj.rotation.y}, {"z", obj.rotation.z}}},
                            {"extra_data", obj.extraData}
                        });
                    }
                    ws->send(objectsMsg.dump(), uWS::OpCode::TEXT);
                    
                    Logger::log("[WS] User Authenticated & Loaded: " + data->username + " on " + player->mapName);
                } else if (data->isAuthenticated) {
                    auto player = GameState::getInstance().getPlayer(data->username);
                    if (!player) return;

                    if (type == "player_update") {
                        bool moved = false;
                        {
                            std::lock_guard<std::recursive_mutex> pLock(player->pMtx);
                            if (j.contains("position")) {
                                auto pos = j["position"];
                                float nx = pos.value("x", 0.0f);
                                float ny = pos.value("y", 0.0f);
                                float nz = pos.value("z", 0.0f);
                                
                                float distSq = std::pow(player->lastPos.x - nx, 2) + std::pow(player->lastPos.y - ny, 2) + std::pow(player->lastPos.z - nz, 2);
                                if (distSq > 0.001f) moved = true;
                                
                                player->lastPos = {nx, ny, nz};
                                player->isMoving = moved;
                            }
                            if (j.contains("rotation")) {
                                auto rot = j["rotation"];
                                player->rotation = {rot.value("x", 0.0f), rot.value("y", 0.0f), rot.value("z", 0.0f)};
                            }
                            
                            // Interrupt casting if moved and spell doesn't allow it
                            if (moved && player->isCasting) {
                                auto ability = AbilityManager::getInstance().getAbility(player->currentSpell);
                                if (ability && !ability->canCastWhileMoving()) {
                                    // We need to interrupt, but interrupt uses pLock too.
                                    // So we'll call a special interrupt that assumes lock is held or defer it.
                                }
                            }
                        }
                        
                        if (moved) {
                             auto ability = AbilityManager::getInstance().getAbility(player->currentSpell);
                             if (ability && !ability->canCastWhileMoving()) {
                                 AbilityManager::getInstance().interrupt(*player);
                             }
                        }

                        json moveMsg = {{"type", "player_moved"}, {"username", player->charName}, {"char_class", player->characterClass}, {"position", j.value("position", json::object())}, {"rotation", j.value("rotation", json::object())}};
                        SocketHandlers::broadcastToMap(player->mapName, moveMsg.dump(), ws);
                    } else if (type == "cast_spell") {
                        SocketHandlers::handleSpellCast(ws, j);
                    } else if (type == "chat_message") {
                        ChatHandlers::handleChatMessage(ws, j);
                    } else if (type == "target_update") {
                        std::lock_guard<std::recursive_mutex> pLock(player->pMtx);
                        player->currentTargetId = j.value("target_id", "");
                    } else if (type == "logout_request") {
                        std::lock_guard<std::recursive_mutex> pLock(player->pMtx);
                        player->logoutTimer = currentTimeMillis() + 10000; // 10 seconds logout timer
                        json s = {{"type", "logout_timer_started"}, {"seconds", 10}};
                        ws->send(s.dump(), uWS::OpCode::TEXT);
                        Logger::log("[WS] Logout timer started for " + data->username);
                    } else if (type == "map_change_request") {
                        SocketHandlers::handleMapChange(ws, j);
                    } else if (type == "party_invite") {
                        SocketHandlers::handlePartyInvite(ws, j);
                    } else if (type == "party_invite_response") {
                        SocketHandlers::handlePartyResponse(ws, j);
                    } else if (type == "party_leave") {
                        SocketHandlers::handlePartyLeave(ws, j);
                    } else if (type == "party_kick") {
                        SocketHandlers::handlePartyKick(ws, j);
                    }
                }
            } catch (const std::exception& e) {
                Logger::log("[WS] Error: " + std::string(e.what()));
            }
        },
        .close = [](auto *ws, int code, std::string_view message) {
            auto data = ws->getUserData();
            if (data->isAuthenticated) {
                auto player = GameState::getInstance().getPlayer(data->username);
                if (player) {
                    {
                        std::lock_guard<std::recursive_mutex> pLock(player->pMtx);
                        player->isDisconnected = true;
                        player->ws = nullptr; // CRITICAL: Stop using this pointer immediately
                    }
                    // Save to DB before removal
                    Database::getInstance().savePlayer(*player);
                }
                GameState::getInstance().removePlayer(data->username);
                Logger::log("[WS] Player disconnected & saved: " + data->username);
            }
        }
    });

    app.listen(port, [port](auto *listen_socket) {
        if (listen_socket) {
            std::cout << "World Server listening on port " << port << std::endl;
        }
    });

    // Game Loop Thread
    std::thread tickThread([this]() {
        while (this->running) {
            this->tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 10Hz
        }
    });

    app.run();
    running = false;
    if (tickThread.joinable()) tickThread.join();
}

void WorldServer::tick() {
    auto nowMs = currentTimeMillis();
    auto players = GameState::getInstance().getPlayersSnapshot();

    // --- Collect Map Stats for Scaling ---
    std::map<std::string, float> mapMaxLevels; 
    std::map<std::string, int> mapMaxPartySize;

    for (auto const& p : players) {
        if (p->isDisconnected) continue;
        
        std::string currentMap;
        int pLevel;
        int pSize = 1;
        {
            std::lock_guard<std::recursive_mutex> pLock(p->pMtx);
            currentMap = p->mapName;
            pLevel = p->level;
            if (!p->partyId.empty()) {
                auto party = GameState::getInstance().getParty(p->partyId);
                if (party) pSize = (int)party->members.size();
            }
        }
        
        if (mapMaxLevels.find(currentMap) == mapMaxLevels.end()) {
            mapMaxLevels[currentMap] = (float)pLevel;
        } else {
            mapMaxLevels[currentMap] = std::max(mapMaxLevels[currentMap], (float)pLevel);
        }
        mapMaxPartySize[currentMap] = std::max(mapMaxPartySize[currentMap], pSize);
    }
    
    // --- Mob Logic ---
    {
        std::lock_guard<std::recursive_mutex> mobLock(GameState::getInstance().getMtx());
        auto& mobs = GameState::getInstance().getMobs();
        for (auto& m : mobs) {
            // Dynamic Scaling
            GameLogic::scaleMobToMap(m, mapMaxLevels, mapMaxPartySize);

            if (m.hp <= 0) {
                if (m.respawnAt > 0 && nowMs > m.respawnAt) { 
                    m.hp = m.maxHp; 
                    m.respawnAt = 0; 
                    m.target = ""; 
                    m.transform = m.home;
                }
                else if (m.respawnAt == 0) m.respawnAt = nowMs + 30000;
                continue;
            }

            // --- Aggro Logic: Search for nearby players if no target ---
            if (m.target.empty()) {
                float closestDistSq = 400.0f; // Aggro range: 20 units
                std::string bestTarget = "";
                
                for (auto const& p : players) {
                    if (p->isDisconnected || p->isInvisible || p->mapName != m.mapName) continue;
                    float dSq = std::pow(m.transform.x - p->lastPos.x, 2) + std::pow(m.transform.z - p->lastPos.z, 2);
                    if (dSq < closestDistSq) {
                        closestDistSq = dSq;
                        bestTarget = p->username;
                    }
                }
                if (!bestTarget.empty()) {
                    m.target = bestTarget;
                    Logger::log("[AI] Mob " + m.id + " aggroed on " + m.target);
                }
            }

            // --- Debuff Checks ---
            bool isFrozen = false;
            bool isChilled = false;
            for (const auto& d : m.debuffs) {
                if (d.type == "Frozen") isFrozen = true;
                if (d.type == "Chill") isChilled = true;
            }

            // --- Chase & Attack Logic ---
            if (!m.target.empty() && !isFrozen) {
                auto pTarget = GameState::getInstance().getPlayer(m.target);
                if (pTarget && !pTarget->isDisconnected && pTarget->mapName == m.mapName) {
                    float dx = pTarget->lastPos.x - m.transform.x;
                    float dz = pTarget->lastPos.z - m.transform.z;
                    float distSq = dx * dx + dz * dz;

                    // Leash: If too far from home, reset
                    float distFromHomeSq = std::pow(m.transform.x - m.home.x, 2) + std::pow(m.transform.z - m.home.z, 2);
                    if (distFromHomeSq > 2500.0f) { // 50 units leash
                        m.target = "";
                        Logger::log("[AI] Mob " + m.id + " leashed.");
                    } else if (distSq < 9.0f) { // Attack range: 3 units
                        if (nowMs - m.lastAttack > 2000) {
                            m.lastAttack = nowMs;
                            int dmg = GameLogic::getMobDamage(m.level);
                            {
                                std::lock_guard<std::recursive_mutex> pLock(pTarget->pMtx);
                                pTarget->hp = std::max(0, pTarget->hp - dmg);
                            }
                            json hitMsg = {{"type", "combat_text"}, {"target_id", "player"}, {"value", dmg}, {"color", "#FF2222"}};
                            SocketHandlers::sendToPlayer(pTarget->username, hitMsg.dump());
                        }
                    } else {
                        // Chase: Move towards target (0.4 units per tick approx 4m/s)
                        float dist = std::sqrt(distSq);
                        float step = isChilled ? 0.15f : 0.4f; // Chill reduces speed significantly
                        m.transform.x += (dx / dist) * step;
                        m.transform.z += (dz / dist) * step;
                        
                        // Update rotation to face player
                        m.rotation = std::atan2(dx, dz);
                    }
                } else {
                    m.target = "";
                }
            }

            // Simple AI: Return home if no target and not at home
            if (m.target.empty() && !isFrozen) {
                float dx = m.home.x - m.transform.x;
                float dz = m.home.z - m.transform.z;
                float distSq = dx * dx + dz * dz;
                if (distSq > 1.0f) {
                    float dist = std::sqrt(distSq);
                    float step = isChilled ? 0.1f : 0.3f;
                    m.transform.x += (dx / dist) * step;
                    m.transform.z += (dz / dist) * step;
                    m.rotation = std::atan2(dx, dz);
                } else {
                    m.rotation += 0.05f; // Idle spin
                }
            }

            // --- Debuff Cleanup ---
            m.debuffs.erase(std::remove_if(m.debuffs.begin(), m.debuffs.end(),
                [nowMs](const Debuff& d) { return nowMs >= d.endTime; }),
                m.debuffs.end());
        }
    }

    // --- Player Periodic Sync ---
    for (auto const& p : players) {
        if (p->isDisconnected) continue;
        
        AbilityManager::getInstance().update(*p);

            // Periodic Status Sync
            {
                std::lock_guard<std::recursive_mutex> pLock(p->pMtx);
                
                // Logout Check
                if (p->logoutTimer > 0 && nowMs >= p->logoutTimer) {
                    p->logoutTimer = 0;
                    json lc = {{"type", "logout_complete"}};
                    SocketHandlers::sendToPlayer(p->username, lc.dump());
                    Logger::log("[WS] Logout complete for " + p->username);
                    
                    // CRITICAL: Save player stats to DB when logout timer completes
                    Database::getInstance().savePlayer(*p);
                }

                // --- Buff Cleanup ---
                bool hadIceBarrier = false;
                for (const auto& b : p->buffs) if (b.type == "Eisbarriere") hadIceBarrier = true;

                p->buffs.erase(std::remove_if(p->buffs.begin(), p->buffs.end(),
                    [nowMs](const Debuff& b) { return nowMs >= b.endTime; }),
                    p->buffs.end());

                bool hasIceBarrier = false;
                for (const auto& b : p->buffs) if (b.type == "Eisbarriere") hasIceBarrier = true;

                if (hadIceBarrier && !hasIceBarrier) {
                    p->shield = 0;
                    Logger::log("[WS] Ice Barrier expired for " + p->username);
                }

                if (nowMs - p->lastStatusSync > 1000) {
                    p->lastStatusSync = nowMs;
                    json buffList = json::array();
                    for (auto const& b : p->buffs) {
                        buffList.push_back({{"type", b.type}, {"remaining", (int)((b.endTime - nowMs) / 1000)}});
                    }
                    json s = {
                        {"type", "player_status"}, 
                        {"username", p->charName}, 
                        {"char_class", p->characterClass}, 
                        {"hp", p->hp}, 
                        {"max_hp", p->maxHp}, 
                        {"level", p->level}, 
                        {"xp", p->xp},
                        {"max_xp", GameLogic::getLevelData(p->level).xpToNextLevel},
                        {"shield", p->shield}, 
                        {"buffs", buffList},
                        {"gravity_enabled", p->gravityEnabled},
                        {"speed_multiplier", p->speedMultiplier},
                        {"is_gm", p->isGMFlagged}
                    };
                    SocketHandlers::broadcastToMap(p->mapName, s.dump());
                }
            }

            // Mob Sync (within map)
            json mobList = json::array();
            {
                std::lock_guard<std::recursive_mutex> mobLock(GameState::getInstance().getMtx());
                for (auto const& m : GameState::getInstance().getMobs()) {
                    if (m.mapName == p->mapName) {
                        // Sync if alive OR died recently (within 5 seconds)
                        bool recentlyDied = (m.hp <= 0 && m.respawnAt > 0 && (nowMs < (m.respawnAt - (GameState::getInstance().getRespawnRate(m.mapName) - 5) * 1000)));
                        
                        if (m.hp > 0 || recentlyDied) {
                            json dList = json::array();
                            for (auto const& db : m.debuffs) {
                                dList.push_back({{"type", db.type}, {"remaining", (int)((db.endTime - nowMs) / 1000)}});
                            }
                            mobList.push_back({{"id", m.id}, {"name", m.name}, {"hp", m.hp}, {"maxHp", m.maxHp}, {"debuffs", dList}, {"transform", {{"x", m.transform.x}, {"y", m.transform.y}, {"z", m.transform.z}, {"rot", m.rotation}}}});
                        }
                    }
                }
            }
            if (!mobList.empty()) {
                SocketHandlers::sendToPlayer(p->username, json{{"type", "mob_sync"}, {"mobs", mobList}}.dump());
            }
    }
}

void WorldServer::stop() { running = false; }

void WorldServer::defer(std::function<void()> cb) {
    if (worldLoop) {
        worldLoop->defer(cb);
    }
}
