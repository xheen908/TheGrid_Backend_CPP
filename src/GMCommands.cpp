#include "GMCommands.hpp"
#include "GameState.hpp"
#include "GameLogic.hpp"
#include "Logger.hpp"
#include "SocketHandlers.hpp"
#include "Database.hpp"
#include <iostream>
#include <algorithm>

bool GMCommands::handleCommand(uWS::WebSocket<false, true, PerSocketData>* ws, const std::string& cmd, const std::vector<std::string>& args, const std::string& fullMsg) {
    auto data = ws->getUserData();
    if (!data) return false;

    auto player = GameState::getInstance().getPlayer(data->username);
    if (!player) return false;

    // Check GM status. data->isGM is the permanent permission from the database.
    if (!data->isGM) {
        return false; 
    }

    if (cmd == "level") {
        if (args.size() >= 1) {
            int newLevel = 1;
            try { newLevel = std::stoi(args[0]); } catch (...) { return true; }
            
            // Try to find target mob or player
            std::string targetId = player->currentTargetId; 
            Logger::log("[DEBUG] GM " + player->charName + " using /level. Current targetId in state: " + (targetId.empty() ? "None" : targetId));
            
            bool targetFound = false;
            // 1. Mob check
            {
                std::lock_guard<std::recursive_mutex> lock(GameState::getInstance().getMtx());
                for (auto& m : GameState::getInstance().getMobs()) {
                    if (m.id == targetId && m.mapName == player->mapName) {
                        m.level = newLevel;
                        m.maxHp = GameLogic::getMobMaxHp(newLevel);
                        m.hp = m.maxHp;
                        targetFound = true;
                        Logger::log("[GM] Mob " + m.name + " (" + m.id + ") level set to " + std::to_string(newLevel) + " by " + player->charName);
                        ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Mob " + m.name + " Level auf " + std::to_string(newLevel) + " gesetzt."}}.dump(), uWS::OpCode::TEXT);
                        break;
                    }
                }
            }

            // 2. Player check (Target)
            if (!targetFound && !targetId.empty()) {
                auto players = GameState::getInstance().getPlayersSnapshot();
                for (auto& p : players) {
                    if (p->charName == targetId || p->username == targetId) {
                        p->level = newLevel;
                        p->xp = 0;
                        auto ld = GameLogic::getLevelData(newLevel);
                        p->maxHp = ld.hp;
                        p->hp = p->maxHp;
                        p->maxMana = ld.mana;
                        p->mana = p->maxMana;
                        
                        json statusMsg = {
                            {"type", "player_status"},
                            {"username", p->charName},
                            {"hp", p->hp},
                            {"max_hp", p->maxHp},
                            {"mana", p->mana},
                            {"max_mana", p->maxMana},
                            {"level", p->level},
                            {"xp", 0},
                            {"max_xp", ld.xpToNextLevel}
                        };
                        auto tWs = (uWS::WebSocket<false, true, PerSocketData>*)p->ws;
                        if (tWs && !p->isDisconnected) {
                            tWs->send(statusMsg.dump(), uWS::OpCode::TEXT);
                            tWs->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Dein Level wurde von einem GM auf " + std::to_string(newLevel) + " gesetzt."}}.dump(), uWS::OpCode::TEXT);
                        }

                        // Broadast LevelUp effect to everyone on map
                        json levelUpMsg = {{"type", "level_up"}, {"username", p->charName}};
                        SocketHandlers::broadcastToMap(p->mapName, levelUpMsg.dump());

                        // Party notification
                        if (!p->partyId.empty()) {
                            auto party = GameState::getInstance().getParty(p->partyId);
                            if (party) {
                                json pMsg = {{"type", "chat_receive"}, {"mode", "party"}, {"from", "System"}, {"message", p->charName + " ist jetzt Level " + std::to_string(newLevel) + "."}};
                                std::string pMsgStr = pMsg.dump();
                                for (auto const& mName : party->members) {
                                    auto member = GameState::getInstance().getPlayer(mName);
                                    if (member && !member->isDisconnected && member->ws) {
                                        ((uWS::WebSocket<false, true, PerSocketData>*)member->ws)->send(pMsgStr, uWS::OpCode::TEXT);
                                    }
                                }
                            }
                        }
                        
                        Database::getInstance().savePlayer(*p);
                        targetFound = true;
                        Logger::log("[GM] Player " + p->charName + " level set to " + std::to_string(newLevel) + " by " + player->charName);
                        ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Spieler " + p->charName + " Level auf " + std::to_string(newLevel) + " gesetzt."}}.dump(), uWS::OpCode::TEXT);
                        break;
                    }
                }
            }

            // 3. Default to self if no target found or no target selected
            if (!targetFound) {
                player->level = newLevel;
                player->xp = 0;
                auto ld = GameLogic::getLevelData(newLevel);
                player->maxHp = ld.hp;
                player->hp = player->maxHp;
                player->maxMana = ld.mana;
                player->mana = player->maxMana;
                
                json statusMsg = {
                    {"type", "player_status"},
                    {"username", player->charName},
                    {"hp", player->hp},
                    {"max_hp", player->maxHp},
                    {"mana", player->mana},
                    {"max_mana", player->maxMana},
                    {"level", player->level},
                    {"xp", 0},
                    {"max_xp", ld.xpToNextLevel}
                };
                ws->send(statusMsg.dump(), uWS::OpCode::TEXT);
                ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Du hast dein Level auf " + std::to_string(newLevel) + " gesetzt."}}.dump(), uWS::OpCode::TEXT);
                
                // Broadast LevelUp effect to everyone on map
                json levelUpSelfMsg = {{"type", "level_up"}, {"username", player->charName}};
                SocketHandlers::broadcastToMap(player->mapName, levelUpSelfMsg.dump());
                
                // Party notification for self
                if (!player->partyId.empty()) {
                    auto party = GameState::getInstance().getParty(player->partyId);
                    if (party) {
                        json pMsg = {{"type", "chat_receive"}, {"mode", "party"}, {"from", "System"}, {"message", player->charName + " ist jetzt Level " + std::to_string(newLevel) + "."}};
                        std::string pMsgStr = pMsg.dump();
                        for (auto const& mName : party->members) {
                            auto member = GameState::getInstance().getPlayer(mName);
                            if (member && !member->isDisconnected && member->ws && member->ws != ws) {
                                ((uWS::WebSocket<false, true, PerSocketData>*)member->ws)->send(pMsgStr, uWS::OpCode::TEXT);
                            }
                        }
                    }
                }

                Database::getInstance().savePlayer(*player);
                Logger::log("[GM] " + player->charName + " set own level to " + std::to_string(newLevel));
            }
        }
        return true;
    }
    else if (cmd == "tele" || cmd == "tp") {
        if (args.size() >= 1) {
            std::string targetName = args[0];
            bool found = false;
            
            // 1. Try to find a player
            auto players = GameState::getInstance().getPlayersSnapshot();
            for (auto& p : players) {
                if (p->charName == targetName || p->username == targetName) {
                    std::string oldMap = player->mapName;
                    player->mapName = p->mapName;
                    player->lastPos = p->lastPos;
                    player->rotation = p->rotation;
                    data->mapName = p->mapName;

                    json leaveMsg = {{"type", "player_left"}, {"username", player->charName}};
                    SocketHandlers::broadcastToMap(oldMap, leaveMsg.dump(), ws);

                    json mapMsg = {
                        {"type", "map_changed"},
                        {"map_name", player->mapName},
                        {"position", {{"x", player->lastPos.x}, {"y", player->lastPos.y}, {"z", player->lastPos.z}}},
                        {"rotation_y", player->rotation.y}
                    };
                    ws->send(mapMsg.dump(), uWS::OpCode::TEXT);
                    found = true;
                    break;
                }
            }

            // 2. If no player found, try to treat it as a map (like /goto)
            if (!found) {
                // Check common map names or just try it
                // We'll normalize to common cases if it matches arena0 -> Arena0
                std::string mapTarget = targetName;
                if (mapTarget == "arena0") mapTarget = "Arena0";
                else if (mapTarget == "arena1") mapTarget = "Arena1";
                else if (mapTarget == "arena2") mapTarget = "Arena2";
                else if (mapTarget == "worldmap0") mapTarget = "WorldMap0";
                else if (mapTarget == "dungeon0") mapTarget = "Dungeon0";
                else if (mapTarget == "testmap0") mapTarget = "TestMap0";

                std::string oldMap = player->mapName;
                player->mapName = mapTarget;
                
                Vector3 telePos = {0.0f, 5.0f, 0.0f};
                if (mapTarget == "TestMap0") {
                    telePos = {0.0f, 10.0f, 0.0f};
                }
                
                player->lastPos = telePos;
                player->rotation = {0, 0, 0};
                data->mapName = mapTarget;

                json leaveMsg = {{"type", "player_left"}, {"username", player->charName}};
                SocketHandlers::broadcastToMap(oldMap, leaveMsg.dump(), ws);

                json mapMsg = {
                    {"type", "map_changed"},
                    {"map_name", mapTarget},
                    {"position", {{"x", telePos.x}, {"y", telePos.y}, {"z", telePos.z}}},
                    {"rotation_y", 0}
                };
                ws->send(mapMsg.dump(), uWS::OpCode::TEXT);
                found = true;
            }
        }
        return true;
    }
    else if (cmd == "goto") {
        if (args.size() >= 1) {
            std::string mapName = args[0];
            // Name Normalisierung
            if (mapName == "arena0") mapName = "Arena0";
            else if (mapName == "worldmap0") mapName = "WorldMap0";
            else if (mapName == "dungeon0") mapName = "Dungeon0";

            std::string oldMap = player->mapName;
            player->mapName = mapName;
            player->lastPos = {0, 1, 0};
            player->rotation = {0, 0, 0};
            data->mapName = mapName;

            json leaveMsg = {{"type", "player_left"}, {"username", player->charName}};
            SocketHandlers::broadcastToMap(oldMap, leaveMsg.dump(), ws);

            json mapMsg = {
                {"type", "map_changed"},
                {"map_name", mapName},
                {"position", {{"x", 0}, {"y", 1}, {"z", 0}}},
                {"rotation_y", 0}
            };
            ws->send(mapMsg.dump(), uWS::OpCode::TEXT);
        } else {
            ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Benutzung: /goto MapName (z.B. Arena0)"}}.dump(), uWS::OpCode::TEXT);
        }
        return true;
    }
    else if (cmd == "gm") {
        bool turnOn = (args.size() >= 1 && args[0] == "on");
        player->isGMFlagged = turnOn;
        ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", std::string("GM-Anzeige: ") + (turnOn ? "AN" : "AUS")}}.dump(), uWS::OpCode::TEXT);
        
        json updateMsg = {
            {"type", "player_moved"},
            {"username", player->charName},
            {"position", {{"x", player->lastPos.x}, {"y", player->lastPos.y}, {"z", player->lastPos.z}}},
            {"rotation", {{"x", player->rotation.x}, {"y", player->rotation.y}, {"z", player->rotation.z}}},
            {"is_gm", player->isGMFlagged}
        };
        SocketHandlers::broadcastToMap(player->mapName, updateMsg.dump(), ws);
        return true;
    }
    else if (cmd == "tp") {
        if (args.size() >= 3) {
            try {
                float x = std::stof(args[0]);
                float y = std::stof(args[1]);
                float z = std::stof(args[2]);
                player->lastPos = {x, y, z};
                
                json mapMsg = {
                    {"type", "map_changed"},
                    {"map_name", player->mapName},
                    {"position", {{"x", x}, {"y", y}, {"z", z}}},
                    {"rotation_y", player->rotation.y}
                };
                ws->send(mapMsg.dump(), uWS::OpCode::TEXT);
            } catch (...) {}
        }
        return true;
    }
    else if (cmd == "move") {
        if (args.size() >= 1) {
            std::string targetName = args[0];
            auto players = GameState::getInstance().getPlayersSnapshot();
            for (auto& p : players) {
                if (p->charName == targetName) {
                    std::string oldMap = p->mapName;
                    p->mapName = player->mapName;
                    p->lastPos = player->lastPos;
                    p->rotation = player->rotation;
                    
                    auto tWs = (uWS::WebSocket<false, true, PerSocketData>*)p->ws;
                    if (tWs && !p->isDisconnected) {
                        auto tData = tWs->getUserData();
                        if (tData) tData->mapName = player->mapName;
                        
                        json leaveMsg = {{"type", "player_left"}, {"username", p->charName}};
                        SocketHandlers::broadcastToMap(oldMap, leaveMsg.dump(), tWs);

                        json mapMsg = {
                            {"type", "map_changed"},
                            {"map_name", p->mapName},
                            {"position", {{"x", p->lastPos.x}, {"y", p->lastPos.y}, {"z", p->lastPos.z}}},
                            {"rotation_y", p->rotation.y}
                        };
                        tWs->send(mapMsg.dump(), uWS::OpCode::TEXT);
                    }
                    break;
                }
            }
        }
        return true;
    }
    else if (cmd == "invisible") {
        bool turnOn = (args.size() >= 1 && args[0] == "on");
        player->isInvisible = turnOn;
        ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", std::string("Unsichtbarkeit: ") + (turnOn ? "AN" : "AUS")}}.dump(), uWS::OpCode::TEXT);
        
        if (turnOn) {
            json leaveMsg = {{"type", "player_left"}, {"username", player->charName}};
            SocketHandlers::broadcastToMap(player->mapName, leaveMsg.dump(), ws);
        } else {
            json arriveMsg = {
                {"type", "player_moved"},
                {"username", player->charName},
                {"position", {{"x", player->lastPos.x}, {"y", player->lastPos.y}, {"z", player->lastPos.z}}},
                {"rotation", {{"x", player->rotation.x}, {"y", player->rotation.y}, {"z", player->rotation.z}}},
                {"is_gm", player->isGMFlagged}
            };
            SocketHandlers::broadcastToMap(player->mapName, arriveMsg.dump(), ws);
        }
        return true;
    }
    else if (cmd == "kick") {
        if (args.size() >= 1) {
            std::string targetName = args[0];
            auto players = GameState::getInstance().getPlayersSnapshot();
            for (auto& p : players) {
                if (p->charName == targetName) {
                    auto tWs = (uWS::WebSocket<false, true, PerSocketData>*)p->ws;
                    if (tWs && !p->isDisconnected) {
                        tWs->send(json{{"type", "error"}, {"message", "Du wurdest von einem GM gekickt."}}.dump(), uWS::OpCode::TEXT);
                        tWs->close();
                        ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Spieler " + targetName + " gekickt."}}.dump(), uWS::OpCode::TEXT);
                    }
                    break;
                }
            }
        }
        return true;
    }
    else if (cmd == "info") {
        std::string targetId = "";
        if (args.empty()) {
            targetId = player->currentTargetId;
            if (targetId.empty()) {
                ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Benutzung: /info [Spielername] oder Ziel auswählen."}}.dump(), uWS::OpCode::TEXT);
                return true;
            }
        } else {
            targetId = args[0];
        }

        // 1. Check if it's a Mob
        bool foundMob = false;
        {
            std::lock_guard<std::recursive_mutex> lock(GameState::getInstance().getMtx());
            for (auto const& m : GameState::getInstance().getMobs()) {
                if (m.id == targetId && m.mapName == player->mapName) {
                    ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "--- Mob Info ---"}}.dump(), uWS::OpCode::TEXT);
                    ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Name: " + m.name}}.dump(), uWS::OpCode::TEXT);
                    ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "ID: " + m.id}}.dump(), uWS::OpCode::TEXT);
                    ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Level: " + std::to_string(m.level)}}.dump(), uWS::OpCode::TEXT);
                    ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "HP: " + std::to_string(m.hp) + "/" + std::to_string(m.maxHp)}}.dump(), uWS::OpCode::TEXT);
                    ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Pos: X:" + std::to_string((int)m.transform.x) + " Y:" + std::to_string((int)m.transform.y) + " Z:" + std::to_string((int)m.transform.z)}}.dump(), uWS::OpCode::TEXT);
                    ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Home: X:" + std::to_string((int)m.home.x) + " Y:" + std::to_string((int)m.home.y) + " Z:" + std::to_string((int)m.home.z)}}.dump(), uWS::OpCode::TEXT);
                    foundMob = true;
                    break;
                }
            }
        }

        if (foundMob) return true;

        // 2. Check if it's a Player
        std::shared_ptr<Player> targetP = nullptr;
        auto players = GameState::getInstance().getPlayersSnapshot();
        for (auto& p : players) {
            if (p->charName == targetId || p->username == targetId) {
                targetP = p;
                break;
            }
        }

        if (targetP) {
            std::string status = targetP->isDisconnected ? "OFFLINE" : "ONLINE";
            ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "--- Spieler Info ---"}}.dump(), uWS::OpCode::TEXT);
            ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Charakter: " + targetP->charName}}.dump(), uWS::OpCode::TEXT);
            ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Account: " + targetP->username}}.dump(), uWS::OpCode::TEXT);
            ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Status: " + status}}.dump(), uWS::OpCode::TEXT);
            ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Level: " + std::to_string(targetP->level)}}.dump(), uWS::OpCode::TEXT);
            ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "XP: " + std::to_string(targetP->xp)}}.dump(), uWS::OpCode::TEXT);
            ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Map: " + targetP->mapName}}.dump(), uWS::OpCode::TEXT);
            ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Position: X:" + std::to_string((int)targetP->lastPos.x) + " Y:" + std::to_string((int)targetP->lastPos.y) + " Z:" + std::to_string((int)targetP->lastPos.z)}}.dump(), uWS::OpCode::TEXT);
        } else {
            ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Ziel \"" + targetId + "\" wurde nicht gefunden."}}.dump(), uWS::OpCode::TEXT);
        }
        return true;
    }
    else if (cmd == "pos" || cmd == "gps" || cmd == "coords") {
        std::string posStr = "Pos: X: " + std::to_string(player->lastPos.x) + 
                          " Y: " + std::to_string(player->lastPos.y) + 
                          " Z: " + std::to_string(player->lastPos.z) +
                          " | Rot: X: " + std::to_string(player->rotation.x) +
                          " Y: " + std::to_string(player->rotation.y) +
                          " Z: " + std::to_string(player->rotation.z);
        ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", posStr}}.dump(), uWS::OpCode::TEXT);
        return true;
    }
    else if (cmd == "target") {
        std::string targetId = player->currentTargetId;
        ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Aktuelles Server-Ziel: " + (targetId.empty() ? "KEINS" : targetId)}}.dump(), uWS::OpCode::TEXT);
        return true;
    }
    else if (cmd == "gravity") {
        if (args.size() >= 1) {
            std::string sub = args[0];
            std::transform(sub.begin(), sub.end(), sub.begin(), ::tolower);
            if (sub == "off") {
                player->gravityEnabled = false;
                ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Gravitation deaktiviert (Fly-Mode AN)."}}.dump(), uWS::OpCode::TEXT);
            } else if (sub == "on") {
                player->gravityEnabled = true;
                ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Gravitation aktiviert."}}.dump(), uWS::OpCode::TEXT);
            }
        }
        return true;
    }
    else if (cmd == "speed") {
        if (args.size() >= 1) {
            try {
                float val = std::stof(args[0]);
                if (val > 0) {
                    player->speedMultiplier = val;
                    ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Geschwindigkeit auf " + std::to_string(val) + " gesetzt."}}.dump(), uWS::OpCode::TEXT);
                }
            } catch (...) {}
        }
        return true;
    }
    else if (cmd == "help") {
        ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "/w, /level, /tele, /invisible, /gm, /tp, /kick, /move, /info, /target, /pos, /gravity, /speed"}}.dump(), uWS::OpCode::TEXT);
        return true;
    }

    return false;
}
