#include <Arduino.h>

// ===== ตั้งค่า APN ตามค่ายซิม =====
// AIS / TRUE = "internet"   DTAC = "www.dtac.co.th"
#define APN "internet"

// ===== ขาของ LilyGO T-A7670 (อ้างอิง utilities.h ของ LilyGO) =====
#define MODEM_TX_PIN       26   // ESP32 ส่ง -> โมเด็ม
#define MODEM_RX_PIN       27   // ESP32 รับ  <- โมเด็ม
#define BOARD_PWRKEY_PIN   4
#define BOARD_POWERON_PIN  12   // เปิด DC boost จ่ายไฟให้โมเด็ม (ห้ามต่อรีเลย์ที่ขานี้)
#define MODEM_RESET_PIN    5
#define MODEM_RESET_LEVEL  HIGH // HIGH = สั่งรีเซ็ต
#define MODEM_DTR_PIN      25   // ต้องเป็น LOW ไม่งั้นโมเด็มอาจเข้าโหมด sleep

HardwareSerial SerialAT(1);

// ---------- ตัวช่วยส่ง AT ----------
// รอจนเจอคำใดคำหนึ่งใน endTokens หรือหมดเวลา
String readUntil(const char *tok1, const char *tok2, uint32_t timeout) {
  String resp;
  uint32_t t = millis();
  while (millis() - t < timeout) {
    while (SerialAT.available()) resp += (char)SerialAT.read();
    if ((tok1 && resp.indexOf(tok1) >= 0) || (tok2 && resp.indexOf(tok2) >= 0)) {
      delay(20);
      while (SerialAT.available()) resp += (char)SerialAT.read();
      break;
    }
  }
  return resp;
}

String sendAT(const String &cmd, uint32_t timeout = 2000, bool quiet = false) {
  while (SerialAT.available()) SerialAT.read();
  SerialAT.println(cmd);
  String resp = readUntil("OK", "ERROR", timeout);
  resp.trim();
  if (!quiet) {
    Serial.println(">> " + cmd);
    Serial.println(resp.length() ? "<< " + resp : "<< (Timeout)");
  }
  return resp;
}

// ---------- เปิดโมเด็ม ----------
void modemPowerOn() {
  pinMode(BOARD_POWERON_PIN, OUTPUT);
  digitalWrite(BOARD_POWERON_PIN, HIGH);

  pinMode(MODEM_DTR_PIN, OUTPUT);
  digitalWrite(MODEM_DTR_PIN, LOW);

  // GPIO5 ถ้าปล่อยลอยอาจค้างโมเด็มไว้ในสถานะรีเซ็ต
  pinMode(MODEM_RESET_PIN, OUTPUT);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
  delay(100);
  digitalWrite(MODEM_RESET_PIN, MODEM_RESET_LEVEL);
  delay(2600);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);

  pinMode(BOARD_PWRKEY_PIN, OUTPUT);
  digitalWrite(BOARD_PWRKEY_PIN, LOW);
  delay(100);
  digitalWrite(BOARD_PWRKEY_PIN, HIGH);
  delay(1000);
  digitalWrite(BOARD_PWRKEY_PIN, LOW);
}

// ลองทั้งสองแบบของ RX/TX เผื่อสลับกัน
bool findModem() {
  const int pinSets[2][2] = {
    {MODEM_RX_PIN, MODEM_TX_PIN},   // RX=27, TX=26 (ตาม LilyGO)
    {MODEM_TX_PIN, MODEM_RX_PIN}    // RX=26, TX=27 (แบบโค้ดเดิม)
  };
  for (int round = 0; round < 10; round++) {          // รอสูงสุด ~20 วินาที
    for (int s = 0; s < 2; s++) {
      SerialAT.end();
      SerialAT.begin(115200, SERIAL_8N1, pinSets[s][0], pinSets[s][1]);
      delay(50);
      SerialAT.println("AT");
      if (readUntil("OK", nullptr, 1000).indexOf("OK") >= 0) {
        Serial.printf("\n✅ เจอโมเด็มที่ RX=%d, TX=%d\n", pinSets[s][0], pinSets[s][1]);
        return true;
      }
    }
    Serial.print(".");
  }
  return false;
}

// ---------- ทดสอบเน็ต ----------
bool testNetwork() {
  Serial.println("\n===== 📶 ทดสอบเครือข่าย =====");

  // ซิมต้องใช้เวลาเตรียมตัวหลังโมเด็มบูต — ถามซ้ำได้สูงสุด ~30 วินาที
  Serial.print("รอซิมพร้อม");
  bool simReady = false;
  for (int i = 0; i < 15; i++) {
    if (sendAT("AT+CPIN?", 1000, true).indexOf("+CPIN: READY") >= 0) { simReady = true; break; }
    Serial.print(".");
    delay(2000);
  }
  Serial.println();
  if (!simReady) {
    Serial.println("❌ ไม่เจอซิม / ซิมติด PIN — เช็คการเสียบซิม");
    return false;
  }
  Serial.println("✅ ซิมพร้อม");

  // รอลงทะเบียนเครือข่าย 4G สูงสุด 60 วินาที
  Serial.print("รอเกาะเครือข่าย");
  bool registered = false;
  for (int i = 0; i < 30; i++) {
    String r = sendAT("AT+CEREG?", 1000, true);
    if (r.indexOf(",1") >= 0 || r.indexOf(",5") >= 0) { registered = true; break; }
    Serial.print(".");
    delay(2000);
  }
  Serial.println();
  sendAT("AT+CSQ");     // 0-31 ยิ่งมากยิ่งดี, 99 = ไม่มีสัญญาณ
  sendAT("AT+COPS?");   // ชื่อค่าย
  if (!registered) {
    Serial.println("❌ ยังไม่เกาะเครือข่าย — เช็คเสาอากาศ LTE / ซิมมีแพ็กเน็ตไหม");
    return false;
  }
  Serial.println("✅ เกาะเครือข่ายแล้ว");

  // เปิด data
  sendAT(String("AT+CGDCONT=1,\"IP\",\"") + APN + "\"");
  sendAT("AT+CGACT=1,1", 15000);
  String ip = sendAT("AT+CGPADDR=1");
  if (ip.indexOf("+CGPADDR: 1,") < 0 || ip.indexOf("0.0.0.0") >= 0) {
    Serial.println("❌ ไม่ได้ IP — เช็ค APN");
    return false;
  }

  // ยิง HTTP GET จริง
  sendAT("AT+HTTPTERM", 1000, true);   // เผื่อค้างจากรอบก่อน
  sendAT("AT+HTTPINIT");
  sendAT("AT+HTTPPARA=\"URL\",\"http://ip-api.com/json\"");
  SerialAT.println("AT+HTTPACTION=0");
  Serial.println(">> AT+HTTPACTION=0 (รอผล...)");
  String act = readUntil("+HTTPACTION:", nullptr, 20000);
  delay(200);
  while (SerialAT.available()) act += (char)SerialAT.read();
  Serial.println("<< " + act);

  bool ok = act.indexOf(",200,") >= 0;
  if (ok) {
    int p = act.lastIndexOf(',');
    int len = act.substring(p + 1).toInt();
    sendAT("AT+HTTPREAD=0," + String(len), 5000);
    Serial.println("✅ อินเทอร์เน็ตใช้งานได้");
  } else {
    Serial.println("❌ HTTP ไม่สำเร็จ");
  }
  sendAT("AT+HTTPTERM", 1000, true);
  return ok;
}

// ---------- GPS ----------
bool gpsStart() {
  Serial.println("\n===== 🛰️ เปิด GPS =====");
  while (SerialAT.available()) SerialAT.read();
  SerialAT.println("AT+CGNSSPWR=1");
  // ต้องรอข้อความ "+CGNSSPWR: READY!" เท่านั้น (คำว่า READY เฉย ๆ จะไปชนกับ +CPIN: READY)
  String r = readUntil("+CGNSSPWR: READY!", "ERROR", 30000);
  Serial.println("<< " + r);
  if (r.indexOf("+CGNSSPWR: READY!") < 0) {
    Serial.println("❌ GNSS ไม่พร้อม — ลองรีเซ็ตบอร์ดอีกรอบ");
    return false;
  }
  Serial.println("✅ GPS เปิดแล้ว — วางเสา GPS กลางแจ้ง รอ fix ครั้งแรก 1-5 นาที");
  return true;
}

// แปลง ddmm.mmmm -> องศาทศนิยม
double toDeg(const String &v, const String &hemi) {
  double x = v.toDouble();
  int d = (int)(x / 100);
  double deg = d + (x - d * 100) / 60.0;
  return (hemi == "S" || hemi == "W") ? -deg : deg;
}

void gpsRead() {
  String r = sendAT("AT+CGNSSINFO", 2000, true);
  int s = r.indexOf("+CGNSSINFO:");
  if (s < 0) { Serial.println("GPS: ไม่มีคำตอบ"); return; }
  String line = r.substring(s + 11);
  line = line.substring(0, line.indexOf('\n'));
  line.trim();

  // แยกด้วย comma
  String f[24]; int n = 0, start = 0;
  for (int i = 0; i <= (int)line.length() && n < 24; i++) {
    if (i == (int)line.length() || line[i] == ',') { f[n++] = line.substring(start, i); start = i + 1; }
  }
  // หาช่อง lat ที่ตามด้วย N/S และ lon ที่ตามด้วย E/W (กันเฟิร์มแวร์ต่างรุ่นจำนวนช่องไม่เท่ากัน)
  for (int i = 0; i + 3 < n; i++) {
    if ((f[i + 1] == "N" || f[i + 1] == "S") && (f[i + 3] == "E" || f[i + 3] == "W") && f[i].length()) {
      double rawLat = f[i].toDouble(), rawLon = f[i + 2].toDouble();
      double lat, lon;
      if (rawLat <= 90 && rawLon <= 180) {
        // เฟิร์มแวร์ A7670 รุ่นใหม่ส่งมาเป็นองศาทศนิยมอยู่แล้ว
        lat = (f[i + 1] == "S") ? -rawLat : rawLat;
        lon = (f[i + 3] == "W") ? -rawLon : rawLon;
      } else {
        // รูปแบบ ddmm.mmmm (เฟิร์มแวร์/โมดูลรุ่นอื่น)
        lat = toDeg(f[i], f[i + 1]);
        lon = toDeg(f[i + 2], f[i + 3]);
      }
      Serial.printf("📍 lat=%.6f lon=%.6f  https://maps.google.com/?q=%.6f,%.6f\n", lat, lon, lat, lon);
      return;
    }
  }
  Serial.println("⏳ GPS ยังไม่ fix (" + line + ")");
}

// ---------- main ----------
bool gpsOn = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ทดสอบ LilyGO T-A7670: โมเด็ม / เน็ต / GPS ===");

  modemPowerOn();
  Serial.print("รอโมเด็มตอบ");
  if (!findModem()) {
    Serial.println("\n❌ ยังไม่เจอโมเด็ม — เช็คไฟเลี้ยง/แบต 18650 และรุ่นบอร์ด");
    return;
  }

  sendAT("ATE0");   // ปิด echo
  sendAT("ATI");    // รุ่นโมดูล

  testNetwork();
  gpsOn = gpsStart();
}

void loop() {
  // พิมพ์ AT command เองใน Serial Monitor ได้ (passthrough)
  while (Serial.available())   SerialAT.write(Serial.read());

  static uint32_t last = 0;
  if (gpsOn && millis() - last > 10000) {
    last = millis();
    gpsRead();
  }
  while (SerialAT.available()) Serial.write(SerialAT.read());
}
