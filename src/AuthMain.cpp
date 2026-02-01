#include "Database.hpp"
#include "Logger.hpp"
#include "App.h"
#include "EnvLoader.hpp"
#include <iostream>

// --- SAFE EMBEDDED ASSETS ---
const std::string INDEX_HTML = R"=====(<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>THE GRID | System Interface</title>
    <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;700&display=swap" rel="stylesheet">
    <link rel="stylesheet" href="style.css">
</head>
<body>
    <div class="container">
        <nav>
            <h1 onclick="location.href='/'" style="cursor:pointer">THE GRID</h1>
            <div class="nav-links">
                <span id="user-info" class="hidden" style="margin-right: 15px; color: var(--neon-blue);"></span>
                <a href="/account" class="nav-btn" id="nav-acc-btn">Login / Join</a>
                <button id="nav-logout-btn" class="nav-btn hidden" onclick="logout()">Logout</button>
            </div>
        </nav>
        <div id="dashboard-page">
            <section style="margin-bottom: 50px;">
                <h2 style="margin-bottom: 25px; letter-spacing: 2px;">LIVE SIMULATION STATUS</h2>
                <table>
                    <thead>
                        <tr><th>Ident</th><th>Charakter</th><th>Map</th><th>Status</th></tr>
                    </thead>
                    <tbody id="live-player-table"></tbody>
                </table>
            </section>
            <section id="user-chars-section" class="hidden">
                <h2 style="margin-bottom: 25px; letter-spacing: 2px;">DEINE CHARAKTERE</h2>
                <div id="char-list" class="char-grid"></div>
            </section>
            <div id="auth-notice" class="hidden" style="text-align: center; padding: 40px; border: 1px dashed var(--glass-border); border-radius: 15px; background: rgba(255,255,255,0.02); margin-top: 20px;">
                <p style="opacity: 0.6;">Logge dich ein, um deine Charaktere zu verwalten.</p>
                <a href="/account" class="nav-btn" style="display: inline-block; margin-top: 15px;">Jetzt Einloggen</a>
            </div>
        </div>
    </div>
    <script>
        async function init() {
            const token = localStorage.getItem('token');
            const username = localStorage.getItem('username');
            fetchLiveStats();
            setInterval(fetchLiveStats, 3000);
            if (token) {
                document.getElementById('nav-logout-btn').classList.remove('hidden');
                document.getElementById('nav-acc-btn').classList.add('hidden');
                document.getElementById('user-info').innerText = username;
                document.getElementById('user-info').classList.remove('hidden');
                document.getElementById('user-chars-section').classList.remove('hidden');
                loadCharacters();
            } else {
                document.getElementById('auth-notice').classList.remove('hidden');
            }
        }
        async function fetchLiveStats() {
            try {
                const res = await fetch('/api/admin/stats');
                const data = await res.json();
                const tableBody = document.getElementById('live-player-table');
                tableBody.innerHTML = '';
                data.players.forEach(p => {
                    const row = document.createElement('tr');
                    const status = p.idle ? '<span class="badge badge-idle">Idle</span>' : '<span style="color: #00FF00">Aktiv</span>';
                    row.innerHTML = '<td>'+p.username+'</td><td style="color: var(--neon-blue)">'+p.charName+'</td><td><span class="badge badge-map">'+p.mapName+'</span></td><td>'+status+'</td>';
                    tableBody.appendChild(row);
                });
                if (data.players.length === 0) tableBody.innerHTML = '<tr><td colspan="4" style="text-align: center; opacity: 0.5;">Momentan keine User aktiv.</td></tr>';
            } catch (e) { console.error(e); }
        }
        async function loadCharacters() {
            const token = localStorage.getItem('token');
            try {
                const res = await fetch('/api/characters', { headers: { 'Authorization': 'Bearer ' + token } });
                if (res.status === 403) return logout();
                const chars = await res.json();
                const charList = document.getElementById('char-list');
                charList.innerHTML = '';
                if (chars.length === 0) { charList.innerHTML = '<div style="grid-column: 1/-1; text-align: center; opacity: 0.5; padding: 40px;">Keine Charaktere gefunden.</div>'; return; }
                chars.forEach(char => {
                    const xpGoal = 100 * Math.pow(char.level, 1.5);
                    const xpPercent = Math.min(100, (char.xp / xpGoal) * 100);
                    const card = document.createElement('div');
                    card.className = 'char-card';
                    card.innerHTML = '<div style="font-size: 1.5em; font-weight: bold; color: '+char.class_info.class_color+'">'+char.char_name+'</div>' +
                                   '<div style="opacity: 0.7; font-size: 0.9em;">'+char.class_info.class_name+' | '+char.class_info.role+'</div>' +
                                   '<div style="color: var(--neon-blue); font-size: 0.8em; margin-top: 10px;">LEVEL '+char.level+'</div>' +
                                   '<div class="xp-bar"><div class="xp-fill" style="width: '+xpPercent+'%"></div></div>' +
                                   '<div style="font-size: 0.75em; opacity: 0.5; margin-top: 5px;">Simulation: '+char.world_state.map_name+'<br>Status: Online</div>';
                    charList.appendChild(card);
                });
            } catch (err) { console.error(err); }
        }
        function logout() { localStorage.clear(); location.href = '/account'; }
        init();
    </script>
</body>
</html>)=====";

const std::string ACCOUNT_HTML = R"=====(<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>THE GRID | Account Portal</title>
    <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;700&display=swap" rel="stylesheet">
    <link rel="stylesheet" href="style.css">
</head>
<body>
    <div class="container">
        <nav>
            <h1 onclick="location.href='/'" style="cursor:pointer">THE GRID</h1>
            <div class="nav-links"><a href="/" class="nav-btn">Zurück zum Dash</a></div>
        </nav>
        <div style="display: flex; justify-content: center; align-items: center; min-height: 60vh;">
            <div class="auth-card">
                <h2 id="auth-title" style="text-align: center; margin-bottom: 30px;">Simulation Login</h2>
                <form id="auth-form">
                    <div class="input-group"><label>User Ident</label><input type="text" id="username" required></div>
                    <div class="input-group"><label>Security Code</label><input type="password" id="password" required></div>
                    <button type="submit" id="submit-btn" class="glow-button">Initialisieren</button>
                    <div id="auth-error" style="color: var(--neon-red); margin-top: 15px; text-align: center; font-size: 0.8em;"></div>
                </form>
                <div style="text-align: center; margin-top: 20px; font-size: 0.85em;">
                    <span id="toggle-text">Neu im System?</span>
                    <span style="color: var(--neon-blue); cursor: pointer;" onclick="toggleAuth()">Registrieren</span>
                </div>
            </div>
        </div>
    </div>
    <script>
        let isLogin = true;
        function toggleAuth() {
            isLogin = !isLogin;
            document.getElementById('auth-title').innerText = isLogin ? 'Simulation Login' : 'System Registrierung';
            document.getElementById('submit-btn').innerText = isLogin ? 'Initialisieren' : 'Zugang Erstellen';
            document.getElementById('toggle-text').innerText = isLogin ? 'Neu im System?' : 'Bereits registriert?';
        }
        document.getElementById('auth-form').addEventListener('submit', async (e) => {
            e.preventDefault();
            const username = document.getElementById('username').value;
            const password = document.getElementById('password').value;
            const statusDiv = document.getElementById('auth-error');
            statusDiv.innerText = '';
            try {
                const response = await fetch(isLogin ? '/api/login' : '/api/register', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ username, password })
                });
                const data = await response.json();
                if (response.ok) {
                    if (isLogin) {
                        localStorage.setItem('token', data.token);
                        localStorage.setItem('username', username);
                        location.href = '/';
                    } else { alert('Registrierung erfolgreich!'); toggleAuth(); }
                } else { statusDiv.innerText = data.error; }
            } catch (err) { statusDiv.innerText = 'Server Error'; }
        });
    </script>
</body>
</html>)=====";

const std::string STYLE_CSS = R"=====(:root {
    --neon-blue: #00FFFF;
    --neon-purple: #9D00FF;
    --neon-red: #FF0055;
    --bg-dark: #050505;
    --panel-bg: rgba(20, 20, 20, 0.85);
    --glass-border: rgba(255, 255, 255, 0.1);
}
* { box-sizing: border-box; transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1); }
body {
    background-color: var(--bg-dark);
    color: white;
    font-family: 'Outfit', sans-serif;
    margin: 0;
    min-height: 100vh;
    background-image: linear-gradient(rgba(0, 255, 255, 0.05) 1px, transparent 1px), linear-gradient(90deg, rgba(0, 255, 255, 0.05) 1px, transparent 1px);
    background-size: 60px 60px;
    overflow-x: hidden;
}
body::before {
    content: " "; display: block; position: fixed; top: 0; left: 0; bottom: 0; right: 0;
    background: linear-gradient(rgba(18, 16, 16, 0) 50%, rgba(0, 0, 0, 0.1) 50%), linear-gradient(90deg, rgba(255, 0, 0, 0.03), rgba(0, 255, 0, 0.01), rgba(0, 0, 255, 0.03));
    z-index: 100; background-size: 100% 4px, 3px 100%; pointer-events: none; opacity: 0.3;
}
.container { max-width: 1200px; margin: 0 auto; padding: 20px; }
nav { display: flex; justify-content: space-between; align-items: center; padding: 20px 0; border-bottom: 2px solid var(--neon-blue); margin-bottom: 40px; box-shadow: 0 10px 20px -10px var(--neon-blue); }
nav h1 { margin: 0; letter-spacing: 5px; font-size: 1.5em; text-shadow: 0 0 10px var(--neon-blue); }
.nav-links { display: flex; gap: 20px; }
.nav-btn { background: transparent; border: 1px solid var(--neon-blue); color: var(--neon-blue); padding: 8px 20px; border-radius: 5px; text-transform: uppercase; font-weight: bold; cursor: pointer; text-decoration: none; font-size: 0.9em; }
.nav-btn:hover { background: var(--neon-blue); color: var(--bg-dark); box-shadow: 0 0 15px var(--neon-blue); }
.auth-card { background: var(--panel-bg); border: 1px solid var(--glass-border); padding: 40px; border-radius: 20px; width: 100%; max-width: 450px; position: relative; box-shadow: 0 0 50px rgba(0, 255, 255, 0.1); }
table { width: 100%; border-collapse: collapse; background: var(--panel-bg); border-radius: 15px; overflow: hidden; backdrop-filter: blur(10px); border: 1px solid var(--glass-border); }
th, td { padding: 15px; text-align: left; border-bottom: 1px solid rgba(255, 255, 255, 0.05); }
th { background: rgba(0, 255, 255, 0.1); color: var(--neon-blue); text-transform: uppercase; font-size: 0.8em; letter-spacing: 2px; }
.badge { padding: 3px 8px; border-radius: 4px; font-size: 0.8em; }
.badge-map { background: rgba(157, 0, 255, 0.2); border: 1px solid var(--neon-purple); color: var(--neon-purple); }
.badge-idle { background: rgba(255, 255, 255, 0.1); color: #aaa; }
.char-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(300px, 1fr)); gap: 30px; margin-top: 30px; }
.char-card { background: var(--panel-bg); border: 1px solid var(--glass-border); border-radius: 15px; padding: 25px; position: relative; overflow: hidden; }
.char-card:hover { transform: translateY(-5px); border-color: var(--neon-blue); box-shadow: 0 10px 30px rgba(0, 255, 255, 0.2); }
.char-card::before { content: ''; position: absolute; top: 0; left: -100%; width: 50%; height: 100%; background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.05), transparent); transition: 0.5s; }
.char-card:hover::before { left: 100%; }
.xp-bar { height: 4px; background: rgba(255, 255, 255, 0.1); border-radius: 2px; margin: 15px 0; overflow: hidden; }
.xp-fill { height: 100%; background: var(--neon-blue); box-shadow: 0 0 10px var(--neon-blue); }
.input-group { margin-bottom: 20px; }
label { display: block; color: var(--neon-blue); font-size: 0.8em; margin-bottom: 8px; text-transform: uppercase; }
input { width: 100%; background: rgba(255, 255, 255, 0.05); border: 1px solid var(--glass-border); padding: 12px; color: white; border-radius: 8px; }
input:focus { outline: none; border-color: var(--neon-blue); box-shadow: 0 0 10px var(--neon-blue); }
.glow-button { width: 100%; padding: 15px; background: var(--neon-blue); border: none; color: var(--bg-dark); font-weight: bold; text-transform: uppercase; cursor: pointer; border-radius: 8px; letter-spacing: 1px; }
.hidden { display: none !important; }
)=====";

int main() {
    Logger::init("auth_server.log");
    EnvLoader::load(".env");

    Logger::log("=========================================");
    Logger::log("Starting The Grid AUTH SERVER (Embedded)...");
    Logger::log("=========================================");

    Database::Config config;
    config.host = std::string(getenv("DB_HOST") ? getenv("DB_HOST") : "localhost");
    config.user = std::string(getenv("DB_USER") ? getenv("DB_USER") : "user_name");
    config.pass = std::string(getenv("DB_PASS") ? getenv("DB_PASS") : "user_passwort");
    
    const char* portStr = getenv("DB_PORT");
    if (portStr) {
        config.port = std::stoi(portStr);
    } else {
        // Fallback to MYSQL_PORT if DB_PORT is not set
        const char* mysqlPortStr = getenv("MYSQL_PORT");
        if (mysqlPortStr) config.port = std::stoi(mysqlPortStr);
    }

    if (!Database::getInstance().connect(config)) {
        Logger::log("[FATAL] Could not connect to MySQL.");
        return 1;
    }

    uWS::App app;

    app.get("/", [](auto *res, auto *req) {
        res->writeHeader("Content-Type", "text/html; charset=utf-8")->end(INDEX_HTML);
    });

    app.get("/account", [](auto *res, auto *req) {
        res->writeHeader("Content-Type", "text/html; charset=utf-8")->end(ACCOUNT_HTML);
    });

    app.get("/style.css", [](auto *res, auto *req) {
        res->writeHeader("Content-Type", "text/css; charset=utf-8")->end(STYLE_CSS);
    });

    app.post("/api/login", [](auto *res, auto *req) {
        res->onAborted([]() {});
        res->onData([res](std::string_view data, bool isLast) {
            static std::string buffer;
            buffer.append(data.data(), data.length());
            if (isLast) {
                try {
                    auto j = json::parse(buffer);
                    buffer.clear();
                    std::string user = j.value("username", "");
                    std::string pass = j.value("password", "");
                    json result = Database::getInstance().authenticate(user, pass);
                    if (!result.is_null()) {
                        res->writeHeader("Content-Type", "application/json")->end(result.dump());
                    } else {
                        res->writeStatus("401 Unauthorized")->end("{\"error\":\"Invalid credentials\"}");
                    }
                } catch (...) { res->writeStatus("400")->end(); }
            }
        });
    });

    app.post("/api/register", [](auto *res, auto *req) {
        res->onAborted([]() {});
        res->onData([res](std::string_view data, bool isLast) {
            static std::string buffer;
            buffer.append(data.data(), data.length());
            if (isLast) {
                try {
                    auto j = json::parse(buffer);
                    buffer.clear();
                    std::string user = j.value("username", "");
                    std::string pass = j.value("password", "");
                    if (Database::getInstance().registerUser(user, pass)) {
                        res->writeHeader("Content-Type", "application/json")->end("{\"success\":true}");
                    } else {
                        res->writeStatus("400 Bad Request")->end("{\"error\":\"Registration failed\"}");
                    }
                } catch (...) { res->writeStatus("400")->end(); }
            }
        });
    });

    app.get("/api/characters", [](auto *res, auto *req) {
        std::string auth = std::string(req->getHeader("authorization"));
        std::string user = "";
        if (auth.starts_with("Bearer ")) {
            std::string token = auth.substr(7);
            if (token.starts_with("session_")) user = token.substr(8);
        }
        if (user.empty()) {
            res->writeStatus("401 Unauthorized")->end("{\"error\":\"Invalid or missing token\"}");
            return;
        }
        json chars = Database::getInstance().getCharactersForUser(user);
        res->writeHeader("Content-Type", "application/json")->end(chars.dump());
    });

    app.post("/api/characters", [](auto *res, auto *req) {
        std::string auth = std::string(req->getHeader("authorization"));
        std::string user = "";
        if (auth.starts_with("Bearer ")) {
            std::string token = auth.substr(7);
            if (token.starts_with("session_")) user = token.substr(8);
        }
        if (user.empty()) {
            res->writeStatus("401 Unauthorized")->end("{\"error\":\"Invalid or missing token\"}");
            return;
        }

        res->onAborted([]() {});
        res->onData([res, user](std::string_view data, bool isLast) {
            static std::string buffer;
            buffer.append(data.data(), data.length());
            if (isLast) {
                try {
                    auto j = json::parse(buffer);
                    buffer.clear();
                    std::string charName = j.value("name", "");
                    std::string charClass = j.value("class", "Mage");
                    if (charName.empty()) {
                        res->writeStatus("400 Bad Request")->end("{\"error\":\"Name is required\"}");
                        return;
                    }
                    if (Database::getInstance().createCharacter(user, charName, charClass)) {
                        res->writeHeader("Content-Type", "application/json")->end("{\"success\":true}");
                    } else {
                        res->writeStatus("400 Bad Request")->end("{\"error\":\"Character creation failed\"}");
                    }
                } catch (...) { res->writeStatus("400")->end(); }
            }
        });
    });

    app.post("/api/characters/delete", [](auto *res, auto *req) {
        std::string auth = std::string(req->getHeader("authorization"));
        std::string user = "";
        if (auth.starts_with("Bearer ")) {
            std::string token = auth.substr(7);
            if (token.starts_with("session_")) user = token.substr(8);
        }
        if (user.empty()) {
            res->writeStatus("401 Unauthorized")->end("{\"error\":\"Invalid or missing token\"}");
            return;
        }

        res->onAborted([]() {});
        res->onData([res, user](std::string_view data, bool isLast) {
            static std::string buffer;
            buffer.append(data.data(), data.length());
            if (isLast) {
                try {
                    auto j = json::parse(buffer);
                    buffer.clear();
                    int charId = j.value("id", -1);
                    if (charId == -1) {
                        res->writeStatus("400 Bad Request")->end("{\"error\":\"ID is required\"}");
                        return;
                    }
                    if (Database::getInstance().deleteCharacter(user, charId)) {
                        res->writeHeader("Content-Type", "application/json")->end("{\"success\":true}");
                    } else {
                        res->writeStatus("400 Bad Request")->end("{\"error\":\"Character deletion failed or unauthorized\"}");
                    }
                } catch (...) { res->writeStatus("400")->end(); }
            }
        });
    });

    app.get("/api/admin/stats", [](auto *res, auto *req) {
        res->writeHeader("Content-Type", "application/json")->end("{\"players\": []}");
    });

    app.listen(3000, [](auto *listen_socket) {
        if (listen_socket) std::cout << "Auth Server listening on port 3000 (Embedded Assets)" << std::endl;
    });

    app.run();
    return 0;
}
