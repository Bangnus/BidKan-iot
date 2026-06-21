#define TINY_GSM_MODEM_A7670
#define TINY_GSM_RX_BUFFER 1024

#include <TinyGsmClient.h>
#include <PubSubClient.h>

// --- ขาเชื่อมต่อของ LilyGO T-A7670SA และอุปกรณ์ ---
#define MODEM_RX 26
#define MODEM_TX 27
#define MODEM_PWRKEY 4
const int RELAY_PIN = 12;
const int BATTERY_PIN = 32;

// --- ตั้งค่าซิมและ MQTT ---
const char apn[] = "internet"; // เปลี่ยนตามค่ายซิมถ้าจำเป็น
const char* mqtt_server = "YOUR_DROPLET_IP";
const int   mqtt_port = 1883;

const char* topic_publish = "bidkan/bikes/BIKE-001/gps";
const char* topic_subscribe = "bidkan/bikes/BIKE-001/command";

HardwareSerial SerialAT(1);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

// ฟังก์ชันทำงานเมื่อเซิร์ฟเวอร์สั่งปลดล็อก
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("📩 [4G] ได้รับคำสั่ง: " + message);

  if (message == "UNLOCK") {
    Serial.println("🔓 ปลดล็อกรถ!");
    digitalWrite(RELAY_PIN, HIGH);
    // ในสถานการณ์จริง อาจจะปลดล็อกจนกว่าลูกค้าจะกดยืนยันคืนรถ
    delay(5000); 
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("🔒 ล็อกรถ");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  analogReadResolution(12);

  // ปลุกโมดูล 4G
  pinMode(MODEM_PWRKEY, OUTPUT);
  digitalWrite(MODEM_PWRKEY, HIGH); delay(100);
  digitalWrite(MODEM_PWRKEY, LOW);  delay(1000);
  digitalWrite(MODEM_PWRKEY, HIGH);

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  Serial.println("สตาร์ท 4G และเชื่อมต่ออินเทอร์เน็ต...");
  modem.restart();
  if (!modem.gprsConnect(apn, "", "")) {
    Serial.println("❌ ต่อเน็ตไม่สำเร็จ");
    while(true);
  }
  
  modem.enableGPS();
  Serial.println("✅ 4G พร้อม! GPS พร้อม!");

  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(mqttCallback);
}

void reconnect() {
  while (!mqtt.connected()) {
    Serial.print("Connecting MQTT...");
    if (mqtt.connect("BIKE_001_PROD")) {
      Serial.println(" ✅");
      mqtt.subscribe(topic_subscribe);
    } else {
      delay(5000);
    }
  }
}

void loop() {
  if (!mqtt.connected()) reconnect();
  mqtt.loop();

  float lat = 0, lon = 0;
  int rawBat = analogRead(BATTERY_PIN);
  int batPercent = map(rawBat, 0, 4095, 0, 100);

  // ดึงพิกัดจริงจากดาวเทียม
  if (modem.getGPS(&lat, &lon)) {
    String payload = "{\"lat\":" + String(lat, 6) + ", \"lon\":" + String(lon, 6) + ", \"battery\":" + String(batPercent) + "}";
    mqtt.publish(topic_publish, payload.c_str());
    Serial.println("🚀 [PROD] ส่งข้อมูล: " + payload);
  } else {
    Serial.println("⏳ รอสัญญาณ GPS...");
  }

  delay(5000);
}