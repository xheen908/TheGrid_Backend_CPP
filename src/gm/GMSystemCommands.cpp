#include "GMUtil.hpp"
#include "GMCommands.hpp"
#include "SocketHandlers.hpp"
#include <algorithm>

namespace GMCommandsImpl {

    void handleGMRank(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player) {
        bool turnOn = (args.size() >= 1 && args[0] == "on");
        player->isGMFlagged = turnOn;
        GMUtil::sendSystemMessage(ws, std::string("GM-Anzeige: ") + (turnOn ? "AN" : "AUS"));
        
        SocketHandlers::broadcastToMap(player->mapName, json{
            {"type", "player_moved"}, {"username", player->charName},
            {"position", {{"x", player->lastPos.x}, {"y", player->lastPos.y}, {"z", player->lastPos.z}}},
            {"rotation", {{"x", player->rotation.x}, {"y", player->rotation.y}, {"z", player->rotation.z}}},
            {"is_gm", player->isGMFlagged}
        }.dump(), ws);
    }

    void handlePos(uWS::WebSocket<false, true, PerSocketData>* ws, std::shared_ptr<Player> player) {
        std::string posStr = "Pos: X: " + std::to_string(player->lastPos.x) + 
                          " Y: " + std::to_string(player->lastPos.y) + 
                          " Z: " + std::to_string(player->lastPos.z) +
                          " | Rot: Y: " + std::to_string(player->rotation.y);
        GMUtil::sendSystemMessage(ws, posStr);
    }

    void handleGravity(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player) {
        if (args.empty()) return;
        std::string sub = args[0];
        std::transform(sub.begin(), sub.end(), sub.begin(), ::tolower);
        if (sub == "off") {
            player->gravityEnabled = false;
            GMUtil::sendSystemMessage(ws, "Gravitation deaktiviert (Fly-Mode AN).");
        } else if (sub == "on") {
            player->gravityEnabled = true;
            GMUtil::sendSystemMessage(ws, "Gravitation aktiviert.");
        }
    }

    void handleSpeed(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player) {
        if (args.empty()) return;
        try {
            float val = std::stof(args[0]);
            if (val > 0) {
                player->speedMultiplier = val;
                GMUtil::sendSystemMessage(ws, "Geschwindigkeit auf " + std::to_string(val) + " gesetzt.");
            }
        } catch (...) {}
    }

    void handleHelp(uWS::WebSocket<false, true, PerSocketData>* ws) {
        GMUtil::sendSystemMessage(ws, "/w, /level, /tele, /invisible, /gm, /tp, /kick, /move, /info, /target, /pos, /gravity, /speed");
    }
}
