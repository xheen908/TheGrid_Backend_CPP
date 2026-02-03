#ifndef GAMESTATE_HPP
#define GAMESTATE_HPP

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Vector3 {
    float x, y, z;
};

struct Rotation {
    float x, y, z;
};

struct Debuff {
    std::string type;
    long long endTime;
};

struct ItemTemplate {
    std::string slug;
    std::string name;
    std::string description;
    std::string type; // Weapon, Armor, Consumable, etc.
    std::string rarity;
    json componentData;
};

struct ItemInstance {
    std::string itemSlug;
    int slotIndex;
    int quantity;
    bool isEquipped;
    // Helper to get template data if needed
};

struct Party {
    std::string id;
    std::string leaderName;
    std::vector<std::string> members; // Usernames
    long long emptySince = 0; // Time when last member went offline
};

struct Player {
    int dbId = 0;
    std::string username;
    std::string charName;
    std::string characterClass = "Mage";
    int level;
    int hp;
    int maxHp;
    int mana = 100;
    int maxMana = 100;
    int xp = 0;
    int shield = 0;
    std::string mapName;
    Vector3 lastPos;
    Rotation rotation;
    bool isGM = false;
    bool isGMFlagged = false;
    bool isInvisible = false;
    bool gravityEnabled = true;
    float speedMultiplier = 1.0f;
    bool isDisconnected = false;
    void* ws = nullptr; 

    // Combat state
    bool isCasting = false;
    bool isMoving = false;
    long long castEnd = 0;
    std::string currentSpell;
    std::string currentTargetId;
    long long gcdUntil = 0;
    std::map<std::string, long long> cooldowns;
    std::vector<Debuff> buffs;
    long long logoutTimer = 0;
    long long lastStatusSync = 0;
    long long lastPartySync = 0;
    std::string partyId = "";
    long long disconnectTime = 0;
    
    std::vector<ItemInstance> inventory;

    mutable std::recursive_mutex pMtx; // Protects strings and vectors in this struct
};


struct GameObject {
    int id;
    std::string mapName;
    std::string type;
    Vector3 position;
    Rotation rotation;
    json extraData;
};

struct Mob {
    std::string id;
    std::string name;
    int level;
    int hp;
    int maxHp;
    int dbLevel; // Original Level from DB
    int dbMaxHp; // Original HP from DB for scaling
    std::string mobType; // Normal, Elite, Rare, Boss
    std::string mapName;
    Vector3 transform;
    float rotation;
    Vector3 home;
    std::string target;
    long long lastAttack = 0;
    long long respawnAt = 0;
    std::vector<Debuff> debuffs;
};

class GameState {
public:
    static GameState& getInstance();

    void loadWorldData();
    void addPlayer(const std::string& username, std::shared_ptr<Player> player);
    void removePlayer(const std::string& username);
    std::shared_ptr<Player> getPlayer(const std::string& username);
    
    // Thread-safe snapshot of active players
    std::vector<std::shared_ptr<Player>> getPlayersSnapshot();
    std::vector<Mob> getMobsSnapshot();
    
    // Returns ref to mobs, but caller MUST handle locking if they keep the reference
    // Better: return a copy or handle locking inside.
    // For simplicity of porting existing logic:
    std::vector<Mob>& getMobs();
    std::vector<GameObject> getGameObjects(const std::string& mapName);
    std::recursive_mutex& getMtx() { return mtx; }

    void addParty(std::shared_ptr<Party> party);
    void removeParty(const std::string& partyId);
    std::shared_ptr<Party> getParty(const std::string& partyId);
    std::vector<std::shared_ptr<Party>> getPartiesSnapshot();

    int getRespawnRate(const std::string& mapName);
    std::map<std::string, json> mapSettings;

private:
    GameState();
    std::map<std::string, std::shared_ptr<Player>> activePlayers;
    std::vector<Mob> mobs;
    std::map<std::string, ItemTemplate> itemTemplates;
    std::map<std::string, std::vector<GameObject>> gameObjects; // map_name -> current objects
    std::map<std::string, std::shared_ptr<Party>> parties;
    std::recursive_mutex mtx;
};

#endif
