const WebSocket = require('ws');

const SERVER_URL = 'ws://127.0.0.1:3001/ws';
const NUM_CLIENTS = 100;
const UPDATE_INTERVAL = 100; // 10 FPS für flüssigere Bewegung

async function spawnClient(id) {
    return new Promise((resolve) => {
        const ws = new WebSocket(SERVER_URL);
        const username = `BotPlayer_${id}`;

        // Zustands-Variablen
        let pos = { x: Math.random() * 100 - 50, y: 0.0, z: Math.random() * 100 - 50 };
        let currentRotY = Math.random() * Math.PI * 2;
        let targetRotY = currentRotY;

        let state = 'IDLE'; // IDLE, WALKING
        let stateTimer = 0;

        let isJumping = false;
        let jumpVelocity = 0;
        const gravity = -0.015;
        const jumpPower = 0.25;

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

                    // 1. Logik-Entscheidungen (alle paar Ticks)
                    stateTimer--;
                    if (stateTimer <= 0) {
                        if (state === 'IDLE') {
                            state = 'WALKING';
                            stateTimer = 30 + Math.random() * 100; // 3-10 Sekunden laufen
                            targetRotY = Math.random() * Math.PI * 2;
                        } else {
                            state = 'IDLE';
                            stateTimer = 20 + Math.random() * 50; // 2-5 Sekunden stehen
                        }
                    }

                    // 2. Drehung sanft anpassen (Lerp-like)
                    let rotDiff = targetRotY - currentRotY;
                    while (rotDiff > Math.PI) rotDiff -= Math.PI * 2;
                    while (rotDiff < -Math.PI) rotDiff += Math.PI * 2;
                    currentRotY += rotDiff * 0.1;

                    // 3. Bewegung
                    if (state === 'WALKING') {
                        const moveSpeed = 0.08; // Natürliche Gehgeschwindigkeit
                        pos.x += Math.sin(currentRotY) * moveSpeed;
                        pos.z += Math.cos(currentRotY) * moveSpeed;
                    }

                    // 4. Springen (gelegentlich)
                    if (!isJumping && state === 'WALKING' && Math.random() < 0.01) {
                        isJumping = true;
                        jumpVelocity = jumpPower;
                    }

                    if (isJumping) {
                        pos.y += jumpVelocity;
                        jumpVelocity += gravity;
                        if (pos.y <= 0) {
                            pos.y = 0;
                            isJumping = false;
                            jumpVelocity = 0;
                        }
                    }

                    // 5. Update senden
                    ws.send(JSON.stringify({
                        type: 'player_update',
                        position: pos,
                        rotation: { x: 0, y: currentRotY, z: 0 }
                    }));

                    // 6. Gelegentlicher Chat
                    if (Math.random() < 0.002) {
                        const messages = ["Hi!", "Nice world", "Lvl up?", "Looking for group", "Hello", "test"];
                        ws.send(JSON.stringify({
                            type: 'chat_message',
                            message: messages[Math.floor(Math.random() * messages.length)],
                            target_id: ""
                        }));
                    }

                }, UPDATE_INTERVAL);
            }
        });

        ws.on('error', () => { });
        ws.on('close', () => { });
    });
}

console.log(`====================================================`);
console.log(`REALISTIC MIGRATION: 100 Bots auf dem Weg...`);
console.log(`Simuliere echte Spielerbewegung (Walking/Idle/Jump)`);
console.log(`====================================================`);

for (let i = 0; i < NUM_CLIENTS; i++) {
    spawnClient(i);
}
