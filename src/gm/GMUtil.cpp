#include "GMUtil.hpp"
#include "SocketHandlers.hpp"
#include <algorithm>

namespace GMUtil {
    void sendSystemMessage(uWS::WebSocket<false, true, PerSocketData>* ws, const std::string& msg) {
        ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", msg}}.dump(), uWS::OpCode::TEXT);
    }

    std::shared_ptr<Player> findPlayer(const std::string& nameOrId) {
        auto players = GameState::getInstance().getPlayersSnapshot();
        for (auto& p : players) {
            if (p->charName == nameOrId || p->username == nameOrId) {
                return p;
            }
        }
        return nullptr;
    }

    bool findMob(const std::string& targetId, const std::string& mapName, Mob& outMob) {
        std::lock_guard<std::recursive_mutex> lock(GameState::getInstance().getMtx());
        for (auto& m : GameState::getInstance().getMobs()) {
            if (m.id == targetId && m.mapName == mapName) {
                outMob = m; // Note: this copies the mob, might not be what we want if we want to modify it directly
                // Actually, GM commands often modify the mob in the state.
                return true;
            }
        }
        return false;
    }

    void broadcastLevelUp(const std::string& charName, const std::string& mapName) {
        json levelUpMsg = {{"type", "level_up"}, {"username", charName}};
        SocketHandlers::broadcastToMap(mapName, levelUpMsg.dump());
    }
}
