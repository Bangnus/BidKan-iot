#define TINY_GSM_MODEM_SIM7600 
#define TINY_GSM_RX_BUFFER 1024

#include <TinyGsmClient.h>
#include <PubSubClient.h>

// --- ขาฮาร์ดแวร์ ---
const int RELAY_IGNITION_PIN = 12; // Relay 1 -> คุมกุญแจรถ
const int RELAY_LOCK_PIN = 14;     // Relay 2 -> คุมล็อกล้อ

#define MODEM_RX     26
#define MODEM_TX     27
#define MODEM_PWRKEY  4

// --- การตั้งค่าเครือข่าย (ปรับตามซิมของคุณ Peerapat) ---
const char apn[]      = "internet"; 
const char gprsUser[] = "";
const char gprsPass[] = "";

// --- MQTT Server ---
const char* mqtt_server = "broker.hivemq.com"; 
const int mqtt_port    = 1883;

// ตัวแปรเก็บประวัติ ID รถ (IMEI) และสถานะรถ
String bikeID = ""; 
String topic_sub = "";
String topic_pub = "";
unsigned long lastStatusTime = 0;
bool currentVehicleState = false; // false = ล็อกอยู่, true = ปลดล็อกใช้งาน

HardwareSerial SerialAT(1);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

// --- ฟังก์ชันรับคำสั่ง (Callback) สั่ง LOCK / UNLOCK ---
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("\n📥 [MQTT ได้รับคำสั่ง]: ");
  Serial.println(message);

  if (message == "UNLOCK") {
    Serial.println("🔓 ดำเนินการปลดล็อกรถ...");
    digitalWrite(RELAY_LOCK_PIN, LOW);       // ดับรีเลย์ล็อกล้อ (ปล่อยล้อฟรี)
    delay(500);
    digitalWrite(RELAY_IGNITION_PIN, HIGH);  // เปิดรีเลย์กุญแจ (รถติด)
    currentVehicleState = true;
    sendVehicleStatus();                     // ส่งสเตตัสกลับทันทีเพื่ออัปเดตหน้าจอแอป
  } 
  else if (message == "LOCK") {
    Serial.println("🔒 ดำเนินการล็อกรถ...");
    digitalWrite(RELAY_IGNITION_PIN, LOW);   // ดับรีเลย์กุญแจ (ดับเครื่อง)
    delay(1000);
    digitalWrite(RELAY_LOCK_PIN, HIGH);     // เปิดรีเลย์ล็อกล้อ (ล้อหนืดตาย)
    currentVehicleState = false;
    sendVehicleStatus();                     // ส่งสเตตัสกลับทันที
  }
}

// --- ฟังก์ชันส่งพิกัด GPS และสถานะรถขึ้น MQTT ---
void sendVehicleStatus() {
  float lat = 0, lon = 0, speed = 0;
  int alt = 0, vsat = 0, usat = 0;
  
  String payload = "{\"bike_id\":\"" + bikeID + "\",";

  Serial.println("\n🛰️ กำลังตรวจสอบพิกัด GPS...");
  if (modem.getGPS(&lat, &lon, &speed, &alt, &vsat, &usat)) {
    payload += "\"lat\":" + String(lat, 6) + ",";
    payload += "\"lon\":" + String(lon, 6) + ",";
    payload += "\"speed\":" + String(speed, 1) + ",";
  } else {
    // หากทดสอบในอาคารแล้วไม่มีสัญญาณดาวเทียม ให้ส่งค่าจำลองไปก่อน
    payload += "\"lat\":13.7563,\"lon\":100.5018,\"speed\":0.0,";
    Serial.println("⚠️ GPS No Fix (ใช้ค่าพิกัดจำลอง)");
  }
  
  payload += "\"status\":\"" + String(currentVehicleState ? "ACTIVE" : "LOCKED") + "\"}";
  
  mqtt.publish(topic_pub.c_str(), payload.c_str());
  Serial.print("📤 ส่งข้อมูลสถานะไปที่ [");
  Serial.print(topic_pub);
  Serial.println("]");
  Serial.println("Payload: " + payload);
}

// --- ฟังก์ชันเชื่อมต่อ MQTT และจัดการ Dynamic Topic ---
void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("🔄 กำลังเชื่อมต่อ MQTT Broker...");
    
    // ใช้เลข IMEI เป็น Client ID เพื่อไม่ให้สับสนกับรถคันอื่น
    String clientId = "SmartMile-Bike-" + bikeID;
    
    if (mqtt.connect(clientId.c_str())) {
      Serial.println("✅ เชื่อมต่อสำเร็จ!");
      
      // สั่ง Subscribe ช่องรับคำสั่งเฉพาะของรถคันนี้
      mqtt.subscribe(topic_sub.c_str());
      Serial.println("📡 Subscribe ไปที่ช่องรับคำสั่ง: " + topic_sub);
    } else {
      Serial.print("❌ ล้มเหลว, state=");
      Serial.print(mqtt.state());
      Serial.println(" จะลองใหม่ใน 5 วินาที");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);
  
  Serial.println("\n=============================================");
  Serial.println("🚀 ระบบ Smart-Mile: ตรวจสอบและควบคุมรถผ่าน 4G");
  Serial.println("=============================================");

  pinMode(RELAY_IGNITION_PIN, OUTPUT);
  pinMode(RELAY_LOCK_PIN, OUTPUT);
  
  // สเตตัสเริ่มต้น: รถล็อกอยู่
  digitalWrite(RELAY_IGNITION_PIN, LOW); 
  digitalWrite(RELAY_LOCK_PIN, HIGH);

  // เปิดโมดูล 4G
  pinMode(MODEM_PWRKEY, OUTPUT);
  digitalWrite(MODEM_PWRKEY, LOW); delay(100);
  digitalWrite(MODEM_PWRKEY, HIGH); delay(1000);
  digitalWrite(MODEM_PWRKEY, LOW);
  
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(5000);

  if (!modem.init()) {
    Serial.println("❌ ไม่สามารถติดต่อกับโมดูล 4G ได้");
    while(true);
  }

  // 🆔 ดึงค่าเลข IMEI มาเก็บไว้ทำเป็น ID รถคันนี้
  bikeID = modem.getIMEI();
  Serial.println("🆔 ID รถคันนี้ (IMEI): " + bikeID);

  // สร้างชื่อ Topic อ้างอิงตาม ID รถโดยอัตโนมัติ
  topic_sub = "smartmile/" + bikeID + "/command"; // ช่องรอรับคำสั่ง LOCK/UNLOCK
  topic_pub = "smartmile/" + bikeID + "/status";  // ช่องส่งสเตตัสพิกัดออกไป

  Serial.println("📡 กำลังเชื่อมต่ออินเทอร์เน็ต GPRS...");
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println("❌ เชื่อมต่ออินเทอร์เน็ตไม่ได้");
    while(true);
  }
  Serial.println("✅ อินเทอร์เน็ตพร้อมใช้งาน!");

  // ดู IP ขำๆ เผื่ออยากรู้ แต่อย่างที่บอกครับมันจะเปลี่ยนไปเรื่อยๆ ตามค่ายมือถือ
  IPAddress localIP = modem.localIP();
  Serial.print("🌐 Private IP จากซิมการ์ด: ");
  Serial.println(localIP);

  // เปิดฮาร์ดแวร์ GPS
  modem.sendAT("+CGNSSPWR=1");
  modem.waitResponse();

  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(mqttCallback);
}

void loop() {
  if (!mqtt.connected()) {
    connectMQTT();
  }
  mqtt.loop();

  // ส่งพิกัด GPS อัปเดตสเตตัสอัตโนมัติทุกๆ 15 วินาที
  if (millis() - lastStatusTime > 15000) {
    lastStatusTime = millis();
    sendVehicleStatus();
  }
}