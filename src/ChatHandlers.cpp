#include "ChatHandlers.hpp"
#include "SocketHandlers.hpp"
#include "GameState.hpp"
#include "GMCommands.hpp"
#include <algorithm>
#include <sstream>

void ChatHandlers::handleChatMessage(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j) {
    auto data = ws->getUserData();
    if (!data) return;
    
    std::string msg = j.value("message", "");
    if (msg.empty()) return;

    if (msg[0] == '/') {
        std::string cmdStr = msg.substr(1);
        size_t spacePos = cmdStr.find(' ');
        std::string cmd = (spacePos == std::string::npos) ? cmdStr : cmdStr.substr(0, spacePos);
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

        if (cmd == "i" || cmd == "invite") {
            if (spacePos != std::string::npos) {
                std::string target = cmdStr.substr(spacePos + 1);
                SocketHandlers::handlePartyInvite(ws, {{"target_name", target}});
            }
            return;
        } else if (cmd == "w" || cmd == "whisper") {
            if (spacePos != std::string::npos) {
                std::string rest = cmdStr.substr(spacePos + 1);
                size_t space2 = rest.find(' ');
                if (space2 != std::string::npos) {
                    std::string targetName = rest.substr(0, space2);
                    std::string content = rest.substr(space2 + 1);
                    
                    auto players = GameState::getInstance().getPlayersSnapshot();
                    bool found = false;
                    for (auto const& p : players) {
                        if (p->charName == targetName && !p->isDisconnected) {
                            json w = {{"type", "chat_receive"}, {"mode", "whisper"}, {"from", data->charName}, {"to", targetName}, {"message", content}};
                            auto tWs = (uWS::WebSocket<false, true, PerSocketData>*)p->ws;
                            if (tWs) tWs->send(w.dump(), uWS::OpCode::TEXT);
                            ws->send(w.dump(), uWS::OpCode::TEXT);
                            found = true;
                            break;
                        }
                    }
                    if (!found) ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Spieler \"" + targetName + "\" wurde nicht gefunden."}}.dump(), uWS::OpCode::TEXT);
                }
            }
            return;
        } else if (cmd == "leave") {
            SocketHandlers::handlePartyLeave(ws, j);
            return;
        } else if (cmd == "p" || cmd == "party") {
            if (spacePos != std::string::npos) {
                std::string content = cmdStr.substr(spacePos + 1);
                auto player = GameState::getInstance().getPlayer(data->username);
                if (player && !player->partyId.empty()) {
                    auto party = GameState::getInstance().getParty(player->partyId);
                    if (party) {
                        json pMsg = {{"type", "chat_receive"}, {"mode", "party"}, {"from", data->charName}, {"message", content}};
                        std::string msgStr = pMsg.dump();
                        for (auto const& mName : party->members) {
                            auto member = GameState::getInstance().getPlayer(mName);
                            if (member && !member->isDisconnected) {
                                auto mWs = (uWS::WebSocket<false, true, PerSocketData>*)member->ws;
                                if (mWs) mWs->send(msgStr, uWS::OpCode::TEXT);
                            }
                        }
                    }
                } else {
                    ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Du bist in keiner Gruppe."}}.dump(), uWS::OpCode::TEXT);
                }
            }
            return;
        } else if (cmd == "1") {
            if (spacePos != std::string::npos) {
                std::string content = cmdStr.substr(spacePos + 1);
                json worldMsg = {{"type", "chat_receive"}, {"mode", "world"}, {"from", data->charName}, {"message", content}};
                SocketHandlers::broadcastToMap(data->mapName, worldMsg.dump());
            }
            return;
        } else if (cmd == "kick") {
            if (spacePos != std::string::npos) {
                std::string target = cmdStr.substr(spacePos + 1);
                SocketHandlers::handlePartyKick(ws, {{"target_name", target}});
            }
            return;
        }

        // Try GM Commands
        std::vector<std::string> args;
        if (spacePos != std::string::npos) {
            std::string rest = cmdStr.substr(spacePos + 1);
            std::stringstream ss(rest);
            std::string arg;
            while (ss >> arg) args.push_back(arg);
        }
        
        if (GMCommands::handleCommand(ws, cmd, args, msg)) {
            return;
        }
    }

    json chatData = {
        {"type", "chat_receive"},
        {"mode", "map"},
        {"from", data->charName},
        {"message", msg}
    };

    SocketHandlers::broadcastToMap(data->mapName, chatData.dump());
}
