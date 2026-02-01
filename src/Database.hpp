#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <mutex>

using json = nlohmann::json;

// Forward declarations for MySQL to avoid header conflicts
struct st_mysql;
typedef struct st_mysql MYSQL;

class Database {
public:
    static Database& getInstance();

    struct Config {
        std::string host;
        std::string user;
        std::string pass;
        int port = 3306;
    };

    bool connect(const Config& config);

    // --- Auth DB Operations ---
    json authenticate(const std::string& username, const std::string& password);
    bool registerUser(const std::string& username, const std::string& password);
    bool checkGMStatus(const std::string& username);

    // --- Charakter DB Operations ---
    json getCharactersForUser(const std::string& username);
    bool createCharacter(const std::string& username, const std::string& charName, const std::string& characterClass);
    bool deleteCharacter(const std::string& username, int charId);
    bool savePlayer(struct Player& player);
    bool loadCharacter(int charId, struct Player& player);

    // --- World DB Operations ---
    json getWorlds();
    std::vector<struct Mob> loadMobs();
    std::vector<struct GameObject> loadGameObjects(const std::string& mapName);

private:
    Database();
    std::mutex mtx;
    MYSQL* conn = nullptr;
    Config currentConfig;
};

#endif
