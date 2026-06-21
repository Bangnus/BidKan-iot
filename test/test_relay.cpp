#include <Arduino.h>

// กำหนดขาที่ต่อกับสัญญาณรีเลย์
const int RELAY_PIN = 12;

void setup() {
  Serial.begin(115200);
  
  // ตั้งค่าขา 12 ให้เป็นขาปล่อยสัญญาณไฟ (OUTPUT)
  pinMode(RELAY_PIN, OUTPUT);
  
  // ค่าเริ่มต้น: ล็อกรถ (ปิดรีเลย์)
  digitalWrite(RELAY_PIN, LOW); 
  Serial.println("--- เริ่มทดสอบระบบล็อก/ปลดล็อก (Relay) ---");
}

void loop() {
  Serial.println("🔓 สั่งปลดล็อกรถ (เปิดรีเลย์)");
  digitalWrite(RELAY_PIN, HIGH); // จ่ายไฟให้รีเลย์ จะได้ยินเสียง "แป๊ก!"
  delay(3000); // รอ 3 วินาที

  Serial.println("🔒 สั่งล็อกรถ (ปิดรีเลย์)");
  digitalWrite(RELAY_PIN, LOW); // ตัดไฟรีเลย์ จะได้ยินเสียงสับกลับ
  delay(3000); // รอ 3 วินาที
}