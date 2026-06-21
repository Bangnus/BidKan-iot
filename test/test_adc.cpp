#include <Arduino.h>

// กำหนดขา Analog สำหรับอ่านแบตเตอรี่
const int BATTERY_PIN = 32;

void setup() {
  Serial.begin(115200);
  
  // ตั้งค่าความละเอียดของ ADC เป็น 12-bit (อ่านค่าได้ 0 - 4095)
  analogReadResolution(12); 
  Serial.println("--- เริ่มทดสอบการอ่านค่าแบตเตอรี่ (ADC) ---");
}

void loop() {
  // 1. อ่านค่าดิบจากขา 32
  int rawValue = analogRead(BATTERY_PIN);
  
  // 2. แปลงค่าดิบ (0-4095) ให้เป็นเปอร์เซ็นต์ (0-100%)
  int batteryPercent = map(rawValue, 0, 4095, 0, 100);

  // 3. แสดงผลทางหน้าจอ
  Serial.print("ค่าดิบ ADC: ");
  Serial.print(rawValue);
  Serial.print("  |  ประเมินแบตเตอรี่: ");
  Serial.print(batteryPercent);
  Serial.println(" %");

  delay(1000); // อัปเดตค่าทุกๆ 1 วินาที
}