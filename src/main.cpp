#include <Arduino.h>

// กำหนดขาอุปกรณ์ตามที่เราเสียบสายไฟกันไว้
const int RELAY_PIN = 5;       // ขา Relay (จำลองสวิตช์กุญแจ)
const int ONBOARD_LED = 2;     // ขา LED บนบอร์ด
const int BUZZER_PIN = 4;      // ขา Buzzer (เสียงแตร)
const int BATTERY_PIN = 34;    // ขา Analog รับค่าโวลลุ่มจำลองแบต

void setup() {
  Serial.begin(115200);
  
  // ตั้งค่าขาที่เป็น "ตัวสั่งงาน" (OUTPUT)
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(ONBOARD_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // ตั้งค่าขาที่เป็น "ตัวรับข้อมูล" (INPUT)
  pinMode(BATTERY_PIN, INPUT);
  
  Serial.println("=== เริ่มการทดสอบฮาร์ดแวร์ (Offline Mode) ===");
}

void loop() {
  // --- ส่วนที่ 1: ทดสอบอ่านค่าแบตเตอรี่ ---
  int analogValue = analogRead(BATTERY_PIN); 
  // แปลงค่าจาก 0-4095 (ของ ESP32) ให้เป็น 0-100%
  int batteryPercent = map(analogValue, 0, 4095, 0, 100); 
  
  Serial.print("🔋 ระดับแบตเตอรี่จำลอง: ");
  Serial.print(batteryPercent);
  Serial.println("%");
  // delay(1000);

  // --- ส่วนที่ 2: สั่งปลดล็อกรถ (เปิด Relay) ---
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(ONBOARD_LED, HIGH);
  
  // // สั่ง Buzzer ดัง "ติ๊ด!" สั้นๆ 1 ครั้ง เพื่อจำลองเสียงตอนปลดล็อก
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100); 
  digitalWrite(BUZZER_PIN, LOW);
  
  Serial.println("🟢 สถานะ: ปลดล็อก (Relay ON)");
  delay(5000); 
  // รอไป 5 วินาที

  // --- ส่วนที่ 3: สั่งล็อกรถ (ปิด Relay) ---
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(ONBOARD_LED, LOW);
  Serial.println("🔴 สถานะ: ล็อกรถ (Relay OFF)");
  Serial.println("--------------------------------");
  delay(5000); 
  // รอ 5 วินาที แล้ววนลูปใหม่
}