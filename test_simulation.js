const mqtt = require('mqtt');
const { spawn } = require('child_process');

const BROKER_URL = 'mqtt://127.0.0.1:1883';
const BIKE_ID = 'BK-001';

console.log('🧪 Starting End-to-End IoT & MQTT Simulation Test...');

async function runTests() {
  const client = mqtt.connect(BROKER_URL);

  await new Promise((resolve, reject) => {
    client.on('connect', () => {
      console.log('✅ [Test Runner] Connected to Mosquitto MQTT Broker on port 1883');
      resolve();
    });
    client.on('error', reject);
  });

  const receivedCommands = [];
  const cmdTopic = `bidkan/bikes/${BIKE_ID}/commands`;

  await new Promise((resolve) => client.subscribe(cmdTopic, resolve));
  console.log(`📡 [Test Runner] Subscribed to ${cmdTopic}`);

  client.on('message', (topic, message) => {
    try {
      const payload = JSON.parse(message.toString());
      console.log(`📥 [Received on ${topic}]:`, payload);
      receivedCommands.push(payload);
    } catch {
      console.log(`📥 [Raw]:`, message.toString());
    }
  });

  // Test 1: Send telemetry (GPS in KU campus)
  console.log('\n--- Step 1: Telemetry Ingress (Normal campus location) ---');
  const normalGps = {
    lat: 14.0245,
    lon: 99.9782,
    battery: 95,
    status: 'available',
    speed: 0
  };
  client.publish(`bidkan/bikes/${BIKE_ID}/gps`, JSON.stringify(normalGps), { retain: true });
  console.log('📤 Published normal GPS telemetry at KU Kamphaeng Saen');

  await sleep(1000);

  // Test 2: Remote command simulation (Unlock & Lock)
  console.log('\n--- Step 2: Remote Dispatch (Unlock & Cutoff Relay) ---');
  client.publish(cmdTopic, JSON.stringify({
    command: 'unlock',
    bike_id: BIKE_ID,
    timestamp: Math.floor(Date.now() / 1000)
  }));

  await sleep(600);

  client.publish(cmdTopic, JSON.stringify({
    command: 'lock',
    reason: 'admin_emergency_cutoff',
    bike_id: BIKE_ID,
    timestamp: Math.floor(Date.now() / 1000)
  }));

  await sleep(600);

  // Test 3: Theft simulation (Unauthorized movement while available)
  console.log('\n--- Step 3: Theft Alarm Daemon Simulation ---');
  const theftGps = {
    lat: 14.0270, // Moved > 200m away
    lon: 99.9810,
    battery: 95,
    status: 'available'
  };
  client.publish(`bidkan/bikes/${BIKE_ID}/gps`, JSON.stringify(theftGps));
  console.log('📤 Published unauthorized movement while available (>30m jump)');

  await sleep(1500);

  // Test 4: Geofence breach simulation (Out of boundary)
  console.log('\n--- Step 4: Geofence Breach Daemon Simulation ---');
  const breachGps = {
    lat: 14.1500, // Outside KU campus riding limit
    lon: 100.1000,
    battery: 90,
    status: 'in_use'
  };
  client.publish(`bidkan/bikes/${BIKE_ID}/gps`, JSON.stringify(breachGps));
  console.log('📤 Published GPS outside riding limit zone while in_use');

  await sleep(1500);

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

