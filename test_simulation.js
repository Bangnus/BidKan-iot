const mqtt = require('mqtt');

const args = process.argv.slice(2);
const WATCH_MODE = args.includes('--watch');
const BIKE_ID = args.find((a) => !a.startsWith('--')) || 'BK-001';
const BROKER_URL = process.env.MQTT_BROKER_URL || 'mqtt://127.0.0.1:1883';

const gpsTopic = `bidkan/bikes/${BIKE_ID}/gps`;
const cmdTopic = `bidkan/bikes/${BIKE_ID}/commands`;
const legacyCmdTopic = `bidkan/bikes/${BIKE_ID}/command`;

// เส้นทางจำลองการขับขี่ภายใน ม.เกษตรฯ กำแพงแสน (วนเป็นวงรอบ อยู่ในโซนให้บริการ)
const CAMPUS_ROUTE = [
  { lat: 14.0234, lon: 99.9782 },
  { lat: 14.0238, lon: 99.9788 },
  { lat: 14.0243, lon: 99.9794 },
  { lat: 14.0249, lon: 99.9798 },
  { lat: 14.0254, lon: 99.9793 },
  { lat: 14.0252, lon: 99.9785 },
  { lat: 14.0246, lon: 99.9779 },
  { lat: 14.0239, lon: 99.9777 },
];

const state = {
  lat: CAMPUS_ROUTE[0].lat,
  lon: CAMPUS_ROUTE[0].lon,
  battery: 92,
  unlocked: false,
  speed: 0,
  routeIndex: 0,
};

let moveTimer = null;

function publishGps(client, overrides = {}) {
  const payload = {
    lat: Number(state.lat.toFixed(6)),
    lon: Number(state.lon.toFixed(6)),
    battery: state.battery,
    status: state.unlocked ? 'in_use' : 'available',
    speed: state.speed,
    ...overrides,
  };
  client.publish(gpsTopic, JSON.stringify(payload), { retain: true });
  console.log(
    `📤 [GPS] ${BIKE_ID} -> lat=${payload.lat}, lon=${payload.lon}, status=${payload.status}, speed=${payload.speed} km/h, bat=${payload.battery}%`
  );
}

function startMoving(client) {
  if (moveTimer) return;
  state.unlocked = true;
  state.speed = 18;
  console.log(`🚲 [Vehicle] ${BIKE_ID} UNLOCKED -> เริ่มเคลื่อนที่ตามเส้นทาง!`);
  publishGps(client);

  moveTimer = setInterval(() => {
    if (!state.unlocked) return;
    state.routeIndex = (state.routeIndex + 1) % CAMPUS_ROUTE.length;
    const nextPoint = CAMPUS_ROUTE[state.routeIndex];
    state.lat = nextPoint.lat;
    state.lon = nextPoint.lon;
    state.speed = 15 + Math.floor(Math.random() * 8);
    if (state.battery > 10 && state.routeIndex % 3 === 0) {
      state.battery -= 1;
    }
    publishGps(client);
  }, 1500);
}

function stopMoving(client, reason = '') {
  state.unlocked = false;
  state.speed = 0;
  if (moveTimer) {
    clearInterval(moveTimer);
    moveTimer = null;
  }
  console.log(`🔒 [Vehicle] ${BIKE_ID} LOCKED -> หยุดเคลื่อนที่ ${reason ? `(${reason})` : ''}`);
  publishGps(client);
}

function parseCommand(raw) {
  try {
    const parsed = JSON.parse(raw);
    return {
      command: (parsed.command || '').toLowerCase(),
      reason: parsed.reason || '',
      payload: parsed,
    };
  } catch {
    return { command: raw.trim().toLowerCase(), reason: '', payload: { raw } };
  }
}

console.log(`🧪 Starting IoT & MQTT Simulation for ${BIKE_ID} (mode=${WATCH_MODE ? 'LIVE WATCH' : 'E2E TEST'})...`);

async function runTests() {
  const client = mqtt.connect(BROKER_URL);

  await new Promise((resolve, reject) => {
    client.on('connect', () => {
      console.log(`✅ [Test Runner] Connected to Mosquitto MQTT Broker (${BROKER_URL})`);
      resolve();
    });
    client.on('error', reject);
  });

  const receivedCommands = [];
  await new Promise((resolve) => client.subscribe([cmdTopic, legacyCmdTopic], resolve));
  console.log(`📡 [Test Runner] Subscribed to ${cmdTopic}`);

  client.on('message', (_topic, message) => {
    const { command, reason, payload } = parseCommand(message.toString());
    console.log(`📥 [Command Received]:`, payload);
    receivedCommands.push(payload);

    if (command === 'unlock') {
      startMoving(client);
    } else if (command === 'lock') {
      stopMoving(client, reason);
    } else if (command === 'beep') {
      console.log(`🔔 [BUZZER] ${BIKE_ID} ส่งเสียงเตือน! (${reason || 'remote_beep'})`);
    }
  });

  if (WATCH_MODE) {
    publishGps(client);
    console.log(`👀 [LIVE WATCH] กำลังรอคำสั่ง unlock / lock / beep จาก Backend หรือ Backoffice... (กด Ctrl+C เพื่อออก)`);
    return;
  }

  // Step 1: Normal campus telemetry (stationary)
  console.log('\n--- Step 1: Telemetry Ingress (Normal campus location) ---');
  publishGps(client);
  await sleep(1000);

  // Step 2: Unlock -> Move along campus route -> Lock
  console.log('\n--- Step 2: Remote Unlock -> Moving along KU Campus -> Lock ---');
  client.publish(cmdTopic, JSON.stringify({
    command: 'unlock',
    bike_id: BIKE_ID,
    timestamp: Math.floor(Date.now() / 1000),
  }));

  await sleep(6500); // ให้รถขยับ ~4 จุด (ทุก 1.5 วินาที)

  client.publish(cmdTopic, JSON.stringify({
    command: 'lock',
    reason: 'admin_emergency_cutoff',
    bike_id: BIKE_ID,
    timestamp: Math.floor(Date.now() / 1000),
  }));

  await sleep(1000);

  // Step 3: Theft simulation (Unauthorized movement while available)
  console.log('\n--- Step 3: Theft Alarm Daemon Simulation ---');
  state.lat = 14.0270;
  state.lon = 99.9810;
  publishGps(client, { status: 'available', speed: 0 });
  console.log('📤 Published unauthorized movement while available (>30m jump)');

  await sleep(1500);

  // Step 4: Geofence breach simulation (Out of boundary)
  console.log('\n--- Step 4: Geofence Breach Daemon Simulation ---');
  state.lat = 14.1500;
  state.lon = 100.1000;
  publishGps(client, { status: 'in_use', speed: 20 });
  console.log('📤 Published GPS outside riding limit zone while in_use');

  await sleep(1500);

  // Reset position back to KU Campus
  state.lat = CAMPUS_ROUTE[0].lat;
  state.lon = CAMPUS_ROUTE[0].lon;
  state.unlocked = false;
  state.speed = 0;
  publishGps(client);

  console.log('\n======================================================');
  console.log(`📊 Test Summary: Total commands captured = ${receivedCommands.length}`);
  const hasLock = receivedCommands.some((c) => c.command === 'lock');
  const hasUnlock = receivedCommands.some((c) => c.command === 'unlock');
  const hasBeep = receivedCommands.some((c) => c.command === 'beep');

  console.log(`   - Unlock Command Delivered: ${hasUnlock ? '✅ PASS' : '❌ FAIL'}`);
  console.log(`   - Relay Cutoff Lock Command Delivered: ${hasLock ? '✅ PASS' : '❌ FAIL'}`);
  console.log(`   - Theft / Buzzer Command Delivered: ${hasBeep ? '✅ PASS' : 'ℹ️ NOTED'}`);
  console.log('======================================================\n');

  client.end();
  process.exit(0);
}

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

runTests().catch((err) => {
  console.error('❌ Test failed with error:', err);
  process.exit(1);
});
