const WebSocket = require('ws');

const SERVER_URL = 'ws://127.0.0.1:3001/ws';
const NUM_CLIENTS = 100;
const UPDATE_INTERVAL = 100;

// --- KOLLISIONS-VORGABEN ---
const MAP_LIMIT = 48; // Rand der Map (bei 50 sind die Wände)
const OBSTACLES = [
    { minX: -15, maxX: 15, minZ: -15, maxZ: 15 } // Beispiel: Zentrales Plateau / Hindernis
];

/**
 * Prüft, ob eine Position gültig ist
 */
function isPositionValid(x, z) {
    // 1. Map-Grenzen prüfen
    if (Math.abs(x) > MAP_LIMIT || Math.abs(z) > MAP_LIMIT) return false;

    // 2. Hindernisse prüfen
    for (const ob of OBSTACLES) {
        if (x > ob.minX && x < ob.maxX && z > ob.minZ && z < ob.maxZ) {
            return false;
        }
    }
    return true;
}

async function spawnClient(id) {
    const ws = new WebSocket(SERVER_URL);
    const username = `BotPlayer_${id}`;

    let pos = { x: (Math.random() * 20 - 10), y: 0.0, z: (Math.random() * 20 - 10) };
    let currentRotY = Math.random() * Math.PI * 2;
    let targetRotY = currentRotY;
    let state = 'IDLE';
    let stateTimer = 0;

    ws.on('open', () => {
        ws.send(JSON.stringify({
            type: 'authenticate',
            token: `session_${username}`,
            character_id: 1000 + id
        }));
    });

    ws.on('message', (data) => {
        const msg = JSON.parse(data);
        if (msg.type === 'authenticated') {
            setInterval(() => {
                if (ws.readyState !== WebSocket.OPEN) return;

                stateTimer--;
                if (stateTimer <= 0) {
                    if (state === 'IDLE') {
                        state = 'WALKING';
                        stateTimer = 30 + Math.random() * 100;
                        targetRotY = Math.random() * Math.PI * 2;
                    } else {
                        state = 'IDLE';
                        stateTimer = 20 + Math.random() * 50;
                    }
                }

                // Drehung
                let rotDiff = targetRotY - currentRotY;
                while (rotDiff > Math.PI) rotDiff -= Math.PI * 2;
                while (rotDiff < -Math.PI) rotDiff += Math.PI * 2;
                currentRotY += rotDiff * 0.1;

                // BEWEGUNG MIT KOLLISIONSABFRAGE
                if (state === 'WALKING') {
                    const moveSpeed = 0.15;
                    const nextX = pos.x + Math.sin(currentRotY) * moveSpeed;
                    const nextZ = pos.z + Math.cos(currentRotY) * moveSpeed;

                    if (isPositionValid(nextX, nextZ)) {
                        pos.x = nextX;
                        pos.z = nextZ;
                    } else {
                        // Kollision! 
                        // Verändere Ziel-Richtung (drehe den Bot weg)
                        targetRotY += Math.PI * 0.8; // Fast umdrehen
                        stateTimer = 5; // Kurz warten/drehen
                    }
                }

                ws.send(JSON.stringify({
                    type: 'player_update',
                    position: pos,
                    rotation: { x: 0, y: currentRotY, z: 0 }
                }));

            }, UPDATE_INTERVAL);
        }
    });

    ws.on('error', () => { });
    ws.on('close', () => { });
}

for (let i = 0; i < NUM_CLIENTS; i++) {
    spawnClient(i);
}