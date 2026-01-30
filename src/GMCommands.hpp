#ifndef GMCOMMANDS_HPP
#define GMCOMMANDS_HPP

#include "App.h"
#include "GameState.hpp"
#include "CommonTypes.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

class GMCommands {
public:
    static bool handleCommand(uWS::WebSocket<false, true, PerSocketData>* ws, const std::string& cmd, const std::vector<std::string>& args, const std::string& fullMsg);
};

#endif
