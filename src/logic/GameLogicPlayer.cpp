#include "../GameLogic.hpp"
#include "SocketHandlers.hpp"
#include "Database.hpp"
#include "Logger.hpp"

void GameLogic::awardXP(Player& player, int amount) {
    {
        std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
        player.xp += amount;
        Logger::log("[XP] " + player.charName + " erhielt " + std::to_string(amount) + " Erfahrung.");
        
        json ctMsg = {
            {"type", "combat_text"}, {"target_id", "player"},
            {"value", "+" + std::to_string(amount) + " XP"},
            {"is_crit", false}, {"color", "#32CD32"}
        };
        SocketHandlers::sendToPlayer(player.username, ctMsg.dump());
    }

    auto levelData = getLevelData(player.level);
    bool leveledUp = false;
    {
        std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
        while (player.xp >= levelData.xpToNextLevel && player.level < 100) {
            player.xp -= levelData.xpToNextLevel;
            player.level++;
            levelData = getLevelData(player.level);
            leveledUp = true;
        }
    }

    if (leveledUp) {
        std::string pName, pMap, pUsername;
        {
            std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
            player.maxHp = levelData.hp;
            player.hp = player.maxHp;
            player.maxMana = levelData.mana;
            player.mana = player.maxMana;
            pName = player.charName;
            pMap = player.mapName;
            pUsername = player.username;
            Logger::log("[LEVEL] " + pName + " ist jetzt Level " + std::to_string(player.level) + "!");
        }
        
        json chatMsg = {{"type", "chat_receive"}, {"mode", "system"}, {"message", "Glückwunsch! Du hast Level " + std::to_string(player.level) + " erreicht!"}};
        if (!player.isDisconnected) SocketHandlers::sendToPlayer(pUsername, chatMsg.dump());

        json levelUpMsg = {{"type", "level_up"}, {"username", pName}};
        SocketHandlers::broadcastToMap(pMap, levelUpMsg.dump());
    }

    SocketHandlers::syncPlayerStatus(GameState::getInstance().getPlayer(player.username));

    Database::getInstance().savePlayer(player);
}

int GameLogic::getInventorySize(int level) {
    if (level <= 10) return 30;
    if (level <= 20) return 40;
    if (level <= 30) return 60;
    if (level <= 40) return 80;
    return 100;
}

void GameLogic::checkQuestKill(Player& player, const std::string& mobTypeId) {
    std::lock_guard<std::recursive_mutex> pLock(player.pMtx);
    bool changed = false;

    for (auto& pq : player.quests) {
        if (pq.status != "active") continue;

        QuestTemplate qt = GameState::getInstance().getQuestTemplate(pq.questId);
        if (qt.objectives.count(mobTypeId)) {
            int required = qt.objectives.at(mobTypeId);
            if (pq.progress[mobTypeId] < required) {
                pq.progress[mobTypeId]++;
                changed = true;
            }

            // Check if all objectives reached
            bool allMet = true;
            for (auto const& [objId, requiredAmount] : qt.objectives) {
                if (pq.progress[objId] < requiredAmount) {
                    allMet = false;
                    break;
                }
            }

            if (allMet) {
                pq.status = "completed";
                json msg = {{"type", "chat_receive"}, {"mode", "system"}, {"message", "Quest abgeschlossen: " + qt.title + "! Kehre zum Questgeber zurück."}};
                SocketHandlers::sendToPlayer(player.username, msg.dump());
                
                json completeMsg = {{"type", "quest_completed"}, {"quest_id", pq.questId}};
                SocketHandlers::sendToPlayer(player.username, completeMsg.dump());
            } else {
                // Update Progress message (optional)
                std::string msgText = qt.title + ": " + std::to_string(pq.progress[mobTypeId]) + "/" + std::to_string(qt.objectives.at(mobTypeId)) + " getötet.";
                json pMsg = {{"type", "chat_receive"}, {"mode", "system"}, {"message", msgText}};
                SocketHandlers::sendToPlayer(player.username, pMsg.dump());
                
                json progUpdate = {{"type", "quest_progress"}, {"quest_id", pq.questId}, {"progress", pq.progress}};
                SocketHandlers::sendToPlayer(player.username, progUpdate.dump());
            }
        }
    }

    if (changed) {
        Database::getInstance().saveQuests(player);
    }
}
