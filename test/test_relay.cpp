#include <Arduino.h>

// --- กำหนดขาอุปกรณ์ ---
const int RELAY_PIN = 12;   // ขาสัญญาณควบคุมรีเลย์ (ล็อก/ปลดล็อก)
const int BATTERY_PIN = 32; // ขา Analog (ADC) สำหรับวัดแรงดันแบตเตอรี่

void setup() {
  Serial.begin(115200);
  delay(10);
  
  // ตั้งค่าขารีเลย์เป็นไฟออก (OUTPUT) และให้เริ่มต้นที่สถานะ ล็อกรถ (LOW)
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); 
  
  // ตั้งค่าความละเอียด ADC ของ ESP32 เป็น 12-bit
  analogReadResolution(12);
  
  Serial.println("\n--- เริ่มระบบทดสอบรวมร่าง (Relay + แบตเตอรี่ สูตร 20K & 1K) ---");
}

// --- ฟังก์ชันสำหรับอ่านและคำนวณแบตเตอรี่ ---
void checkBattery() {
  // 1. อ่านค่าดิบจากขา 32
  int rawValue = analogRead(BATTERY_PIN);
  
  // 2. แปลงค่าดิบ (2304 ถึง 3226) ให้เป็น 0% ถึง 100% ตามสูตร R1=20K, R2=1K
  int batteryPercent = map(rawValue, 2304, 3226, 0, 100);

  // 3. ป้องกันค่าเปอร์เซ็นต์หลุดกรอบ (เวลาไฟกระชาก)
  if (batteryPercent > 100) {
    batteryPercent = 100;
  } else if (batteryPercent < 0) {
    batteryPercent = 0;
  }

  // แสดงผล
  Serial.print(" -> ค่าดิบ ADC: ");
  Serial.print(rawValue);
  Serial.print(" | แบตเตอรี่เหลือ: ");
  Serial.print(batteryPercent);
  Serial.println(" %");
}

void loop() {
  // ==========================================
  // จังหวะที่ 1: สั่งปลดล็อกรถ (ทำงาน 3 วินาที)
  // ==========================================
  Serial.println("\n🔓 [คำสั่ง] สั่งปลดล็อกรถ (เปิดรีเลย์)");
  digitalWrite(RELAY_PIN, HIGH); // จ่ายไฟให้รีเลย์ (แป๊ก!)
  
  // อ่านค่าแบตเตอรี่ 3 ครั้ง (ครั้งละ 1 วินาที) ระหว่างที่รถปลดล็อกอยู่
  for (int i = 0; i < 3; i++) {
    Serial.print("[สถานะ: ปลดล็อก]");
    checkBattery(); // เรียกใช้ฟังก์ชันอ่านแบตเตอรี่ที่เราเขียนไว้ด้านบน
    delay(1000); 
  }

  // ==========================================
  // จังหวะที่ 2: สั่งล็อกรถ (ทำงาน 3 วินาที)
  // ==========================================
  Serial.println("\n🔒 [คำสั่ง] สั่งล็อกรถ (ปิดรีเลย์)");
  digitalWrite(RELAY_PIN, LOW); // ตัดไฟรีเลย์ (สับกลับ!)
  
  // อ่านค่าแบตเตอรี่ 3 ครั้ง (ครั้งละ 1 วินาที) ระหว่างที่รถล็อกอยู่
  for (int i = 0; i < 3; i++) {
    Serial.print("[สถานะ: ล็อกรถ]  ");
    checkBattery(); 
    delay(1000);
  }
}