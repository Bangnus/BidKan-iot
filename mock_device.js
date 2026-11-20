const mqtt = require('mqtt');
const readline = require('readline');

// รองรับระบุ Bike ID ผ่าน Argument เช่น: node mock_device.js BK-001
const imei = process.argv[2] || 'BK-001';
const brokerUrl = process.env.MQTT_BROKER_URL || 'mqtt://127.0.0.1:1883';

// Topics
const topicPub = `bidkan/bikes/${imei}/gps`;
const topicPubStatus = `bidkan/bikes/${imei}/status`;
const topicSubCmd = `bidkan/bikes/${imei}/command`;
const topicSubCmds = `bidkan/bikes/${imei}/commands`;

// พิกัดเริ่มต้น: มหาวิทยาลัยเกษตรศาสตร์ กำแพงแสน (KU Kamphaeng Saen)
const KU_CAMPUS = { lat: 14.024500, lon: 99.978200 };
const OUTSIDE_ZONE = { lat: 14.080000, lon: 100.050000 }; // นอกเขตให้บริการ

let state = {
    lat: KU_CAMPUS.lat,
    lon: KU_CAMPUS.lon,
    battery: 95,
    status: 'LOCKED',
    speed: 0,
    relay_ignition: false, // ดับเครื่อง
    relay_lock: true       // ล็อกล้อ
};

console.log(`\n=============================================================`);
console.log(`🚲 BidKan IoT ESP32 Mock Simulator — Bike ID: ${imei}`);
console.log(`📡 Connecting to MQTT Broker: ${brokerUrl}`);
console.log(`=============================================================`);

const client = mqtt.connect(brokerUrl, {
    clientId: `Mock-ESP32-${imei}-${Math.random().toString(16).substring(2, 8)}`
});

client.on('connect', () => {
    console.log(`✅ [MQTT] เชื่อมต่อสำเร็จ! Client ID: Mock-ESP32-${imei}`);

    client.subscribe([topicSubCmd, topicSubCmds], (err) => {
        if (!err) {
            console.log(`📡 [MQTT] Subscribe คำสั่งที่:`);
            console.log(`   - ${topicSubCmd}`);
            console.log(`   - ${topicSubCmds}`);
        }
    });

    sendTelemetry();
    setInterval(sendTelemetry, 10000); // ส่งพิกัดทุก 10 วินาที
    setupInteractiveCLI();
});

client.on('message', (topic, message) => {
    const raw = message.toString().trim();
    console.log(`\n📩 [MQTT IN] คำสั่งจากระบบ (${topic}): ${raw}`);

    let cmd = raw.toUpperCase();
    let reason = '';

    try {
        const parsed = JSON.parse(raw);
        if (parsed.command) cmd = parsed.command.toUpperCase();
        else if (parsed.status) cmd = parsed.status === 'LOCKED' ? 'LOCK' : 'UNLOCK';
        if (parsed.reason) reason = parsed.reason;
    } catch {
        // Plaintext command
    }

    if (cmd === 'UNLOCK') {
        executeUnlock();
    } else if (cmd === 'LOCK') {
        executeLock(reason);
    } else if (cmd === 'BEEP' || cmd === 'LOCATE' || cmd === 'FIND_BIKE') {
        console.log(`🔊 [IOT SPEAKER / BUZZER] 📍 บี๊บส่งสัญญาณหาตำแหน่งรถ! (${reason || 'คำสั่งจากแอป/หลังบ้าน'})`);
    } else if (cmd === 'GEOFENCE_ALERT') {
        console.log(`🔊 [IOT SPEAKER / BUZZER] ⚠️ ออกนอกเขตให้บริการ! ลำโพงบนตัวรถส่งเสียงหวูดเตือนดังขึ้น (Geofence Alert)`);
    } else if (cmd === 'NO_PARKING') {
        console.log(`🔊 [IOT SPEAKER / BUZZER] 🚫 จอดนอกโซนที่กำหนด! ลำโพงบนตัวรถส่งเสียงเตือนพื้นที่ห้ามจอด (No-Parking Alert)`);
    } else {
        console.log(`⚠️ [COMMAND] ไม่รู้จักคำสั่ง: ${cmd}`);
    }
});

function executeUnlock() {
    console.log(`[Vehicle] 🔓 Step 1: คลายล็อกล้อ (RELAY_LOCK = LOW)...`);
    state.relay_lock = false;
    setTimeout(() => {
        console.log(`[Vehicle] ⚡ Step 2: เปิดกุญแจจ่ายไฟมอเตอร์ (RELAY_IGNITION = HIGH)...`);
        state.relay_ignition = true;
        state.status = 'UNLOCKED';
        state.speed = 15;
        console.log(`✅ [Vehicle] รถปลดล็อกพร้อมขับขี่! (เสียงตอนปลดรถใช้เสียงเดิมของตัวรถ)`);
        sendTelemetry();
    }, 400);
}

function executeLock(reason) {
    if (reason === 'out_of_boundary') {
        console.log(`🔊 [IOT SPEAKER / BUZZER] ⚠️ ออกนอกเขตให้บริการ! ลำโพงบนตัวรถส่งเสียงหวูดเตือนดังขึ้น (Geofence Alert)`);
    }
    if (reason === 'out_of_boundary') {
        console.log(`🔊 [IOT SPEAKER / BUZZER] ⚠️ ออกนอกเขตให้บริการ! ลำโพงบนตัวรถส่งเสียงหวูดเตือนดังขึ้น (Geofence Alert)`);
    }
    console.log(`[Vehicle] 🛑 Step 1: สั่งตัดไฟมอเตอร์ทันที (RELAY_IGNITION = LOW)...`);
    state.relay_ignition = false;
    state.speed = 0;
    setTimeout(() => {
        console.log(`[Vehicle] 🔒 Step 2: สั่งล็อกล้อไฟฟ้า (RELAY_LOCK = HIGH)...`);
        state.relay_lock = true;
        state.status = 'LOCKED';
        console.log(`🔒 [Vehicle] รถล็อกเรียบร้อย! ${reason ? `[สาเหตุ: ${reason}]` : ''}`);
        sendTelemetry();
    }, 400);
}

function sendTelemetry() {
    if (state.status === 'UNLOCKED' && state.battery > 5) {
        state.battery = Math.max(5, state.battery - 1);
        state.lat += 0.000120;
        state.lon += 0.000090;
    }

    const payload = JSON.stringify({
        lat: parseFloat(state.lat.toFixed(6)),
        lon: parseFloat(state.lon.toFixed(6)),
        battery: state.battery,
        status: state.status,
        speed: state.speed,
        relay_ignition: state.relay_ignition,
        relay_lock: state.relay_lock
    });

    client.publish(topicPub, payload, { retain: true });
    client.publish(topicPubStatus, payload, { retain: true });
    console.log(`📤 [TELEMETRY] ${imei} -> ${payload}`);
}

function setupInteractiveCLI() {
    if (!process.stdin.isTTY) return;

    readline.emitKeypressEvents(process.stdin);
    process.stdin.setRawMode(true);

    printHelpMenu();

    process.stdin.on('keypress', (str, key) => {
        if (key.ctrl && key.name === 'c') {
            console.log('\n👋 จบการจำลอง IoT ESP32');
            process.exit(0);
        }
        const k = key.name ? key.name.toLowerCase() : str;
        if (k === 'u') {
            console.log('\n>> [จำลอง]: ผู้ใช้สแกน QR Code เพื่อปลดล็อกรถ...');
            executeUnlock();
        } else if (k === 'l') {
            console.log('\n>> [จำลอง]: จบการเดินทาง/สั่งล็อกรถ...');
            executeLock('User Ended Ride');
        } else if (k === 'b') {
            console.log('\n>> [จำลอง]: รถวิ่งทะลุออกนอกพื้นที่ Riding Limit Zone (Geofence Breach)!');
            state.lat = OUTSIDE_ZONE.lat;
            state.lon = OUTSIDE_ZONE.lon;
            sendTelemetry();
            console.log(`⚠️ รอรับคำสั่งสั่งตัดไฟ (Relay Cutoff) อัตโนมัติจาก Go Backend...`);
        } else if (k === 't') {
            console.log('\n>> [จำลอง]: รถถูกเคลื่อนย้ายขณะสถานะว่าง (Theft Simulation)!');
            state.lat += 0.001000;
            state.lon += 0.001000;
            sendTelemetry();
            console.log(`🚨 รอรับคำสั่งส่งเสียงร้องเตือนกันขโมย (Theft Buzzer) จาก Backend...`);
        } else if (k === 'd') {
            console.log('\n>> [จำลอง]: จำลองแบตเตอรี่วิกฤต (<15%)');
            state.battery = 12;
            sendTelemetry();
        } else if (k === 'h') {
            printHelpMenu();
        }
    });
}

function printHelpMenu() {
    console.log(`\n⌨️  [Interactive Simulator Controls]:`);
    console.log(`   [u] ปลดล็อกรถ (Unlock & Power ON)`);
    console.log(`   [l] ล็อกรถ (Lock & Power OFF)`);
    console.log(`   [b] จำลองขับออกนอกเขตโซน (Geofence Breach -> ทดสอบ Relay Cutoff)`);
    console.log(`   [t] จำลองการเคลื่อนย้ายรถผิดปกติ (Theft Movement -> ทดสอบ Theft Alarm)`);
    console.log(`   [d] จำลองแบตเตอรี่ต่ำวิกฤต 12% (Low Battery Guard)`);
    console.log(`   [h] แสดงเมนูช่วยเหลือ | [Ctrl+C] ออกจากการจำลอง\n`);
}
