#ifndef GMCOMMANDSIMPL_HPP
#define GMCOMMANDSIMPL_HPP

#include "App.h"
#include "GameState.hpp"
#include <string>
#include <vector>
#include <memory>

namespace GMCommandsImpl {
    void handleLevel(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player);
    void handleKick(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args);
    void handleInfo(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player);
    void handleInvisible(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player);
    
    void handleTeleport(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player);
    void handleMove(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player);
    void handleTpCoords(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player);
    
    void handleGMRank(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player);
    void handlePos(uWS::WebSocket<false, true, PerSocketData>* ws, std::shared_ptr<Player> player);
    void handleGravity(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player);
    void handleSpeed(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player);
    void handleHeal(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player);
    void handleAddItem(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<std::string>& args, std::shared_ptr<Player> player);
    void handleHelp(uWS::WebSocket<false, true, PerSocketData>* ws);
}

#endif
