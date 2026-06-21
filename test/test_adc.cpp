// 1. อ่านค่าดิบจากขา 32
  int rawValue = analogRead(BATTERY_PIN);
  
  // 2. แปลงค่าดิบ (2304 ถึง 3226) ให้เป็น 0% ถึง 100%
  int batteryPercent = map(rawValue, 2304, 3226, 0, 100);

  // 3. ป้องกันค่าหลุดกรอบ (เช่น จังหวะเบรกหรือบิดคันเร่งแรงๆ ไฟอาจจะแกว่ง)
  if (batteryPercent > 100) {
    batteryPercent = 100;
  } else if (batteryPercent < 0) {
    batteryPercent = 0;
  }

  Serial.print("ค่าดิบ ADC: ");
  Serial.print(rawValue);
  Serial.print(" | แบตเตอรี่รถเหลือ: ");
  Serial.print(batteryPercent);
  Serial.println(" %");