#include "Server.hpp"
#include "Database.hpp"
#include "Logger.hpp"
#include <iostream>
#include <filesystem>

void initDefaultData() {
    auto& db = Database::getInstance();
    auto& users = db.getUsers();
    auto& characters = db.getCharacters();

    if (users.empty()) {
        Logger::log("[DB] Initializing default data...");
        users.push_back({{"id", 1}, {"username", "xheen908"}, {"password_hash", "password"}, {"gm_status", true}});
        users.push_back({{"id", 2}, {"username", "admin"}, {"password_hash", "admin"}, {"gm_status", true}});
        users.push_back({{"id", 3}, {"username", "client"}, {"password_hash", "client"}});

        characters.push_back({
            {"id", 101}, {"user_id", 3}, {"char_name", "Fheen"},
            {"level", 10}, {"xp", 5000},
            {"world_state", {"map_name", "WorldMap0"}},
            {"transform", {"position_x", 100.5}, {"position_y", 200.0}, {"position_z", 0.0}}
        });
        characters.push_back({
            {"id", 102}, {"user_id", 2}, {"char_name", "Xheen"},
            {"level", 50}, {"xp", 999999},
            {"world_state", {"map_name", "WorldMap0"}},
            {"transform", {"position_x", 0.0}, {"position_y", 0.0}, {"position_z", 0.0}}
        });
        db.saveData("db_persistence.json");
    }
}

int main() {
    // Force log to project root
    Logger::init("server.log");
    Logger::log("=========================================");
    Logger::log("Starting The Grid C++ Backend...");
    Logger::log("=========================================");

    Database::getInstance().loadData("db_persistence.json");
    initDefaultData();

    GameServer server;
    server.start(3000);

    return 0;
}
