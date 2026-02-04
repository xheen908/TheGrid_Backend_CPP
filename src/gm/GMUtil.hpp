#ifndef GMUTIL_HPP
#define GMUTIL_HPP

#include "App.h"
#include "GameState.hpp"
#include "CommonTypes.hpp"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace GMUtil {
    void sendSystemMessage(uWS::WebSocket<false, true, PerSocketData>* ws, const std::string& msg);
    std::shared_ptr<Player> findPlayer(const std::string& nameOrId);
    bool findMob(const std::string& targetId, const std::string& mapName, Mob& outMob);
    void broadcastLevelUp(const std::string& charName, const std::string& mapName);
}

#endif
