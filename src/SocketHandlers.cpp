#include "SocketHandlers.hpp"
#include "GameLogic.hpp"
#include "Database.hpp"
#include "Logger.hpp"
#include "abilities/AbilityManager.hpp"
#include "GMCommands.hpp"
#include "Server.hpp"
#include <iostream>
#include <cmath>
#include <functional>

void SocketHandlers::handleMapChange(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j) {
    auto data = ws->getUserData();
    if (!data) return;
    
    std::string newMap = j.value("map_name", "WorldMap0");
    std::string oldMap = data->mapName;
    
    Logger::log("[MAP] Player " + data->username + " requesting map change: " + oldMap + " -> " + newMap);

    auto player = GameState::getInstance().getPlayer(data->username);
    if (player) {
        // 1. Inform others on old map
        json leaveMsg = {{"type", "player_left"}, {"username", player->charName}};
        broadcastToMap(oldMap, leaveMsg.dump(), ws);

        {
            std::lock_guard<std::recursive_mutex> pLock(player->pMtx);
            // 2. Update player data
            player->mapName = newMap;
            if (j.contains("position")) {
                auto pos = j["position"];
                player->lastPos.x = pos.value("x", 0.0f);
                player->lastPos.y = pos.value("y", 0.0f);
                player->lastPos.z = pos.value("z", 0.0f);
            }
        }
        data->mapName = newMap;
        
        // 3. Confirm to player
        json response = {
            {"type", "map_changed"},
            {"map_name", newMap},
            {"position", j.contains("position") ? j["position"] : json::object()},
            {"rotation_y", j.value("rotation_y", 0.0f)}
        };
        ws->send(response.dump(), uWS::OpCode::TEXT);

        // 3b. Send game objects to player
        syncGameObjects(ws, newMap);
        
        // 4. Save to DB
        Database::getInstance().savePlayer(*player);

        // 5. Synchronization with other players on the new map
        auto allPlayers = GameState::getInstance().getPlayersSnapshot();
        for (auto const& p : allPlayers) {
            if (p->username != player->username && p->mapName == newMap && !p->isInvisible) {
                // Tell THIS player about p
                json moveMsg = {
                    {"type", "player_moved"},
                    {"username", p->charName},
                    {"char_class", p->characterClass},
                    {"position", {{"x", p->lastPos.x}, {"y", p->lastPos.y}, {"z", p->lastPos.z}}},
                    {"rotation", {{"x", p->rotation.x}, {"y", p->rotation.y}, {"z", p->rotation.z}}},
                    {"is_gm", p->isGMFlagged}
                };
                ws->send(moveMsg.dump(), uWS::OpCode::TEXT);
                
                // Tell p about THIS player
                json enterMsg = {
                    {"type", "player_moved"},
                    {"username", player->charName},
                    {"char_class", player->characterClass},
                    {"position", j.contains("position") ? j["position"] : json::object()},
                    {"rotation", {{"x", player->rotation.x}, {"y", player->rotation.y}, {"z", player->rotation.z}}},
                    {"is_gm", player->isGMFlagged}
                };
                auto clientWs = (uWS::WebSocket<false, true, PerSocketData>*)p->ws;
                if (clientWs && !p->isDisconnected) {
                    clientWs->send(enterMsg.dump(), uWS::OpCode::TEXT);
                }
            }
        }
        Logger::log("[MAP] Change success: " + data->username);
    } else {
        Logger::log("[MAP] ERROR: Player not found for session " + data->username);
    }
}

void SocketHandlers::handleSpellCast(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j) {
    auto data = ws->getUserData();
    if (!data) return;
    
    std::string spell = j.value("spell_id", "");
    std::string targetId = j.value("target_id", "");
    
    auto player = GameState::getInstance().getPlayer(data->username);
    if (!player) return;

    AbilityManager::getInstance().startCasting(*player, spell, targetId);
}


void SocketHandlers::handlePartyInvite(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j) {
    auto data = ws->getUserData();
    if (!data) return;

    std::string targetCharName = j.value("target_name", "");
    if (targetCharName.empty() || targetCharName == data->charName) return;

    auto sender = GameState::getInstance().getPlayer(data->username);
    if (!sender) return;

    std::shared_ptr<Player> target = nullptr;
    auto players = GameState::getInstance().getPlayersSnapshot();
    for (auto const& p : players) {
        if (p->charName == targetCharName) {
            target = p;
            break;
        }
    }

    if (!target) {
        ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Spieler \"" + targetCharName + "\" nicht gefunden."}}.dump(), uWS::OpCode::TEXT);
        return;
    }

    if (!target->partyId.empty()) {
        ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", targetCharName + " ist bereits in einer Gruppe."}}.dump(), uWS::OpCode::TEXT);
        return;
    }

    if (!sender->partyId.empty()) {
        auto party = GameState::getInstance().getParty(sender->partyId);
        if (party) {
            if (party->leaderName != sender->username) {
                ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Nur der Gruppenleiter kann einladen."}}.dump(), uWS::OpCode::TEXT);
                return;
            }
            if (party->members.size() >= 5) {
                ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Die Gruppe ist bereits voll."}}.dump(), uWS::OpCode::TEXT);
                return;
            }
        }
    }

    json inviteReq = {{"type", "party_invite_request"}, {"from", sender->charName}};
    auto targetWs = (uWS::WebSocket<false, true, PerSocketData>*)target->ws;
    if (targetWs && !target->isDisconnected) {
        targetWs->send(inviteReq.dump(), uWS::OpCode::TEXT);
        ws->send(json{{"type", "chat_receive"}, {"mode", "system"}, {"message", "Einladung an " + targetCharName + " gesendet."}}.dump(), uWS::OpCode::TEXT);
    }
}

void SocketHandlers::handlePartyResponse(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j) {
    auto data = ws->getUserData();
    if (!data) return;

    bool accept = j.value("accept", false);
    std::string fromCharName = j.value("from", "");

    auto responder = GameState::getInstance().getPlayer(data->username);
    if (!responder) return;

    if (!accept) return;

    std::shared_ptr<Player> sender = nullptr;
    auto players = GameState::getInstance().getPlayersSnapshot();
    for (auto const& p : players) {
        if (p->charName == fromCharName) {
            sender = p;
            break;
        }
    }

    if (!sender || sender->isDisconnected) return;

    std::shared_ptr<Party> party = nullptr;
    if (sender->partyId.empty()) {
        party = std::make_shared<Party>();
        party->id = "party_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        party->leaderName = sender->username;
        party->members.push_back(sender->username);
        sender->partyId = party->id;
        GameState::getInstance().addParty(party);
    } else {
        party = GameState::getInstance().getParty(sender->partyId);
    }

    if (party && party->members.size() < 5) {
        party->members.push_back(responder->username);
        responder->partyId = party->id;
        
        json update = {{"type", "party_update"}};
        json memberList = json::array();
        for (auto const& mName : party->members) {
            auto p = GameState::getInstance().getPlayer(mName);
            if (p) {
                memberList.push_back({
                    {"username", p->username},
                    {"name", p->charName}, {"hp", p->hp}, {"max_hp", p->maxHp}, 
                    {"mana", p->mana}, {"max_mana", p->maxMana},
                    {"level", p->level}, {"is_leader", p->username == party->leaderName},
                    {"target", p->currentTargetId}
                });
            }
        }
        update["members"] = memberList;
        
        for (auto const& mName : party->members) {
            auto p = GameState::getInstance().getPlayer(mName);
            if (p && !p->isDisconnected) {
                auto mWs = (uWS::WebSocket<false, true, PerSocketData>*)p->ws;
                if (mWs) mWs->send(update.dump(), uWS::OpCode::TEXT);
            }
        }
    }
}

void SocketHandlers::handlePartyLeave(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j) {
    auto data = ws->getUserData();
    if (!data) return;

    auto player = GameState::getInstance().getPlayer(data->username);
    if (!player || player->partyId.empty()) return;

    auto party = GameState::getInstance().getParty(player->partyId);
    if (party) {
        party->members.erase(std::remove(party->members.begin(), party->members.end(), player->username), party->members.end());
        player->partyId = "";

        if (party->members.empty()) {
            GameState::getInstance().removeParty(party->id);
        } else {
            if (party->leaderName == player->username) {
                party->leaderName = party->members[0];
            }
            // Update remaining
            json update = {{"type", "party_update"}};
            json memberList = json::array();
            for (auto const& mName : party->members) {
                auto p = GameState::getInstance().getPlayer(mName);
                if (p) {
                    memberList.push_back({
                        {"username", p->username},
                        {"name", p->charName}, {"hp", p->hp}, {"max_hp", p->maxHp}, 
                        {"mana", p->mana}, {"max_mana", p->maxMana},
                        {"level", p->level}, {"is_leader", p->username == party->leaderName},
                        {"target", p->currentTargetId}
                    });
                }
            }
            update["members"] = memberList;
            for (auto const& mName : party->members) {
                auto p = GameState::getInstance().getPlayer(mName);
                if (p && !p->isDisconnected) {
                    auto mWs = (uWS::WebSocket<false, true, PerSocketData>*)p->ws;
                    if (mWs) mWs->send(update.dump(), uWS::OpCode::TEXT);
                }
            }
        }
    }
    ws->send(json{{"type", "party_update"}, {"members", json::array()}}.dump(), uWS::OpCode::TEXT);
}

void SocketHandlers::handlePartyKick(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j) {
    auto data = ws->getUserData();
    if (!data) return;

    std::string targetCharName = j.value("target_name", "");
    auto leader = GameState::getInstance().getPlayer(data->username);
    if (!leader || leader->partyId.empty()) return;

    auto party = GameState::getInstance().getParty(leader->partyId);
    if (!party || party->leaderName != leader->username) return;

    auto players = GameState::getInstance().getPlayersSnapshot();
    std::shared_ptr<Player> target = nullptr;
    for (auto const& p : players) {
        if (p->charName == targetCharName) {
            target = p;
            break;
        }
    }

    if (target && target->partyId == party->id) {
        if (target->username == leader->username) return; // Cannot kick self

        party->members.erase(std::remove(party->members.begin(), party->members.end(), target->username), party->members.end());
        target->partyId = "";
        
        // Notify kicked player
        auto tWs = (uWS::WebSocket<false, true, PerSocketData>*)target->ws;
        if (tWs) tWs->send(json{{"type", "party_update"}, {"members", json::array()}}.dump(), uWS::OpCode::TEXT);

        if (party->members.size() <= 1) {
            // Dissolve party if only one left
            for (auto const& mName : party->members) {
                auto p = GameState::getInstance().getPlayer(mName);
                if (p) {
                    p->partyId = "";
                    auto mWs = (uWS::WebSocket<false, true, PerSocketData>*)p->ws;
                    if (mWs) mWs->send(json{{"type", "party_update"}, {"members", json::array()}}.dump(), uWS::OpCode::TEXT);
                }
            }
            GameState::getInstance().removeParty(party->id);
        } else {
            // Update remaining
            json update = {{"type", "party_update"}};
            json memberList = json::array();
            for (auto const& mName : party->members) {
                auto p = GameState::getInstance().getPlayer(mName);
                if (p) {
                    memberList.push_back({
                        {"username", p->username},
                        {"name", p->charName}, {"hp", p->hp}, {"max_hp", p->maxHp}, 
                        {"mana", p->mana}, {"max_mana", p->maxMana},
                        {"level", p->level}, {"is_leader", p->username == party->leaderName},
                        {"target", p->currentTargetId}
                    });
                }
            }
            update["members"] = memberList;
            for (auto const& mName : party->members) {
                auto p = GameState::getInstance().getPlayer(mName);
                if (p && !p->isDisconnected) {
                    auto mWs = (uWS::WebSocket<false, true, PerSocketData>*)p->ws;
                    if (mWs) mWs->send(update.dump(), uWS::OpCode::TEXT);
                }
            }
        }
    }
}

void SocketHandlers::broadcastToMap(const std::string& mapName, const std::string& message, uWS::WebSocket<false, true, PerSocketData>* exclude) {
    WorldServer::defer([mapName, message, exclude]() {
        auto players = GameState::getInstance().getPlayersSnapshot();
        for (auto const& player : players) {
            std::string pMap;
            bool connected;
            {
                std::lock_guard<std::recursive_mutex> pLock(player->pMtx);
                pMap = player->mapName;
                connected = (player->ws != nullptr && !player->isDisconnected);
                
                if (connected && pMap == mapName) {
                    auto clientWs = (uWS::WebSocket<false, true, PerSocketData>*)player->ws;
                    if (clientWs && clientWs != exclude) {
                        clientWs->send(message, uWS::OpCode::TEXT);
                    }
                }
            }
        }
    });
}

void SocketHandlers::handleMoveItem(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j) {
    auto data = ws->getUserData();
    if (!data) return;

    auto player = GameState::getInstance().getPlayer(data->username);
    if (!player) return;

    int fromSlot = j.value("from_slot", -1);
    int toSlot = j.value("to_slot", -1);

    if (fromSlot < 0 || toSlot < 0) return;

    {
        std::lock_guard<std::recursive_mutex> pLock(player->pMtx);
        
        ItemInstance* fromItem = nullptr;
        ItemInstance* toItem = nullptr;

        for (auto& item : player->inventory) {
            if (item.slotIndex == fromSlot) fromItem = &item;
            if (item.slotIndex == toSlot) toItem = &item;
        }

        if (fromItem) {
            if (toItem) {
                // Swap slots
                toItem->slotIndex = fromSlot;
            }
            fromItem->slotIndex = toSlot;
            
            Logger::log("[INV] player " + player->charName + " moved item from " + std::to_string(fromSlot) + " to " + std::to_string(toSlot));
            
            // Sync inventory back
            json invMsg = {{"type", "inventory_sync"}, {"items", json::array()}};
            for (const auto& item : player->inventory) {
                auto tmpl = GameState::getInstance().getItemTemplate(item.itemId);
                invMsg["items"].push_back({
                    {"item_id", item.itemId},
                    {"slot", item.slotIndex},
                    {"quantity", item.quantity},
                    {"equipped", item.isEquipped},
                    {"name", tmpl.name},
                    {"description", tmpl.description},
                    {"rarity", tmpl.rarity},
                    {"extra_data", tmpl.componentData}
                });
            }
            ws->send(invMsg.dump(), uWS::OpCode::TEXT);
        }
    }
}

void SocketHandlers::handleUseItem(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j) {
    auto data = ws->getUserData();
    if (!data) return;

    auto player = GameState::getInstance().getPlayer(data->username);
    if (!player) return;

    int slotIdx = j.value("slot_index", -1);
    if (slotIdx < 0) return;

    {
        std::lock_guard<std::recursive_mutex> pLock(player->pMtx);
        
        auto it = std::find_if(player->inventory.begin(), player->inventory.end(), 
            [slotIdx](const ItemInstance& item) { return item.slotIndex == slotIdx; });

        if (it != player->inventory.end()) {
            std::string slug = it->itemId;
            bool consumed = false;

            if (slug == "2") { // Item ID 2: Health Potion (Full Heal)
                player->hp = player->maxHp;
                player->lastStatusSync = 0; // Trigger status update
                Logger::log("[ITEM] " + player->charName + " used Item ID 2 (Full Heal)");
                consumed = true;
            }

            if (consumed) {
                it->quantity--;
                if (it->quantity <= 0) {
                    player->inventory.erase(it);
                }
                
                // Sync inventory
                json invMsg = {{"type", "inventory_sync"}, {"items", json::array()}};
                for (const auto& item : player->inventory) {
                    auto tmpl = GameState::getInstance().getItemTemplate(item.itemId);
                    invMsg["items"].push_back({
                        {"item_id", item.itemId},
                        {"slot", item.slotIndex},
                        {"quantity", item.quantity},
                        {"equipped", item.isEquipped},
                        {"name", tmpl.name},
                        {"description", tmpl.description},
                        {"rarity", tmpl.rarity},
                        {"extra_data", tmpl.componentData}
                    });
                }
                ws->send(invMsg.dump(), uWS::OpCode::TEXT);
                
                // Save immediately
                Database::getInstance().savePlayer(*player);
                Database::getInstance().saveInventory(*player);

                // Feedback
                auto tmpl = GameState::getInstance().getItemTemplate(it->itemId);
                std::string itemLink = "[color=#00FFFF][url=item:" + it->itemId + "]" + tmpl.name + "[/url][/color]";
                json msg = {{"type", "chat_receive"}, {"mode", "system"}, {"message", "Du hast " + itemLink + " benutzt."}};
                ws->send(msg.dump(), uWS::OpCode::TEXT);
            }
        }
    }
}

void SocketHandlers::handleDestroyItem(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j) {
    auto data = ws->getUserData();
    if (!data) return;

    auto player = GameState::getInstance().getPlayer(data->username);
    if (!player) return;

    int slotIdx = j.value("slot_index", -1);
    if (slotIdx < 0) return;

    {
        std::lock_guard<std::recursive_mutex> pLock(player->pMtx);
        
        auto it = std::find_if(player->inventory.begin(), player->inventory.end(), 
            [slotIdx](const ItemInstance& item) { return item.slotIndex == slotIdx; });

        if (it != player->inventory.end()) {
            std::string itemName = GameState::getInstance().getItemTemplate(it->itemId).name;
            player->inventory.erase(it);
            
            Logger::log("[INV] player " + player->charName + " destroyed item in slot " + std::to_string(slotIdx));
            
            // Sync inventory
            json invMsg = {{"type", "inventory_sync"}, {"items", json::array()}};
            for (const auto& item : player->inventory) {
                auto tmpl = GameState::getInstance().getItemTemplate(item.itemId);
                invMsg["items"].push_back({
                    {"item_id", item.itemId},
                    {"slot", item.slotIndex},
                    {"quantity", item.quantity},
                    {"equipped", item.isEquipped},
                    {"name", tmpl.name},
                    {"description", tmpl.description},
                    {"rarity", tmpl.rarity},
                    {"extra_data", tmpl.componentData}
                });
            }
            ws->send(invMsg.dump(), uWS::OpCode::TEXT);
            
            // Save immediately
            Database::getInstance().saveInventory(*player);

            // Feedback
            std::string itemLink = "[color=#00FFFF][url=item:" + it->itemId + "]" + itemName + "[/url][/color]";
            json msg = {{"type", "chat_receive"}, {"mode", "system"}, {"message", "Gegenstand " + itemLink + " wurde zerstört."}};
            ws->send(msg.dump(), uWS::OpCode::TEXT);
        }
    }
}

// --- Trading Helpers ---

static void sendTradeUpdate(std::shared_ptr<Player> p1, std::shared_ptr<Player> p2, std::shared_ptr<Trade> trade) {
    auto formatItems = [](const std::vector<ItemInstance>& items) {
        json arr = json::array();
        for (const auto& item : items) {
            auto tmpl = GameState::getInstance().getItemTemplate(item.itemId);
            arr.push_back({
                {"item_id", item.itemId},
                {"quantity", item.quantity},
                {"name", tmpl.name},
                {"description", tmpl.description},
                {"rarity", tmpl.rarity},
                {"extra_data", tmpl.componentData}
            });
        }
        return arr;
    };

    if (p1 && p1->ws) {
        json j1 = {
            {"type", "trade_update"},
            {"my_items", formatItems(trade->items1)},
            {"partner_items", formatItems(trade->items2)},
            {"my_ready", trade->ready1},
            {"partner_ready", trade->ready2}
        };
        ((uWS::WebSocket<false, true, PerSocketData>*)p1->ws)->send(j1.dump(), uWS::OpCode::TEXT);
    }

    if (p2 && p2->ws) {
        json j2 = {
            {"type", "trade_update"},
            {"my_items", formatItems(trade->items2)},
            {"partner_items", formatItems(trade->items1)},
            {"my_ready", trade->ready2},
            {"partner_ready", trade->ready1}
        };
        ((uWS::WebSocket<false, true, PerSocketData>*)p2->ws)->send(j2.dump(), uWS::OpCode::TEXT);
    }
}

void SocketHandlers::handleTradeRequest(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j) {
    auto data = ws->getUserData();
    auto player = GameState::getInstance().getPlayer(data->username);
    std::string targetUsername = j.value("target", ""); // Technical ID (username)
    
    auto target = GameState::getInstance().getPlayer(targetUsername);
    if (!target || !target->ws || target->username == player->username) {
        json failure = {{"type", "chat_receive"}, {"mode", "system"}, {"message", "Handel fehlgeschlagen: Spieler nicht gefunden oder ungültig."}};
        ws->send(failure.dump(), uWS::OpCode::TEXT);
        return;
    }

    // TODO: Check distance

    json msg = {
        {"type", "trade_invited"}, 
        {"from_user", player->username}, 
        {"from_char", player->charName}
    };
    ((uWS::WebSocket<false, true, PerSocketData>*)target->ws)->send(msg.dump(), uWS::OpCode::TEXT);
    
    Logger::log("[TRADE] " + player->charName + " invited " + target->charName);
}

void SocketHandlers::handleTradeResponse(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j) {
    auto data = ws->getUserData();
    auto player = GameState::getInstance().getPlayer(data->username);
    std::string partnerUsername = j.value("partner", ""); // Technical ID (username)
    bool accepted = j.value("accepted", false);

    auto partner = GameState::getInstance().getPlayer(partnerUsername);
    if (!partner || !partner->ws) return;

    if (accepted) {
        auto trade = std::make_shared<Trade>();
        trade->id = player->username + "_" + partner->username;
        trade->p1 = player->username;
        trade->p2 = partner->username;
        GameState::getInstance().addTrade(trade);

        json msg1 = {{"type", "trade_started"}, {"partner", partner->charName}};
        ws->send(msg1.dump(), uWS::OpCode::TEXT);

        json msg2 = {{"type", "trade_started"}, {"partner", player->charName}};
        ((uWS::WebSocket<false, true, PerSocketData>*)partner->ws)->send(msg2.dump(), uWS::OpCode::TEXT);
        
        Logger::log("[TRADE] Started between " + player->charName + " and " + partner->charName);
        sendTradeUpdate(player, partner, trade);
    }
}

void SocketHandlers::handleTradeAddItem(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j) {
    auto data = ws->getUserData();
    auto player = GameState::getInstance().getPlayer(data->username);
    auto trade = GameState::getInstance().getTradeForPlayer(player->username);
    if (!trade) return;

    int slotIdx = j.value("slot_index", -1);
    
    std::lock_guard<std::recursive_mutex> pLock(player->pMtx);
    auto it = std::find_if(player->inventory.begin(), player->inventory.end(), 
        [slotIdx](const ItemInstance& item) { return item.slotIndex == slotIdx; });

    if (it != player->inventory.end()) {
        auto& items = (trade->p1 == player->username) ? trade->items1 : trade->items2;
        if (items.size() < 8) {
            items.push_back(*it);
            trade->ready1 = trade->ready2 = false; // Reset ready on change
            trade->confirmed1 = trade->confirmed2 = false;
            
            auto p2name = (trade->p1 == player->username) ? trade->p2 : trade->p1;
            auto p2 = GameState::getInstance().getPlayer(p2name);
            sendTradeUpdate(player, p2, trade);
        }
    }
}

void SocketHandlers::handleTradeRemoveItem(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j) {
    auto data = ws->getUserData();
    auto player = GameState::getInstance().getPlayer(data->username);
    auto trade = GameState::getInstance().getTradeForPlayer(player->username);
    if (!trade) return;

    int tradeSlotIdx = j.value("trade_slot", -1);
    auto& items = (trade->p1 == player->username) ? trade->items1 : trade->items2;
    
    if (tradeSlotIdx >= 0 && tradeSlotIdx < (int)items.size()) {
        items.erase(items.begin() + tradeSlotIdx);
        trade->ready1 = trade->ready2 = false; 
        trade->confirmed1 = trade->confirmed2 = false;

        auto p2name = (trade->p1 == player->username) ? trade->p2 : trade->p1;
        auto p2 = GameState::getInstance().getPlayer(p2name);
        sendTradeUpdate(player, p2, trade);
    }
}

void SocketHandlers::handleTradeReady(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j) {
    auto data = ws->getUserData();
    auto player = GameState::getInstance().getPlayer(data->username);
    auto trade = GameState::getInstance().getTradeForPlayer(player->username);
    if (!trade) return;

    bool ready = j.value("ready", false);
    if (trade->p1 == player->username) trade->ready1 = ready;
    else trade->ready2 = ready;

    trade->confirmed1 = trade->confirmed2 = false;

    auto p2name = (trade->p1 == player->username) ? trade->p2 : trade->p1;
    auto p2 = GameState::getInstance().getPlayer(p2name);
    sendTradeUpdate(player, p2, trade);
}

void SocketHandlers::handleTradeConfirm(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j) {
    auto data = ws->getUserData();
    auto p1 = GameState::getInstance().getPlayer(data->username);
    auto trade = GameState::getInstance().getTradeForPlayer(p1->username);
    if (!trade || !trade->ready1 || !trade->ready2) return;

    if (trade->p1 == p1->username) trade->confirmed1 = true;
    else trade->confirmed2 = true;

    if (trade->confirmed1 && trade->confirmed2) {
        // PERFOM TRADE
        auto p2 = GameState::getInstance().getPlayer(trade->p1 == p1->username ? trade->p2 : trade->p1);
        
        {
            std::lock_guard<std::recursive_mutex> l1(p1->pMtx);
            std::lock_guard<std::recursive_mutex> l2(p2->pMtx);

            // Verify items still exist in inventory
            auto verify = [](std::shared_ptr<Player> p, const std::vector<ItemInstance>& items) {
                for (const auto& ti : items) {
                    auto it = std::find_if(p->inventory.begin(), p->inventory.end(), 
                        [&ti](const ItemInstance& inv) { return inv.itemId == ti.itemId && inv.slotIndex == ti.slotIndex && inv.quantity >= ti.quantity; });
                    if (it == p->inventory.end()) return false;
                }
                return true;
            };

            if (verify(p1, (trade->p1 == p1->username ? trade->items1 : trade->items2)) &&
                verify(p2, (trade->p2 == p2->username ? trade->items2 : trade->items1))) 
            {
                // Transfer items
                auto transfer = [](std::shared_ptr<Player> from, std::shared_ptr<Player> to, const std::vector<ItemInstance>& items) {
                    for (const auto& ti : items) {
                        // Remove from sender
                        auto it = std::find_if(from->inventory.begin(), from->inventory.end(), 
                            [&ti](const ItemInstance& inv) { return inv.itemId == ti.itemId && inv.slotIndex == ti.slotIndex; });
                        if (it != from->inventory.end()) {
                            it->quantity -= ti.quantity;
                            if (it->quantity <= 0) from->inventory.erase(it);
                        }

                        // Add to receiver (find free slot)
                        int freeSlot = -1;
                        for (int s = 0; s < 30; ++s) { // Max 30 slots
                            bool taken = false;
                            for (const auto& i : to->inventory) if (i.slotIndex == s) taken = true;
                            if (!taken) { freeSlot = s; break; }
                        }
                        if (freeSlot != -1) {
                            ItemInstance ni = ti;
                            ni.slotIndex = freeSlot;
                            ni.isEquipped = false;
                            to->inventory.push_back(ni);
                        }
                    }
                };

                transfer(p1, p2, (trade->p1 == p1->username ? trade->items1 : trade->items2));
                transfer(p2, p1, (trade->p1 == p1->username ? trade->items2 : trade->items1));

                Database::getInstance().saveInventory(*p1);
                Database::getInstance().saveInventory(*p2);

                json success = {{"type", "trade_complete"}};
                ((uWS::WebSocket<false, true, PerSocketData>*)p1->ws)->send(success.dump(), uWS::OpCode::TEXT);
                ((uWS::WebSocket<false, true, PerSocketData>*)p2->ws)->send(success.dump(), uWS::OpCode::TEXT);

                // Trigger inventory sync
                // (Existing handleInventorySync logic needed here, or just manual sync message)
                auto sendInv = [](std::shared_ptr<Player> p) {
                    json invMsg = {{"type", "inventory_sync"}, {"items", json::array()}};
                    for (const auto& item : p->inventory) {
                        auto tmpl = GameState::getInstance().getItemTemplate(item.itemId);
                        invMsg["items"].push_back({
                            {"item_id", item.itemId}, {"slot", item.slotIndex}, {"quantity", item.quantity},
                            {"equipped", item.isEquipped}, {"name", tmpl.name}, {"description", tmpl.description},
                            {"rarity", tmpl.rarity}, {"extra_data", tmpl.componentData}
                        });
                    }
                    ((uWS::WebSocket<false, true, PerSocketData>*)p->ws)->send(invMsg.dump(), uWS::OpCode::TEXT);
                };
                sendInv(p1);
                sendInv(p2);
            }
        }
        GameState::getInstance().removeTrade(trade->id);
    }
}

void SocketHandlers::handleTradeCancel(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j) {
    auto data = ws->getUserData();
    auto p1 = GameState::getInstance().getPlayer(data->username);
    auto trade = GameState::getInstance().getTradeForPlayer(p1->username);
    if (!trade) return;

    auto p2name = (trade->p1 == p1->username) ? trade->p2 : trade->p1;
    auto p2 = GameState::getInstance().getPlayer(p2name);

    GameState::getInstance().removeTrade(trade->id);

    json msg = {{"type", "trade_canceled"}};
    ws->send(msg.dump(), uWS::OpCode::TEXT);
    if (p2 && p2->ws) ((uWS::WebSocket<false, true, PerSocketData>*)p2->ws)->send(msg.dump(), uWS::OpCode::TEXT);
}

void SocketHandlers::syncGameObjects(uWS::WebSocket<false, true, PerSocketData>* ws, const std::string& mapName) {
    auto objects = GameState::getInstance().getGameObjects(mapName);
    json objectsMsg = {{"type", "game_objects_init"}, {"objects", json::array()}};
    for (auto const& obj : objects) {
        objectsMsg["objects"].push_back({
            {"id", obj.id},
            {"type", obj.type},
            {"position", {{"x", obj.position.x}, {"y", obj.position.y}, {"z", obj.position.z}}},
            {"rotation", {{"x", obj.rotation.x}, {"y", obj.rotation.y}, {"z", obj.rotation.z}}},
            {"extra_data", obj.extraData}
        });
    }
    ws->send(objectsMsg.dump(), uWS::OpCode::TEXT);
}

void SocketHandlers::sendSafe(uWS::WebSocket<false, true, PerSocketData>* ws, const std::string& message) {
    if (!ws) return;
    WorldServer::defer([ws, message]() {
        // Warning: ws might be dangling here if not carefully managed.
        // Use sendToPlayer for background tasks instead.
        ws->send(message, uWS::OpCode::TEXT);
    });
}
void SocketHandlers::handleQuestInteract(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j) {
    auto data = ws->getUserData();
    if (!data) return;

    int npcId = j.value("npc_id", -1);
    auto player = GameState::getInstance().getPlayer(data->username);
    if (!player) return;

    auto objs = GameState::getInstance().getGameObjects(player->mapName);
    GameObject* npc = nullptr;
    for (auto& obj : objs) {
        if (obj.id == npcId && obj.type == "quest_giver") {
            npc = &obj;
            break;
        }
    }

    if (!npc) return;

    std::string questId = npc->extraData.value("quest_id", "");
    if (questId.empty()) return;

    QuestTemplate qt = GameState::getInstance().getQuestTemplate(questId);
    
    // Check if player has quest or already completed it
    PlayerQuest* pq = nullptr;
    {
        std::lock_guard<std::recursive_mutex> lock(player->pMtx);
        for (auto& q : player->quests) {
            if (q.questId == questId) {
                pq = &q;
                break;
            }
        }
    }

    json response = {
        {"type", "quest_info"},
        {"quest_id", questId},
        {"title", qt.title},
        {"description", qt.description},
        {"objectives", qt.objectives}
    };

    if (pq) {
        response["status"] = pq->status;
        response["progress"] = pq->progress;
    } else {
        response["status"] = "available";
    }

    ws->send(response.dump(), uWS::OpCode::TEXT);
}

void SocketHandlers::handleQuestAccept(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j) {
    auto data = ws->getUserData();
    if (!data) return;

    std::string questId = j.value("quest_id", "");
    auto player = GameState::getInstance().getPlayer(data->username);
    if (!player || questId.empty()) return;

    {
        std::lock_guard<std::recursive_mutex> lock(player->pMtx);
        for (auto const& q : player->quests) {
            if (q.questId == questId) return; // Already have it
        }
        
        PlayerQuest newPq;
        newPq.questId = questId;
        newPq.status = "active";
        // Progress defaults to empty/0
        player->quests.push_back(newPq);
    }

    QuestTemplate qt = GameState::getInstance().getQuestTemplate(questId);
    json response = {
        {"type", "quest_accepted"},
        {"quest_id", questId},
        {"title", qt.title},
        {"description", qt.description},
        {"objectives", qt.objectives}
    };
    ws->send(response.dump(), uWS::OpCode::TEXT);
    Database::getInstance().saveQuests(*player);
}

void SocketHandlers::handleQuestReward(uWS::WebSocket<false, true, PerSocketData>* ws, const json& j) {
    auto data = ws->getUserData();
    if (!data) return;

    std::string questId = j.value("quest_id", "");
    auto player = GameState::getInstance().getPlayer(data->username);
    if (!player || questId.empty()) return;

    PlayerQuest* pq = nullptr;
    {
        std::lock_guard<std::recursive_mutex> lock(player->pMtx);
        for (auto& q : player->quests) {
            if (q.questId == questId) {
                pq = &q;
                break;
            }
        }
    }

    if (!pq || pq->status != "completed") return;

    QuestTemplate qt = GameState::getInstance().getQuestTemplate(questId);
    
    // XP Belohnung berechnen
    int rewardXp = qt.rewardXpBase;
    if (player->level >= 60) {
        rewardXp = qt.rewardXpMax;
    } else if (player->level > 1) {
        // Lineare Interplolation oder einfach base
        // rewardXp = (int)(qt.rewardXpBase + (player->level - 1) * ((qt.rewardXpMax - qt.rewardXpBase) / 59.0));
    }

    pq->status = "rewarded";
    ws->send(json{{"type", "quest_rewarded"}, {"quest_id", questId}}.dump(), uWS::OpCode::TEXT);
    
    GameLogic::awardXP(*player, rewardXp);
    Database::getInstance().saveQuests(*player);
}

void SocketHandlers::sendToPlayer(const std::string& username, const std::string& message) {
    WorldServer::defer([username, message]() {
        auto player = GameState::getInstance().getPlayer(username);
        if (player) {
            void* pWs = nullptr;
            bool connected = false;
            {
                std::lock_guard<std::recursive_mutex> pLock(player->pMtx);
                pWs = player->ws;
                connected = !player->isDisconnected;
                if (pWs && connected) {
                    auto ws = (uWS::WebSocket<false, true, PerSocketData>*)pWs;
                    ws->send(message, uWS::OpCode::TEXT);
                }
            }
        }
    });
}

void SocketHandlers::syncPlayerStatus(std::shared_ptr<Player> player) {
    if (!player) return;
    
    LevelData ld = GameLogic::getLevelData(player->level);
    json statusMsg;
    std::string syncUsername;
    
    {
        std::lock_guard<std::recursive_mutex> pLock(player->pMtx);
        syncUsername = player->username;
        statusMsg = {
            {"type", "player_status"}, {"username", player->charName},
            {"hp", player->hp}, {"max_hp", player->maxHp},
            {"mana", player->mana}, {"max_mana", player->maxMana},
            {"shield", player->shield}, {"level", player->level},
            {"xp", player->xp}, {"max_xp", ld.xpToNextLevel},
            {"speed_multiplier", player->speedMultiplier},
            {"gravity_enabled", player->gravityEnabled},
            {"is_gm", player->isGMFlagged}
        };
    }
    
    sendToPlayer(syncUsername, statusMsg.dump());
}
