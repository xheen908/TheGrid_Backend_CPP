#include "GameState.hpp"
#include "Database.hpp"
#include <iostream>

GameState::GameState() {
    // Basic settings that don't need DB
    mapSettings["WorldMap0"] = {{"respawnRate", 30}};
    mapSettings["Arena0"] = {{"respawnRate", 30}};
    mapSettings["Arena1"] = {{"respawnRate", 30}};
    mapSettings["Arena2"] = {{"respawnRate", 30}};
    mapSettings["Dungeon0"] = {{"respawnRate", 30}};
    mapSettings["TestMap0"] = {{"respawnRate", 30}};
}

void GameState::loadWorldData() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    mobs = Database::getInstance().loadMobs();
    itemTemplates = Database::getInstance().loadItemTemplates();
    questTemplates = Database::getInstance().loadQuestTemplates();
    
    // Load objects for known maps
    for (auto const& [mapName, settings] : mapSettings) {
        gameObjects[mapName] = Database::getInstance().loadGameObjects(mapName);
    }
}

GameState& GameState::getInstance() {
    static GameState instance;
    return instance;
}

void GameState::addPlayer(const std::string& username, std::shared_ptr<Player> player) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    activePlayers[username] = player;
}

void GameState::removePlayer(const std::string& username) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    activePlayers.erase(username);
}

std::shared_ptr<Player> GameState::getPlayer(const std::string& username) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    auto it = activePlayers.find(username);
    if (it != activePlayers.end()) return it->second;
    return nullptr;
}

std::vector<std::shared_ptr<Player>> GameState::getPlayersSnapshot() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    std::vector<std::shared_ptr<Player>> snapshot;
    snapshot.reserve(activePlayers.size());
    for (auto const& [name, p] : activePlayers) {
        snapshot.push_back(p);
    }
    return snapshot;
}

std::vector<Mob> GameState::getMobsSnapshot() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    return mobs;
}

std::vector<Mob>& GameState::getMobs() {
    return mobs;
}

std::vector<GameObject> GameState::getGameObjects(const std::string& mapName) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    if (gameObjects.find(mapName) != gameObjects.end()) {
        return gameObjects.at(mapName);
    }
    return {};
}

ItemTemplate GameState::getItemTemplate(const std::string& itemId) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    if (itemTemplates.find(itemId) != itemTemplates.end()) {
        return itemTemplates.at(itemId);
    }
    ItemTemplate t;
    t.itemId = itemId;
    t.name = "Unknown Item";
    return t;
}

QuestTemplate GameState::getQuestTemplate(const std::string& questId) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    if (questTemplates.find(questId) != questTemplates.end()) {
        return questTemplates.at(questId);
    }
    QuestTemplate q;
    q.id = questId;
    q.title = "Unbekannte Quest";
    return q;
}

int GameState::getRespawnRate(const std::string& mapName) {
    if (mapSettings.find(mapName) != mapSettings.end()) {
        return mapSettings.at(mapName).value("respawnRate", 30);
    }
    return 30; // Default
}

void GameState::addParty(std::shared_ptr<Party> party) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    parties[party->id] = party;
}

void GameState::removeParty(const std::string& partyId) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    parties.erase(partyId);
}

std::shared_ptr<Party> GameState::getParty(const std::string& partyId) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    auto it = parties.find(partyId);
    if (it != parties.end()) return it->second;
    return nullptr;
}

std::vector<std::shared_ptr<Party>> GameState::getPartiesSnapshot() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    std::vector<std::shared_ptr<Party>> snapshot;
    snapshot.reserve(parties.size());
    for (auto const& [id, p] : parties) snapshot.push_back(p);
    return snapshot;
}

void GameState::addTrade(std::shared_ptr<Trade> trade) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    activeTrades[trade->id] = trade;
}

void GameState::removeTrade(const std::string& tradeId) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    activeTrades.erase(tradeId);
}

std::shared_ptr<Trade> GameState::getTrade(const std::string& tradeId) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    auto it = activeTrades.find(tradeId);
    if (it != activeTrades.end()) return it->second;
    return nullptr;
}

std::shared_ptr<Trade> GameState::getTradeForPlayer(const std::string& username) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    for (auto const& [id, t] : activeTrades) {
        if (t->p1 == username || t->p2 == username) return t;
    }
    return nullptr;
}

std::string GameState::getMobName(const std::string& mobId) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    for (const auto& m : mobs) {
        if (m.id == mobId || m.typeId == mobId) return m.name;
    }
    return "Unbekannter Gegner (" + mobId + ")";
}
