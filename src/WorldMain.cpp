#include "Server.hpp"
#include "Database.hpp"
#include "Logger.hpp"
#include "EnvLoader.hpp"
#include "GameState.hpp"
#include <iostream>

int main() {
    Logger::init("world_server.log");
    EnvLoader::load(".env");

    Logger::log("=========================================");
    Logger::log("Starting The Grid WORLD SERVER...");
    Logger::log("=========================================");

    // Database initialization (Read from ENV)
    Database::Config config;
    config.host = std::string(getenv("DB_HOST") ? getenv("DB_HOST") : "localhost");
    config.user = std::string(getenv("DB_USER") ? getenv("DB_USER") : "user_name");
    config.pass = std::string(getenv("DB_PASS") ? getenv("DB_PASS") : "user_passwort");
    
    const char* portStr = getenv("DB_PORT");
    if (portStr) {
        config.port = std::stoi(portStr);
    } else {
        const char* mysqlPortStr = getenv("MYSQL_PORT");
        if (mysqlPortStr) config.port = std::stoi(mysqlPortStr);
    }

    Logger::log("[DB] Config: Host=" + config.host + ", User=" + config.user);

    if (!Database::getInstance().connect(config)) {
        Logger::log("[FATAL] Could not connect to MySQL.");
        return 1;
    }

    // Load static world data from DB
    GameState::getInstance().loadWorldData();

    WorldServer server;
    // World Server listens on 3001
    server.start(3001);

    return 0;
}
