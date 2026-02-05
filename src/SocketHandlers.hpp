#ifndef SOCKETHANDLERS_HPP
#define SOCKETHANDLERS_HPP

#include "App.h"
#include "GameState.hpp"
#include "CommonTypes.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class SocketHandlers {
public:
    static void handleMapChange(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j);
    static void handleSpellCast(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j);
    static void handlePartyInvite(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j);
    static void handlePartyResponse(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j);
    static void handlePartyLeave(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j);
    static void handlePartyKick(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j);
    static void handleMoveItem(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j);
    static void handleUseItem(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j);
    static void handleDestroyItem(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j);

    // Trading
    static void handleTradeRequest(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j);
    static void handleTradeResponse(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j);
    static void handleTradeAddItem(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j);
    static void handleTradeRemoveItem(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j);
    static void handleTradeReady(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j);
    static void handleTradeConfirm(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j);
    static void handleTradeCancel(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j);
    
    // Quests
    static void handleQuestInteract(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j);
    static void handleQuestAccept(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j);
    static void handleQuestReward(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j);

    static void syncGameObjects(uWS::WebSocket<false, true, PerSocketData>* ws, const std::string& mapName);
    static void broadcastToMap(const std::string& mapName, const std::string& message, uWS::WebSocket<false, true, PerSocketData>* exclude = nullptr);
    static void sendSafe(uWS::WebSocket<false, true, PerSocketData>* ws, const std::string& message);
    static void sendToPlayer(const std::string& username, const std::string& message);
    static void syncPlayerStatus(std::shared_ptr<Player> player);
};

#endif
