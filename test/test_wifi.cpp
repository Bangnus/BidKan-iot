#include <WiFi.h>
#include <PubSubClient.h>

// --- ตั้งค่า WiFi และ MQTT ---
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "YOUR_DROPLET_IP"; // IP ของเซิร์ฟเวอร์ DigitalOcean
const int   mqtt_port = 1883;

// --- ตั้งค่าหัวข้อ MQTT สำหรับ Smart-Mile ---
const char* topic_publish = "bidkan/bikes/BIKE-001/gps";     // ส่งสถานะรถไปบอกเซิร์ฟเวอร์
const char* topic_subscribe = "bidkan/bikes/BIKE-001/command"; // รอรับคำสั่งจากเซิร์ฟเวอร์

// --- ขาอุปกรณ์ ---
const int RELAY_PIN = 12;
const int BATTERY_PIN = 32;

WiFiClient espClient;
PubSubClient mqtt(espClient);

// ฟังก์ชันทำงานเมื่อได้รับคำสั่งจาก Go Fiber
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("📩 ได้รับคำสั่ง: " + message);

  // สมมติเซิร์ฟเวอร์ส่งมาว่า "UNLOCK"
  if (message == "UNLOCK") {
    Serial.println("🔓 ปลดล็อกรถ!");
    digitalWrite(RELAY_PIN, HIGH);
    delay(5000); // จำลองการปลดล็อก 5 วินาที
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("🔒 ล็อกรถกลับสู่สภาพเดิม");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  analogReadResolution(12);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\n✅ WiFi Connected!");

  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(mqttCallback);
}

void reconnect() {
  while (!mqtt.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqtt.connect("BIKE_001_DEV")) {
      Serial.println(" ✅");
      mqtt.subscribe(topic_subscribe); // รอฟังคำสั่ง
    } else {
      delay(5000);
    }
  }
}

void loop() {
  if (!mqtt.connected()) reconnect();
  mqtt.loop();

  // 1. อ่านค่าแบตเตอรี่จริง
  int rawBat = analogRead(BATTERY_PIN);
  int batPercent = map(rawBat, 0, 4095, 0, 100);

  // 2. สร้างพิกัดจำลอง (Mock) + ค่าแบตเตอรี่
  String payload = "{\"lat\":13.756330, \"lon\":100.501820, \"battery\":" + String(batPercent) + "}";
  
  // 3. ส่งข้อมูลขึ้นเซิร์ฟเวอร์
  mqtt.publish(topic_publish, payload.c_str());
  Serial.println("🚀 [DEV] ส่งข้อมูล: " + payload);

  delay(5000); // ส่งทุก 5 วินาที
}