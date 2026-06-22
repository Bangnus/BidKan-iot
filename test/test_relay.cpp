#include <Arduino.h>

// --- กำหนดขาอุปกรณ์ ---
const int RELAY_IGNITION_PIN = 12; // Relay 1: สวิตช์กุญแจ (จ่ายไฟให้รถ)
const int RELAY_LOCK_PIN = 14;     // Relay 2: สั่งล็อกล้อ (ต่อตรง Controller)
const int BATTERY_PIN = 32;        // ขาวัดแบตเตอรี่ (วงจร 20K & 1K)

void setup() {
  Serial.begin(115200);
  delay(10);
  
  // 1. ตั้งค่าขารีเลย์เป็น OUTPUT
  pinMode(RELAY_IGNITION_PIN, OUTPUT);
  pinMode(RELAY_LOCK_PIN, OUTPUT);
  
  // 2. 🔒 สถานะเริ่มต้นตอนเสียบไฟ: รถจอดล็อกอยู่
  digitalWrite(RELAY_IGNITION_PIN, LOW); // ดับเครื่อง (กุญแจ OFF)
  digitalWrite(RELAY_LOCK_PIN, HIGH);    // สั่งล็อกล้อค้างไว้ (เชื่อมสัญญาณลง GND)
  
  // 3. ตั้งค่าการอ่านแบตเตอรี่
  analogReadResolution(12);
  
  Serial.println("\n--- เริ่มระบบ: แบบ B (ต่อตรงล็อกล้อ) + กุญแจ + วัดแบต ---");
}

void checkBattery() {
  int rawValue = analogRead(BATTERY_PIN);
  int batteryPercent = map(rawValue, 2304, 3226, 0, 100);

  if (batteryPercent > 100) batteryPercent = 100;
  if (batteryPercent < 0) batteryPercent = 0;

  Serial.print(" | แบตเตอรี่เหลือ: ");
  Serial.print(batteryPercent);
  Serial.println(" %");
}

void loop() {
  // ==========================================
  // เหตุการณ์ที่ 1: ลูกค้าสแกนเช่ารถ (Unlock)
  // ==========================================
  Serial.println("\n🔓 [คำสั่ง] ปลดล็อกรถ...");
  
  // สเต็ป 1: คลายล็อกล้อก่อน
  digitalWrite(RELAY_LOCK_PIN, LOW); // ตัดการเชื่อมต่อกราวด์ ล้อหมุนได้อิสระ
  delay(500); // รอให้ล้อคลายตัวครึ่งวินาที
  
  // สเต็ป 2: เปิดระบบไฟรถ
  digitalWrite(RELAY_IGNITION_PIN, HIGH); // จ่ายไฟกุญแจ
  
  Serial.print(">>> รถพร้อมขับขี่");
  checkBattery();
  
  delay(5000); // จำลองลูกค้ากำลังขี่รถไป 5 วินาที

  // ==========================================
  // เหตุการณ์ที่ 2: ลูกค้ากดยืนยันคืนรถ (Lock)
  // ==========================================
  Serial.println("\n🔒 [คำสั่ง] ล็อกรถ...");
  
  // สเต็ป 1: ดับเครื่องยนต์ก่อน
  digitalWrite(RELAY_IGNITION_PIN, LOW); // ตัดไฟกุญแจ
  delay(1000); // รอ 1 วินาทีให้รถดับสนิท
  
  // สเต็ป 2: สั่งล็อกล้อ
  digitalWrite(RELAY_LOCK_PIN, HIGH); // ดึงสัญญาณลงกราวด์ ล้อล็อกหนืดทันที
  
  Serial.print(">>> รถล็อกสมบูรณ์ เข็นไม่ได้");
  checkBattery();

  delay(5000); // จำลองการจอดรอรลูกค้าคนต่อไป 5 วินาที
}