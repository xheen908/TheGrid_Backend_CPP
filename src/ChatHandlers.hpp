#ifndef CHATHANDLERS_HPP
#define CHATHANDLERS_HPP

#include "App.h"
#include "CommonTypes.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ChatHandlers {
public:
    static void handleChatMessage(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j);
};

#endif
