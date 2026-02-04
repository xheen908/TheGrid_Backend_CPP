#ifdef WIN32
    #include <winsock2.h>
#endif
#include <mysql.h>
#include "Database.hpp"
#include "GameState.hpp"
#include "GameLogic.hpp"
#include "Logger.hpp"
#include <iostream>
#include <vector>

Database::Database() : conn(nullptr) {}

Database& Database::getInstance() {
    static Database instance;
    return instance;
}

bool Database::connect(const Config& config) {
    std::lock_guard lock(mtx);
    currentConfig = config;
    
    if (conn) {
        mysql_close(conn);
    }
    
    conn = mysql_init(NULL);
    if (conn == NULL) return false;

    if (mysql_real_connect(conn, config.host.c_str(), config.user.c_str(), config.pass.c_str(), 
                           NULL, config.port, NULL, 0) == NULL) {
        Logger::log("[DB] ERROR: Connection failed - " + std::string(mysql_error(conn)) + " (Host: " + config.host + ":" + std::to_string(config.port) + ")");
        mysql_close(conn);
        conn = nullptr;
        return false;
    }

    Logger::log("[DB] Connected to MySQL successfully at " + config.host);
    return true; 
}

json Database::authenticate(const std::string& username, const std::string& password) {
    std::lock_guard lock(mtx);
    if (!conn) return nullptr;

    // Use absolute DB reference: auth_db.users
    std::string query = "SELECT id, gm_status FROM auth_db.users WHERE username='" + username + "' AND password_hash='" + password + "'";
    if (mysql_query(conn, query.c_str())) {
        Logger::log("[DB] Auth Error: " + std::string(mysql_error(conn)));
        return nullptr;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) return nullptr;

    MYSQL_ROW row = mysql_fetch_row(result);
    json response = nullptr;
    if (row) {
        response = {
            {"token", "session_" + username},
            {"username", username},
            {"userId", std::stoi(row[0])},
            {"isGM", std::stoi(row[1]) != 0}
        };
    }
    mysql_free_result(result);
    return response;
}

bool Database::registerUser(const std::string& username, const std::string& password) {
    std::lock_guard lock(mtx);
    if (!conn) return false;
    std::string query = "INSERT INTO auth_db.users (username, password_hash) VALUES ('" + username + "', '" + password + "')";
    if (mysql_query(conn, query.c_str())) {
        Logger::log("[DB] Register Error: " + std::string(mysql_error(conn)));
        return false;
    }
    return true;
}

bool Database::checkGMStatus(const std::string& username) {
    std::lock_guard lock(mtx);
    if (!conn) return false;
    std::string query = "SELECT gm_status FROM auth_db.users WHERE username='" + username + "'";
    if (mysql_query(conn, query.c_str())) return false;
    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) return false;
    MYSQL_ROW row = mysql_fetch_row(result);
    bool status = false;
    if (row) status = std::stoi(row[0]) != 0;
    mysql_free_result(result);
    return status;
}

json Database::getCharactersForUser(const std::string& username) {
    std::lock_guard lock(mtx);
    if (!conn) return json::array();

    // Characters are in charakter_db, Users are in auth_db
    std::string query = "SELECT c.id, c.char_name, c.level, c.map_name, c.hp, c.max_hp, c.pos_x, c.pos_y, c.pos_z, c.character_class "
                        "FROM charakter_db.characters c "
                        "JOIN auth_db.users u ON c.user_id = u.id WHERE u.username = '" + username + "'";
    
    Logger::log("[DB] Fetching chars for " + username);

    if (mysql_query(conn, query.c_str())) {
        Logger::log("[DB] Fetch Chars Error: " + std::string(mysql_error(conn)));
        return json::array();
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) return json::array();

    json chars = json::array();
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        std::string className = row[9] ? row[9] : "Mage";
        std::string role = "Damage";
        std::string color = "#00FF00";

        if (className == "Barbarian") { role = "Tank"; color = "#FF5500"; }
        else if (className == "Knight") { role = "Tank"; color = "#AAAAAA"; }
        else if (className == "Ranger") { role = "Damage"; color = "#55FF55"; }
        else if (className == "Rogue") { role = "Damage"; color = "#FFFF00"; }
        else if (className == "Mage") { role = "Damage"; color = "#00FFFF"; }

        chars.push_back({
            {"id", std::stoi(row[0])},
            {"char_name", row[1]},
            {"level", std::stoi(row[2])},
            {"hp", std::stoi(row[4])},
            {"max_hp", std::stoi(row[5])},
            {"world_state", {{"map_name", row[3]}}},
            {"transform", {{"position_x", std::stof(row[6])}, {"position_y", std::stof(row[7])}, {"position_z", std::stof(row[8])}}},
            {"class_info", {{"class_name", className}, {"class_color", color}, {"role", role}}}
        });
    }
    
    Logger::log("[DB] Found " + std::to_string(chars.size()) + " chars.");
    mysql_free_result(result);
    return chars;
}

bool Database::createCharacter(const std::string& username, const std::string& charName, const std::string& characterClass) {
    std::lock_guard lock(mtx);
    if (!conn) return false;

    // 1. Get user_id
    std::string userQuery = "SELECT id FROM auth_db.users WHERE username='" + username + "'";
    if (mysql_query(conn, userQuery.c_str())) return false;
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return false;
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        mysql_free_result(res);
        return false;
    }
    int userId = std::stoi(row[0]);
    mysql_free_result(res);

    // 2. Insert character
    std::string insertQuery = "INSERT INTO charakter_db.characters (user_id, char_name, character_class) VALUES (" + 
                             std::to_string(userId) + ", '" + charName + "', '" + characterClass + "')";
    
    if (mysql_query(conn, insertQuery.c_str())) {
        Logger::log("[DB] Create Character Error: " + std::string(mysql_error(conn)));
        return false;
    }
    return true;
}

bool Database::deleteCharacter(const std::string& username, int charId) {
    std::lock_guard lock(mtx);
    if (!conn) return false;

    // Simplified query for better compatibility
    std::string query = "DELETE FROM charakter_db.characters WHERE id = " + std::to_string(charId) + 
                        " AND user_id = (SELECT id FROM auth_db.users WHERE username = '" + username + "')";

    Logger::log("[DB] Deleting character " + std::to_string(charId) + " for user " + username);

    if (mysql_query(conn, query.c_str())) {
        Logger::log("[DB] Delete Character Error: " + std::string(mysql_error(conn)));
        return false;
    }

    // Check if anything was actually deleted
    if (mysql_affected_rows(conn) == 0) {
        Logger::log("[DB] Delete Character: No character found or unauthorized for id " + std::to_string(charId));
        return false;
    }

    return true;
}

bool Database::savePlayer(Player& player) {
    std::lock_guard lock(mtx);
    std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
    if (!conn) return false;

    std::string query = "UPDATE charakter_db.characters SET "
                        "level=" + std::to_string(player.level) + ", "
                        "xp=" + std::to_string(player.xp) + ", "
                        "hp=" + std::to_string(player.hp) + ", "
                        "max_hp=" + std::to_string(player.maxHp) + ", "
                        "mana=" + std::to_string(player.mana) + ", "
                        "max_mana=" + std::to_string(player.maxMana) + ", "
                        "pos_x=" + std::to_string(player.lastPos.x) + ", "
                        "pos_y=" + std::to_string(player.lastPos.y) + ", "
                        "pos_z=" + std::to_string(player.lastPos.z) + ", "
                        "map_name='" + player.mapName + "' "
                        "WHERE id=" + std::to_string(player.dbId);

    if (mysql_query(conn, query.c_str())) {
        Logger::log("[DB] Save Error: " + std::string(mysql_error(conn)));
        return false;
    }
    return true;
}

bool Database::loadCharacter(int charId, Player& player) {
    std::lock_guard lock(mtx);
    std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
    if (!conn) return false;

    std::string query = "SELECT c.id, c.char_name, c.level, c.xp, c.hp, c.max_hp, c.mana, c.max_mana, c.map_name, c.pos_x, c.pos_y, c.pos_z, u.gm_status, u.username, c.character_class "
                        "FROM charakter_db.characters c "
                        "JOIN auth_db.users u ON c.user_id = u.id "
                        "WHERE c.id = " + std::to_string(charId);

    if (mysql_query(conn, query.c_str())) {
        Logger::log("[DB] Load Char Error: " + std::string(mysql_error(conn)));
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) return false;

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        player.dbId = std::stoi(row[0]);
        player.charName = row[1];
        player.level = std::stoi(row[2]);
        player.xp = std::stoi(row[3]);
        player.hp = std::stoi(row[4]);
        
        // Stats immer neu berechnen basierend auf dem Level (Global Scaling)
        auto levelData = GameLogic::getLevelData(player.level);
        player.maxHp = levelData.hp;
        player.maxMana = levelData.mana;
        
        // HP Anpassung falls sie durch das neue Scaling über das Limit geht (Initialer Load)
        if (player.hp > player.maxHp) player.hp = player.maxHp;

        player.mana = std::stoi(row[6]);
        if (player.mana > player.maxMana) player.mana = player.maxMana;
        
        player.mapName = row[8];
        player.lastPos = {std::stof(row[9]), std::stof(row[10]), std::stof(row[11])};
        player.isGM = std::stoi(row[12]) != 0;
        if (row[13]) player.username = row[13];
        
        if (row[14]) {
            player.characterClass = row[14];
        } else {
            player.characterClass = "Mage";
        }
    }
    mysql_free_result(result);
    return row != nullptr;
}

json Database::getWorlds() {
    return json::array();
}

std::vector<Mob> Database::loadMobs() {
    std::lock_guard lock(mtx);
    std::vector<Mob> mobList;
    if (!conn) return mobList;

    std::string query = "SELECT mob_id, map_name, level, hp, name, mob_type, pos_x, pos_y, pos_z, model_id FROM world_db.mobs";
    if (mysql_query(conn, query.c_str())) {
        Logger::log("[DB] LoadMobs Error: " + std::string(mysql_error(conn)));
        return mobList;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) return mobList;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        Mob m;
        m.id = row[0];
        m.mapName = row[1];
        m.dbLevel = std::stoi(row[2]);
        if (m.dbLevel < 1) m.dbLevel = 1;
        m.dbMaxHp = std::stoi(row[3]); 
        if (m.dbMaxHp < 1) m.dbMaxHp = 1;

        m.level = m.dbLevel;
        m.maxHp = m.dbMaxHp;
        m.hp = m.maxHp;
        m.name = row[4];
        m.mobType = row[5] ? row[5] : "Normal";
        m.modelId = row[9] ? row[9] : "neon_sphere";

        m.transform = {std::stof(row[6]), std::stof(row[7]), std::stof(row[8])};
        m.home = m.transform;
        m.rotation = 0.0f;
        mobList.push_back(m);
    }
    mysql_free_result(result);
    Logger::log("[DB] Loaded " + std::to_string(mobList.size()) + " mobs from database.");
    return mobList;
}

std::vector<GameObject> Database::loadGameObjects(const std::string& mapName) {
    std::lock_guard lock(mtx);
    std::vector<GameObject> objects;
    if (!conn) return objects;

    std::string query = "SELECT id, type, pos_x, pos_y, pos_z, rot_x, rot_y, rot_z, extra_data FROM world_db.game_objects WHERE map_name = '" + mapName + "'";
    if (mysql_query(conn, query.c_str())) {
        Logger::log("[DB] loadGameObjects Error: " + std::string(mysql_error(conn)));
        return objects;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) return objects;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        GameObject obj;
        obj.id = std::stoi(row[0]);
        obj.mapName = mapName;
        obj.type = row[1];
        obj.position = {std::stof(row[2]), std::stof(row[3]), std::stof(row[4])};
        obj.rotation = {std::stof(row[5]), std::stof(row[6]), std::stof(row[7])};
        
        if (row[8]) {
            try {
                obj.extraData = json::parse(row[8]);
            } catch (...) {
                obj.extraData = json::object();
            }
        } else {
            obj.extraData = json::object();
        }
        
        objects.push_back(obj);
    }
    mysql_free_result(result);
    Logger::log("[DB] Loaded " + std::to_string(objects.size()) + " game objects for map " + mapName);
    return objects;
}

std::map<std::string, ItemTemplate> Database::loadItemTemplates() {
    std::lock_guard lock(mtx);
    std::map<std::string, ItemTemplate> templates;
    if (!conn) return templates;

    std::string query = "SELECT item_id, name, description, type, rarity, component_data FROM world_db.item_templates";
    if (mysql_query(conn, query.c_str())) {
        Logger::log("[DB] loadItemTemplates Error: " + std::string(mysql_error(conn)));
        return templates;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) return templates;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        ItemTemplate t;
        t.itemId = row[0];
        t.name = row[1];
        t.description = row[2] ? row[2] : "";
        t.type = row[3];
        t.rarity = row[4];
        if (row[5]) {
            try { t.componentData = json::parse(row[5]); } 
            catch (...) { t.componentData = json::object(); }
        } else {
            t.componentData = json::object();
        }
        templates[t.itemId] = t;
    }
    mysql_free_result(result);
    Logger::log("[DB] Loaded " + std::to_string(templates.size()) + " item templates.");
    return templates;
}

bool Database::loadInventory(Player& player) {
    std::lock_guard lock(mtx);
    std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
    if (!conn) return false;

    player.inventory.clear();
    std::string query = "SELECT item_id, slot_index, quantity, is_equipped FROM charakter_db.character_inventory WHERE character_id = " + std::to_string(player.dbId);
    
    if (mysql_query(conn, query.c_str())) {
        Logger::log("[DB] loadInventory Error: " + std::string(mysql_error(conn)));
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) return false;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        ItemInstance item;
        item.itemId = row[0];
        item.slotIndex = std::stoi(row[1]);
        item.quantity = std::stoi(row[2]);
        item.isEquipped = std::stoi(row[3]) != 0;
        player.inventory.push_back(item);
    }
    mysql_free_result(result);
    return true;
}

bool Database::saveInventory(Player& player) {
    std::lock_guard lock(mtx);
    std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
    if (!conn) return false;

    // Direct approach: delete existing and re-insert
    std::string delQuery = "DELETE FROM charakter_db.character_inventory WHERE character_id = " + std::to_string(player.dbId);
    mysql_query(conn, delQuery.c_str());

    for (const auto& item : player.inventory) {
        std::string insQuery = "INSERT INTO charakter_db.character_inventory (character_id, item_id, slot_index, quantity, is_equipped) VALUES (" +
            std::to_string(player.dbId) + ", '" +
            item.itemId + "', " +
            std::to_string(item.slotIndex) + ", " +
            std::to_string(item.quantity) + ", " +
            (item.isEquipped ? "1" : "0") + ")";
        
        if (mysql_query(conn, insQuery.c_str())) {
            Logger::log("[DB] saveInventory Item Error: " + std::string(mysql_error(conn)));
        }
    }
    return true;
}
