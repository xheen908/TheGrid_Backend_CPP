#include "GMUtil.hpp"
#include "GMCommands.hpp"
#include "GameLogic.hpp"
#include "Logger.hpp"
#include "Database.hpp"
#include "SocketHandlers.hpp"

namespace GMCommandsImpl {

    void handleLevel(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player) {
        if (args.empty()) return;
        int newLevel = 1;
        try { newLevel = std::stoi(args[0]); } catch (...) { return; }

        std::string targetId = player->currentTargetId;
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
                    GMUtil::sendSystemMessage(ws, "Mob " + m.name + " Level auf " + std::to_string(newLevel) + " gesetzt.");
                    break;
                }
            }
        }

        // 2. Player check
        if (!targetFound && !targetId.empty()) {
            auto targetP = GMUtil::findPlayer(targetId);
            if (targetP) {
                targetP->level = newLevel;
                targetP->xp = 0;
                auto ld = GameLogic::getLevelData(newLevel);
                targetP->maxHp = ld.hp;
                targetP->hp = targetP->maxHp;
                targetP->maxMana = ld.mana;
                targetP->mana = targetP->maxMana;

                json statusMsg = {
                    {"type", "player_status"}, {"username", targetP->charName},
                    {"hp", targetP->hp}, {"max_hp", targetP->maxHp},
                    {"mana", targetP->mana}, {"max_mana", targetP->maxMana},
                    {"level", targetP->level}, {"xp", 0}, {"max_xp", ld.xpToNextLevel}
                };

                auto tWs = (uWS::WebSocket<false, true, PerSocketData>*)targetP->ws;
                if (tWs && !targetP->isDisconnected) {
                    tWs->send(statusMsg.dump(), uWS::OpCode::TEXT);
                    GMUtil::sendSystemMessage(tWs, "Dein Level wurde von einem GM auf " + std::to_string(newLevel) + " gesetzt.");
                }

                GMUtil::broadcastLevelUp(targetP->charName, targetP->mapName);
                Database::getInstance().savePlayer(*targetP);
                targetFound = true;
                GMUtil::sendSystemMessage(ws, "Spieler " + targetP->charName + " Level auf " + std::to_string(newLevel) + " gesetzt.");
            }
        }

        // 3. Default to self
        if (!targetFound) {
            player->level = newLevel;
            player->xp = 0;
            auto ld = GameLogic::getLevelData(newLevel);
            player->maxHp = ld.hp;
            player->hp = player->maxHp;
            player->maxMana = ld.mana;
            player->mana = player->maxMana;

            json statusMsg = {
                {"type", "player_status"}, {"username", player->charName},
                {"hp", player->hp}, {"max_hp", player->maxHp},
                {"mana", player->mana}, {"max_mana", player->maxMana},
                {"level", player->level}, {"xp", 0}, {"max_xp", ld.xpToNextLevel}
            };
            ws->send(statusMsg.dump(), uWS::OpCode::TEXT);
            GMUtil::sendSystemMessage(ws, "Du hast dein Level auf " + std::to_string(newLevel) + " gesetzt.");
            GMUtil::broadcastLevelUp(player->charName, player->mapName);
            Database::getInstance().savePlayer(*player);
        }
    }

    void handleKick(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args) {
        if (args.empty()) return;
        auto targetP = GMUtil::findPlayer(args[0]);
        if (targetP) {
            auto tWs = (uWS::WebSocket<false, true, PerSocketData>*)targetP->ws;
            if (tWs && !targetP->isDisconnected) {
                tWs->send(json{{"type", "error"}, {"message", "Du wurdest von einem GM gekickt."}}.dump(), uWS::OpCode::TEXT);
                tWs->close();
                GMUtil::sendSystemMessage(ws, "Spieler " + targetP->charName + " gekickt.");
            }
        }
    }

    void handleInfo(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player) {
        std::string targetId = args.empty() ? player->currentTargetId : args[0];
        if (targetId.empty()) {
            GMUtil::sendSystemMessage(ws, "Benutzung: /info [Spielername] oder Ziel auswählen.");
            return;
        }

        bool found = false;
        // Mob check
        {
            std::lock_guard<std::recursive_mutex> lock(GameState::getInstance().getMtx());
            for (auto const& m : GameState::getInstance().getMobs()) {
                if (m.id == targetId && m.mapName == player->mapName) {
                    GMUtil::sendSystemMessage(ws, "--- Mob Info ---");
                    GMUtil::sendSystemMessage(ws, "Name: " + m.name);
                    GMUtil::sendSystemMessage(ws, "ID: " + m.id);
                    GMUtil::sendSystemMessage(ws, "Level: " + std::to_string(m.level));
                    GMUtil::sendSystemMessage(ws, "HP: " + std::to_string(m.hp) + "/" + std::to_string(m.maxHp));
                    GMUtil::sendSystemMessage(ws, "Pos: X:" + std::to_string((int)m.transform.x) + " Y:" + std::to_string((int)m.transform.y) + " Z:" + std::to_string((int)m.transform.z));
                    found = true;
                    break;
                }
            }
        }

        if (found) return;

        // Player check
        auto targetP = GMUtil::findPlayer(targetId);
        if (targetP) {
            std::string status = targetP->isDisconnected ? "OFFLINE" : "ONLINE";
            GMUtil::sendSystemMessage(ws, "--- Spieler Info ---");
            GMUtil::sendSystemMessage(ws, "Charakter: " + targetP->charName);
            GMUtil::sendSystemMessage(ws, "Account: " + targetP->username);
            GMUtil::sendSystemMessage(ws, "Status: " + status);
            GMUtil::sendSystemMessage(ws, "Level: " + std::to_string(targetP->level));
            GMUtil::sendSystemMessage(ws, "Map: " + targetP->mapName);
            GMUtil::sendSystemMessage(ws, "Position: X:" + std::to_string((int)targetP->lastPos.x) + " Y:" + std::to_string((int)targetP->lastPos.y) + " Z:" + std::to_string((int)targetP->lastPos.z));
        } else {
            GMUtil::sendSystemMessage(ws, "Ziel \"" + targetId + "\" wurde nicht gefunden.");
        }
    }

    void handleInvisible(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player) {
        bool turnOn = (args.size() >= 1 && args[0] == "on");
        player->isInvisible = turnOn;
        GMUtil::sendSystemMessage(ws, std::string("Unsichtbarkeit: ") + (turnOn ? "AN" : "AUS"));
        
        if (turnOn) {
            json leaveMsg = {{"type", "player_left"}, {"username", player->charName}};
            SocketHandlers::broadcastToMap(player->mapName, leaveMsg.dump(), ws);
        } else {
            json arriveMsg = {
                {"type", "player_moved"}, {"username", player->charName},
                {"position", {{"x", player->lastPos.x}, {"y", player->lastPos.y}, {"z", player->lastPos.z}}},
                {"rotation", {{"x", player->rotation.x}, {"y", player->rotation.y}, {"z", player->rotation.z}}},
                {"is_gm", player->isGMFlagged}
            };
            SocketHandlers::broadcastToMap(player->mapName, arriveMsg.dump(), ws);
        }
    }
    void handleHeal(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player) {
        std::string targetId = player->currentTargetId;
        bool targetFound = false;

        // 1. Mob check
        {
            std::lock_guard<std::recursive_mutex> lock(GameState::getInstance().getMtx());
            for (auto& m : GameState::getInstance().getMobs()) {
                if (m.id == targetId && m.mapName == player->mapName) {
                    m.hp = m.maxHp;
                    targetFound = true;
                    GMUtil::sendSystemMessage(ws, "Mob " + m.name + " geheilt.");
                    break;
                }
            }
        }

        // 2. Player check
        if (!targetFound && !targetId.empty()) {
            auto targetP = GMUtil::findPlayer(targetId);
            if (targetP) {
                targetP->hp = targetP->maxHp;
                targetP->mana = targetP->maxMana;
                
                json statusMsg = {
                    {"type", "player_status"}, {"username", targetP->charName},
                    {"hp", targetP->hp}, {"max_hp", targetP->maxHp},
                    {"mana", targetP->mana}, {"max_mana", targetP->maxMana}
                };
                
                auto tWs = (uWS::WebSocket<false, true, PerSocketData>*)targetP->ws;
                if (tWs && !targetP->isDisconnected) {
                    tWs->send(statusMsg.dump(), uWS::OpCode::TEXT);
                    GMUtil::sendSystemMessage(tWs, "Du wurdest von einem GM geheilt.");
                }
                GMUtil::sendSystemMessage(ws, "Spieler " + targetP->charName + " geheilt.");
                targetFound = true;
            }
        }

        // 3. Heal self
        if (!targetFound) {
            player->hp = player->maxHp;
            player->mana = player->maxMana;
            json statusMsg = {
                {"type", "player_status"}, {"username", player->charName},
                {"hp", player->hp}, {"max_hp", player->maxHp},
                {"mana", player->mana}, {"max_mana", player->maxMana}
            };
            ws->send(statusMsg.dump(), uWS::OpCode::TEXT);
            GMUtil::sendSystemMessage(ws, "Du hast dich selbst geheilt.");
        }
    }
}
