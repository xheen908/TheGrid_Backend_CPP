#include "GMUtil.hpp"
#include "GMCommands.hpp"
#include "SocketHandlers.hpp"
#include <iostream>

namespace GMCommandsImpl {

    void handleTeleport(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player) {
        if (args.empty()) return;
        std::string target = args[0];
        auto targetP = GMUtil::findPlayer(target);
        
        std::string oldMap = player->mapName;
        std::string newMap = "";
        Vector3 newPos = {0, 0, 0};

        if (targetP) {
            newMap = targetP->mapName;
            newPos = targetP->lastPos;
        } else {
            // Treat as map name
            newMap = target;
            // Normalization
            if (newMap == "arena0") newMap = "Arena0";
            else if (newMap == "arena1") newMap = "Arena1";
            else if (newMap == "arena2") newMap = "Arena2";
            else if (newMap == "worldmap0") newMap = "WorldMap0";
            else if (newMap == "dungeon0") newMap = "Dungeon0";
            else if (newMap == "testmap0") newMap = "TestMap0";
            
            newPos = (newMap == "TestMap0") ? Vector3{0, 10, 0} : Vector3{0, 5, 0};
        }

        player->mapName = newMap;
        player->lastPos = newPos;
        auto data = ws->getUserData();
        if (data) data->mapName = newMap;

        SocketHandlers::broadcastToMap(oldMap, json{{"type", "player_left"}, {"username", player->charName}}.dump(), ws);
        ws->send(json{
            {"type", "map_changed"}, {"map_name", newMap},
            {"position", {{"x", newPos.x}, {"y", newPos.y}, {"z", newPos.z}}},
            {"rotation_y", 0}
        }.dump(), uWS::OpCode::TEXT);
        SocketHandlers::syncGameObjects(ws, newMap);
    }

    void handleMove(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player) {
        if (args.empty()) return;
        auto targetP = GMUtil::findPlayer(args[0]);
        if (targetP) {
            std::string oldMap = targetP->mapName;
            targetP->mapName = player->mapName;
            targetP->lastPos = player->lastPos;
            
            auto tWs = (uWS::WebSocket<false, true, PerSocketData>*)targetP->ws;
            if (tWs && !targetP->isDisconnected) {
                auto tData = tWs->getUserData();
                if (tData) tData->mapName = player->mapName;
                SocketHandlers::broadcastToMap(oldMap, json{{"type", "player_left"}, {"username", targetP->charName}}.dump(), tWs);
                tWs->send(json{
                    {"type", "map_changed"}, {"map_name", targetP->mapName},
                    {"position", {{"x", targetP->lastPos.x}, {"y", targetP->lastPos.y}, {"z", targetP->lastPos.z}}},
                    {"rotation_y", targetP->rotation.y}
                }.dump(), uWS::OpCode::TEXT);
            }
        }
    }

    void handleTpCoords(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player) {
        if (args.size() < 3) return;
        try {
            float x = std::stof(args[0]), y = std::stof(args[1]), z = std::stof(args[2]);
            player->lastPos = {x, y, z};
            ws->send(json{
                {"type", "map_changed"}, {"map_name", player->mapName},
                {"position", {{"x", x}, {"y", y}, {"z", z}}},
                {"rotation_y", player->rotation.y}
            }.dump(), uWS::OpCode::TEXT);
            SocketHandlers::syncGameObjects(ws, player->mapName);
        } catch (...) {}
    }
}
