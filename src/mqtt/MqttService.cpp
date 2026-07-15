#include "MqttService.h"

// Static instance pointer for callback routing
MqttService* MqttService::_instance = nullptr;

MqttService::MqttService()
    : _onCommand(nullptr)
    , _lastReconnectAttempt(0)
{
}

void MqttService::begin(TinyGsmClient& gsmClient, CommandCallback onCommand) {
    _instance  = this;
    _onCommand = onCommand;

    _mqtt.setClient(gsmClient);
    _mqtt.setServer(MQTT_SERVER, MQTT_PORT);
    _mqtt.setCallback(_mqttCallbackStatic);

    Serial.println("[MQTT] ตั้งค่าเสร็จ — Server: " + String(MQTT_SERVER));
}

bool MqttService::connect(const String& bikeId) {
    _bikeId  = bikeId;
    _topicPub = String(MQTT_TOPIC_PREFIX) + "/" + bikeId + "/status";
    _topicSub = String(MQTT_TOPIC_PREFIX) + "/" + bikeId + "/command";

    Serial.println("[MQTT] กำลังเชื่อมต่อ broker...");
    Serial.println("[MQTT] Client ID: " + bikeId);
    Serial.println("[MQTT] Pub topic: " + _topicPub);
    Serial.println("[MQTT] Sub topic: " + _topicSub);

    String clientId = "BidKan-" + bikeId;

    bool connected = false;
    // รองรับ MQTT auth ถ้าตั้งค่าไว้
    if (strlen(MQTT_USER) > 0) {
        connected = _mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS);
    } else {
        connected = _mqtt.connect(clientId.c_str());
    }

    if (connected) {
        Serial.println("[MQTT] ✅ เชื่อมต่อสำเร็จ!");
        if (_mqtt.subscribe(_topicSub.c_str())) {
            Serial.println("[MQTT] 📡 Subscribe สำเร็จ: " + _topicSub);
        } else {
            Serial.println("[MQTT] ⚠️ Subscribe ล้มเหลว: " + _topicSub);
        }
        return true;
    }

    Serial.print("[MQTT] ❌ ล้มเหลว, state=");
    Serial.println(_mqtt.state());
    return false;
}

bool MqttService::publishStatus(const JsonDocument& doc, bool retain) {
    size_t requiredSize = measureJson(doc);
    
    // เผื่อขนาด buffer 512 bytes ให้ครอบคลุมข้อมูลส่วนใหญ่
    char buffer[512];
    
    if (requiredSize >= sizeof(buffer)) {
        Serial.println("[MQTT] ⚠️ Payload ใหญ่เกิน buffer! ข้อมูลอาจถูกตัดทิ้ง (Size: " + String(requiredSize) + ")");
        return false;
    }

    size_t len = serializeJson(doc, buffer, sizeof(buffer));

    if (_mqtt.publish(_topicPub.c_str(), (const uint8_t*)buffer, len, retain)) {
        Serial.println("[MQTT] 📤 ส่งข้อมูล: " + String(buffer));
        return true;
    } else {
        Serial.println("[MQTT] ❌ ส่งข้อมูลล้มเหลว");
        return false;
    }
}

void MqttService::loop() {
    _mqtt.loop();
}

bool MqttService::isConnected() {
    return _mqtt.connected();
}

bool MqttService::reconnectIfNeeded() {
    if (_mqtt.connected()) {
        return true;
    }

    // Debounce: ป้องกันการพยายามต่อรัวๆ ทุกรอบ loop()
    unsigned long now = millis();
    if (now - _lastReconnectAttempt < MQTT_RECONNECT_DELAY_MS) {
        return false;
    }
    _lastReconnectAttempt = now;

    Serial.println("[MQTT] 🔄 หลุดการเชื่อมต่อ กำลัง reconnect...");
    return connect(_bikeId);
}

// Static callback — route ไปยัง instance method
void MqttService::_mqttCallbackStatic(char* topic, byte* payload, unsigned int length) {
    if (_instance) {
        _instance->_handleMessage(topic, payload, length);
    }
}

// Instance method — แปลง payload แล้วเรียก user callback
void MqttService::_handleMessage(char* topic, byte* payload, unsigned int length) {
    // กรอง Topic แบบ zero-allocation เพื่อหลีกเลี่ยง heap fragmentation
    if (strcmp(topic, _topicSub.c_str()) != 0) {
        Serial.println("[MQTT] ⚠️ ละทิ้งข้อความจาก Topic ที่ไม่ได้คาดหวัง: " + String(topic));
        return;
    }

    String message;
    message.reserve(length);
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.println("[MQTT] 📩 ได้รับคำสั่ง: " + message);

    if (_onCommand) {
        _onCommand(message);
    }
}
