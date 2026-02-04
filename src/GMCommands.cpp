#include "GMCommands.hpp"
#include "gm/GMCommandsImpl.hpp"
#include "gm/GMUtil.hpp"
#include "GameState.hpp"
#include "Logger.hpp"

bool GMCommands::handleCommand(uWS::WebSocket<false, true, PerSocketData>* ws, const std::string& cmd, const std::vector<std::string>& args, const std::string& fullMsg) {
    auto data = ws->getUserData();
    if (!data || !data->isGM) return false;

    auto player = GameState::getInstance().getPlayer(data->username);
    if (!player) return false;

    if (cmd == "level") {
        GMCommandsImpl::handleLevel(ws, args, player);
    } else if (cmd == "tele" || cmd == "tp" || cmd == "goto") {
        if ((cmd == "tp" && args.size() >= 3)) {
            GMCommandsImpl::handleTpCoords(ws, args, player);
        } else {
            GMCommandsImpl::handleTeleport(ws, args, player);
        }
    } else if (cmd == "gm") {
        GMCommandsImpl::handleGMRank(ws, args, player);
    } else if (cmd == "move") {
        GMCommandsImpl::handleMove(ws, args, player);
    } else if (cmd == "invisible") {
        GMCommandsImpl::handleInvisible(ws, args, player);
    } else if (cmd == "heal" || cmd == "revive") {
        GMCommandsImpl::handleHeal(ws, args, player);
    } else if (cmd == "kick") {
        GMCommandsImpl::handleKick(ws, args);
    } else if (cmd == "additem" || cmd == "item") {
        GMCommandsImpl::handleAddItem(ws, args, player);
    } else if (cmd == "info") {
        GMCommandsImpl::handleInfo(ws, args, player);
    } else if (cmd == "pos" || cmd == "gps" || cmd == "coords") {
        GMCommandsImpl::handlePos(ws, player);
    } else if (cmd == "target") {
        GMUtil::sendSystemMessage(ws, "Aktuelles Server-Ziel: " + (player->currentTargetId.empty() ? "KEINS" : player->currentTargetId));
    } else if (cmd == "gravity") {
        GMCommandsImpl::handleGravity(ws, args, player);
    } else if (cmd == "speed") {
        GMCommandsImpl::handleSpeed(ws, args, player);
    } else if (cmd == "help") {
        GMCommandsImpl::handleHelp(ws);
    } else {
        return false;
    }

    return true;
}
