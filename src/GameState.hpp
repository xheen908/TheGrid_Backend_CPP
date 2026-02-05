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
    std::string itemId;
    std::string name;
    std::string description;
    std::string type; // Weapon, Armor, Consumable, etc.
    std::string rarity;
    json componentData;
};

struct ItemInstance {
    std::string itemId;
    int slotIndex;
    int quantity;
    bool isEquipped;
    // Helper to get template data if needed
};

struct QuestObjective {
    std::string type; // "kill", "collect"
    std::string target; // Mob ID or Item ID
    int amount;
};

struct QuestTemplate {
    std::string id;
    std::string title;
    std::string description;
    std::map<std::string, int> objectives; // target_id -> count
    int rewardXpBase;
    int rewardXpMax;
};

struct PlayerQuest {
    std::string questId;
    std::string status; // "active", "completed", "rewarded"
    std::map<std::string, int> progress; // target_id -> current_count
};

struct Trade {
    std::string id;
    std::string p1; // Username
    std::string p2; // Username
    std::vector<ItemInstance> items1;
    std::vector<ItemInstance> items2;
    bool ready1 = false;
    bool ready2 = false;
    bool confirmed1 = false;
    bool confirmed2 = false;
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
    std::vector<PlayerQuest> quests;
    std::vector<std::string> knownAbilities;

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
    std::string modelId;
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
    ItemTemplate getItemTemplate(const std::string& itemId);
    QuestTemplate getQuestTemplate(const std::string& questId);
    std::recursive_mutex& getMtx() { return mtx; }

    void addParty(std::shared_ptr<Party> party);
    void removeParty(const std::string& partyId);
    std::shared_ptr<Party> getParty(const std::string& partyId);
    std::vector<std::shared_ptr<Party>> getPartiesSnapshot();

    int getRespawnRate(const std::string& mapName);
    
    void addTrade(std::shared_ptr<Trade> trade);
    void removeTrade(const std::string& tradeId);
    std::shared_ptr<Trade> getTrade(const std::string& tradeId);
    std::shared_ptr<Trade> getTradeForPlayer(const std::string& username);

    std::map<std::string, json> mapSettings;

private:
    GameState();
    std::map<std::string, std::shared_ptr<Player>> activePlayers;
    std::vector<Mob> mobs;
    std::map<std::string, ItemTemplate> itemTemplates;
    std::map<std::string, QuestTemplate> questTemplates;
    std::map<std::string, std::vector<GameObject>> gameObjects; // map_name -> current objects
    std::map<std::string, std::shared_ptr<Party>> parties;
    std::map<std::string, std::shared_ptr<Trade>> activeTrades;
    std::recursive_mutex mtx;
};

#endif
