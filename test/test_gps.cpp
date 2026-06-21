#include <Arduino.h>

// ขาเชื่อมต่อของ LilyGO T-A7670SA
#define MODEM_RX 26
#define MODEM_TX 27
#define MODEM_PWRKEY 4

HardwareSerial SerialAT(1);

// ฟังก์ชันส่ง AT Command
String sendAT(String command, const int timeout) {
  String response = "";
  SerialAT.println(command);
  long int time = millis();
  while ((time + timeout) > millis()) {
    while (SerialAT.available()) {
      char c = SerialAT.read();
      response += c;
    }
  }
  Serial.print(response);
  return response;
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // ปลุกชิป 4G
  pinMode(MODEM_PWRKEY, OUTPUT);
  digitalWrite(MODEM_PWRKEY, HIGH); delay(100);
  digitalWrite(MODEM_PWRKEY, LOW);  delay(1000);
  digitalWrite(MODEM_PWRKEY, HIGH);

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  Serial.println("\n--- เริ่มทดสอบฮาร์ดแวร์ A7670SA ---");
  sendAT("AT", 1000); // เช็กสถานะชิป
  sendAT("AT+CPIN?", 1000); // เช็กซิมการ์ด
  sendAT("AT+CSQ", 1000); // เช็กสัญญาณคลื่นโทรศัพท์

  Serial.println("\n--- เปิดระบบ GPS ---");
  sendAT("AT+CGNSSPWR=1", 2000); // จ่ายไฟให้ชิป GPS
}

void loop() {
  Serial.println("\n[ ดึงข้อมูลพิกัดดาวเทียม... ]");
  sendAT("AT+CGNSSINFO", 2000); // ขอพิกัด
  delay(5000);
}