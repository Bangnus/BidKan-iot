#include "ModemManager.h"

ModemManager::ModemManager()
    : _serialAT(1)
    , _modem(_serialAT)
    , _client(_modem)
{
}

// ฟังก์ชันเช็คว่าชิป 4G เปิดอยู่แล้วหรือยัง (ลองส่ง AT ไปถาม)
bool ModemManager::isModemReady() {
    // ล้างบัฟเฟอร์เก่าทิ้งก่อนเผื่อมีขยะค้าง
    while(_serialAT.available()) _serialAT.read(); 
    
    _serialAT.println("AT");
    _serialAT.setTimeout(1000); // ตั้งเวลารอสูงสุด 1 วินาที
    
    // find() จะหยุดรอทันทีที่เจอคำว่า "OK" (ทำงานเร็วกว่าการดัก delay ตายตัว)
    if (_serialAT.find("OK")) {
        return true;
    }
    return false;
}

void ModemManager::powerOn() {
    // PWRKEY ทำหน้าที่เป็นทั้งปุ่ม ON และ OFF (เหมือนปุ่มพาวเวอร์บนมือถือ)
    // ถ้าชิปเปิดอยู่แล้ว การกด PWRKEY ซ้ำจะเป็นการ "ปิด" มันแทน!
    // ดังนั้นต้องเช็คก่อนว่าเปิดอยู่หรือยัง

    int pwrKey = MODEM_PWRKEY_PIN;
    pinMode(pwrKey, OUTPUT);

    // เริ่ม Serial ก่อนเพื่อเช็คสถานะ
    _serialAT.begin(MODEM_BAUD, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
    delay(100);

    // 1. เช็คว่าชิป 4G เปิดอยู่แล้วหรือยัง
    if (isModemReady()) {
        Serial.println("[Modem] ✅ ชิป 4G เปิดอยู่แล้ว — ข้ามขั้นตอนกดปุ่มเปิด");
        return;
    }

    // 2. ถ้ายังไม่เปิด ค่อยกดปุ่ม PWRKEY (HIGH -> LOW -> HIGH)
    Serial.println("[Modem] 🔋 กำลังกดปุ่มเปิดชิป 4G...");
    digitalWrite(pwrKey, HIGH);
    delay(100);
    digitalWrite(pwrKey, LOW);
    delay(1000);
    digitalWrite(pwrKey, HIGH);
    delay(3000); // รอให้ชิปบูตเสร็จ

    // 3. เช็คอีกรอบเพื่อความมั่นใจ
    if (isModemReady()) {
        Serial.println("[Modem] ✅ เปิดชิป 4G สำเร็จ!");
    } else {
        Serial.println("[Modem] ⚠️ ชิป 4G ยังไม่ตอบ — อาจต้องเช็คไฟเลี้ยงหรือซิมการ์ด");
    }
}

void ModemManager::powerOff() {
    Serial.println("[Modem] 🔌 กำลังปิดโมดูล 4G...");
    _modem.poweroff();
}

bool ModemManager::begin() {
    Serial.println("[Modem] กำลังเปิดโมดูล 4G...");

    // 1. Power on (จะเช็คว่าเปิดอยู่แล้วหรือยัง ก่อนกด PWRKEY)
    powerOn();

    // 2. รอให้ชิปพร้อม
    delay(MODEM_BOOT_DELAY_MS);

    // 3. Init modem (ผ่าน TinyGSM)
    if (!_modem.init()) {
        Serial.println("[Modem] ❌ ไม่สามารถติดต่อโมดูล 4G ได้");
        return false;
    }

    String imei = _modem.getIMEI();
    Serial.println("[Modem] IMEI: " + imei);

    // 4. เปิด GPS ทันทีที่ชิปพร้อม (ไม่ต้องรอเน็ต)
    enableGPS();

    // 5. เชื่อมต่อ GPRS
    if (!connectGPRS()) {
        Serial.println("[Modem] ⚠️ เริ่มต้นระบบโดยไม่มีอินเทอร์เน็ต (ออฟไลน์โหมด)");
        // return true ไปก่อน เพื่อให้ loop หลักทำงานต่อ และคอยพยายามต่อเน็ตใหม่ใน loop
    } else {
        Serial.println("[Modem] ✅ 4G + GPS พร้อมใช้งาน!");
    }

    return true; // คืนค่า true เสมอ เพื่อให้ระบบหลัก (GPS/BLE) ทำงานต่อได้
}

bool ModemManager::connectGPRS() {
    Serial.println("[Modem] กำลังเชื่อมต่ออินเทอร์เน็ต GPRS...");

    for (int attempt = 1; attempt <= GPRS_CONNECT_MAX_RETRIES; attempt++) {
        Serial.print("[Modem] ลองครั้งที่ ");
        Serial.print(attempt);
        Serial.print("/");
        Serial.println(GPRS_CONNECT_MAX_RETRIES);

        if (_modem.gprsConnect(APN_NAME, APN_USER, APN_PASS)) {
            Serial.println("[Modem] ✅ เชื่อมต่ออินเทอร์เน็ตสำเร็จ!");

            IPAddress localIP = _modem.localIP();
            Serial.print("[Modem] IP: ");
            Serial.println(localIP);
            return true;
        }

        Serial.println("[Modem] ❌ ล้มเหลว รอ retry...");
        delay(MQTT_RECONNECT_DELAY_MS);
    }

    Serial.println("[Modem] ❌ เชื่อมต่อ GPRS ไม่สำเร็จหลังจากลองครบแล้ว");
    return false;
}

bool ModemManager::enableGPS() {
    Serial.println("[Modem] เปิด GPS...");
    // เปิดฮาร์ดแวร์ GNSS ผ่าน AT command
    _modem.sendAT("+CGNSSPWR=1");
    if (_modem.waitResponse(10000L) != 1) {
        Serial.println("[Modem] ⚠️ เปิด GPS ไม่สำเร็จ");
        return false;
    }
    Serial.println("[Modem] ✅ GPS เปิดแล้ว");
    return true;
}

bool ModemManager::getGPSData(float &lat, float &lon, float &speed) {
    // ใช้ AT+CGNSSINFO เพื่อดึงพิกัด GNSS
    // Response format: +CGNSSINFO: <mode>,<GPS-SVs>,<GLONASS-SVs>,...,<lat>,<N/S>,<lon>,<E/W>,...
    _modem.sendAT("+CGNSSINFO");
    
    String response = "";
    if (_modem.waitResponse(10000L, "+CGNSSINFO:") == 1) {
        response = _modem.stream.readStringUntil('\n');
        _modem.waitResponse(); // consume OK
    } else {
        return false;
    }

    return parseGNSSInfo(response, lat, lon, speed);
}

bool ModemManager::parseGNSSInfo(const String& response, float &lat, float &lon, float &speed) {
    // +CGNSSINFO: <mode>,<GPS-SVs>,<GLONASS-SVs>,<BEIDOU-SVs>,<lat>,<N/S>,<lon>,<E/W>,<date>,<UTC-time>,<alt>,<speed>,<course>
    // Example: +CGNSSINFO: 3,08,04,01,1345.5678,N,10030.1234,E,150726,110000.0,25.3,0.5,180.2
    // If no fix: +CGNSSINFO: ,,,,,,,,
    
    if (response.length() < 10) return false;
    
    // Check if we have a valid fix (mode > 0)
    int firstComma = response.indexOf(',');
    if (firstComma < 0) return false;
    
    String mode = response.substring(0, firstComma);
    mode.trim();
    if (mode.length() == 0 || mode.toInt() == 0) return false;
    
    // Parse: skip mode, GPS-SVs, GLONASS-SVs, BEIDOU-SVs to get lat
    int commaCount = 0;
    int pos = 0;
    String fields[13];
    int fieldIndex = 0;
    
    for (int i = 0; i <= (int)response.length() && fieldIndex < 13; i++) {
        if (i == (int)response.length() || response[i] == ',') {
            fields[fieldIndex] = response.substring(pos, i);
            fields[fieldIndex].trim();
            fieldIndex++;
            pos = i + 1;
        }
    }
    
    if (fieldIndex < 8) return false;
    
    // fields[4] = lat (DDMM.MMMM), fields[5] = N/S
    // fields[6] = lon (DDDMM.MMMM), fields[7] = E/W
    String latStr = fields[4];
    String latDir = fields[5];
    String lonStr = fields[6];
    String lonDir = fields[7];
    
    if (latStr.length() == 0 || lonStr.length() == 0) return false;
    
    // Convert DDMM.MMMM to decimal degrees
    float latRaw = latStr.toFloat();
    int latDeg = (int)(latRaw / 100);
    float latMin = latRaw - (latDeg * 100);
    lat = latDeg + (latMin / 60.0f);
    if (latDir == "S") lat = -lat;
    
    float lonRaw = lonStr.toFloat();
    int lonDeg = (int)(lonRaw / 100);
    float lonMin = lonRaw - (lonDeg * 100);
    lon = lonDeg + (lonMin / 60.0f);
    if (lonDir == "W") lon = -lon;
    
    // Parse speed (fields[11]) if available
    speed = 0;
    if (fieldIndex > 11 && fields[11].length() > 0) {
        speed = fields[11].toFloat();
    }
    
    return true;
}

String ModemManager::getIMEI() {
    return _modem.getIMEI();
}

TinyGsmClient& ModemManager::getClient() {
    return _client;
}

TinyGsm& ModemManager::getModem() {
    return _modem;
}
