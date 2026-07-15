const mqtt = require('mqtt');

// การตั้งค่าให้เชื่อมต่อกับ Local Mosquitto ใน Docker
const brokerUrl = 'mqtt://127.0.0.1:1883';
const imei = 'BIKE-001'; // ปรับให้ตรงกับรูปใน MQTTX
const topicPub = `bidkan/bikes/${imei}/gps`;
const topicSub = `bidkan/bikes/${imei}/command`;

// สถานะจำลอง
let state = {
    lat: 13.756300,
    lon: 100.501800,
    battery: 100,
    status: 'LOCKED'
};

console.log(`🚀 กำลังเชื่อมต่อ MQTT Broker: ${brokerUrl}`);
const client = mqtt.connect(brokerUrl, {
    clientId: `Mock-BidKan-${imei}-${Math.random().toString(16).substring(2, 8)}`
});

client.on('connect', () => {
    console.log('✅ เชื่อมต่อ MQTT สำเร็จ!');
    
    // Subscribe รอรับคำสั่งจากแอป
    client.subscribe(topicSub, (err) => {
        if (!err) {
            console.log(`📡 Subscribe รอรับคำสั่งที่: ${topicSub}`);
        }
    });

    // เริ่มลูปส่งข้อมูลทุก 15 วินาที
    sendTelemetry();
    setInterval(sendTelemetry, 15000);
});

client.on('message', (topic, message) => {
    let cmd = message.toString().trim();
    console.log(`\n📩 ได้รับคำสั่งจากแอป: ${cmd}`);

    // ลองแปลงเป็น JSON ถ้าคำสั่งส่งมาเป็น JSON object
    try {
        const parsed = JSON.parse(cmd);
        if (parsed.status) {
            if (parsed.status === 'LOCKED') cmd = 'LOCK';
            else if (parsed.status === 'UNLOCKED') cmd = 'UNLOCK';
        }
    } catch (e) {
        // ถ้าไม่ใช่ JSON ก็ปล่อยให้เป็น Plaintext ดั้งเดิม
    }

    if (cmd === 'UNLOCK') {
        state.status = 'UNLOCKED';
        console.log('🔓 ปลดล็อกรถเรียบร้อย');
        sendTelemetry(); // อัปเดตแอปทันที
    } else if (cmd === 'LOCK') {
        state.status = 'LOCKED';
        console.log('🔒 ล็อกรถเรียบร้อย');
        sendTelemetry(); // อัปเดตแอปทันที
    } else {
        console.log(`⚠️ ไม่รู้จักคำสั่ง: ${cmd}`);
    }
});

function sendTelemetry() {
    // จำลองแบตลดลงเรื่อยๆ
    if (state.battery > 15) {
        state.battery -= 1;
    }

    // จำลองรถขยับนิดหน่อยถ้าปลดล็อกแล้ว
    if (state.status === 'UNLOCKED') {
        state.lat += 0.000100;
        state.lon += 0.000100;
    }

    const payload = JSON.stringify(state);
    client.publish(topicPub, payload, { retain: true }, () => {
        console.log(`📤 ส่งข้อมูล: ${payload}`);
    });
}
