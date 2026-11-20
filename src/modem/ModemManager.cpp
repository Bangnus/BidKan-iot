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

    // 0. จ่ายไฟเลี้ยงโมเด็ม + กันโมเด็มหลับ (ต้องทำก่อนคุยกับโมเด็ม)
    pinMode(BOARD_POWERON_PIN, OUTPUT);
    digitalWrite(BOARD_POWERON_PIN, HIGH);
    pinMode(MODEM_DTR_PIN, OUTPUT);
    digitalWrite(MODEM_DTR_PIN, LOW);
    pinMode(MODEM_RESET_PIN, OUTPUT);
    digitalWrite(MODEM_RESET_PIN, LOW);
    pinMode(pwrKey, OUTPUT);
    digitalWrite(pwrKey, LOW);

    // เริ่ม Serial ก่อนเพื่อเช็คสถานะ
    _serialAT.begin(MODEM_BAUD, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
    delay(100);

    // 1. เช็คว่าชิป 4G เปิดอยู่แล้วหรือยัง
    if (isModemReady()) {
        Serial.println("[Modem] ✅ ชิป 4G เปิดอยู่แล้ว — ข้ามขั้นตอนกดปุ่มเปิด");
        return;
    }

    // 2. รีเซ็ตโมเด็ม แล้วกด PWRKEY ตามลำดับทางการของ LilyGO (LOW -> HIGH 1s -> LOW)
    Serial.println("[Modem] 🔋 กำลังรีเซ็ตและกดปุ่มเปิดชิป 4G...");
    digitalWrite(MODEM_RESET_PIN, HIGH);
    delay(2600);
    digitalWrite(MODEM_RESET_PIN, LOW);
    digitalWrite(pwrKey, LOW);
    delay(100);
    digitalWrite(pwrKey, HIGH);
    delay(1000);
    digitalWrite(pwrKey, LOW);
    
    // 3. รอให้ชิปบูตเสร็จ (delay 1s + timeout 1s = ~2s ต่อรอบ, รวม 10 รอบ = สูงสุด ~20 วินาที)
    Serial.print("[Modem] รอชิป 4G บูต.");
    bool booted = false;
    for (int i = 0; i < 10; i++) {
        delay(1000);
        Serial.print(".");
        if (isModemReady()) {
            booted = true;
            break;
        }
    }
    Serial.println();

    if (booted) {
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

    // 1. Power on (จะเช็คว่าเปิดอยู่แล้วหรือยัง ก่อนกด PWRKEY และมี retry loop 10s แล้ว)
    powerOn();

    // 2. Init modem (ผ่าน TinyGSM)
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
        delay(GPRS_RETRY_DELAY_MS);
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

bool ModemManager::getGPSData(float &lat, float &lon) {
    // ใช้ AT+CGNSSINFO เพื่อดึงพิกัด GNSS
    // Response format: +CGNSSINFO: <mode>,<GPS-SVs>,<GLONASS-SVs>,...,<lat>,<N/S>,<lon>,<E/W>,...
    _modem.sendAT("+CGNSSINFO");
    
    String response = "";
    if (_modem.waitResponse(10000L, "+CGNSSINFO:") == 1) {
        // ตั้ง timeout เพื่อไม่ให้จมถ้าเกิดอ่านไม่จบ
        _modem.stream.setTimeout(2000); 
        response = _modem.stream.readStringUntil('\n');
        _modem.stream.setTimeout(1000); // รีเซ็ตกลับเป็นค่าเริ่มต้นมาตรฐาน
        _modem.waitResponse(); // consume OK
    } else {
        return false; // Time out waiting for +CGNSSINFO
    }

    if (response.length() == 0) {
        Serial.println("[Modem] ⚠️ GPS ตอบกลับแต่ไม่มีข้อมูล");
        return false;
    }

    return parseGNSSInfo(response, lat, lon);
}

bool ModemManager::parseGNSSInfo(const String& response, float &lat, float &lon) {
    // +CGNSSINFO: <mode>,<GPS-SVs>,<GLONASS-SVs>,<BEIDOU-SVs>,<lat>,<N/S>,<lon>,<E/W>,<date>,<UTC-time>,<alt>,<speed>,<course>
    // Example:  3,08,04,01,1345.5678,N,10030.1234,E,150726,110000.0,25.3,0.5,180.2 (Note: "+CGNSSINFO:" was already parsed out)
    // If no fix:  ,,,,,,,,
    
    if (response.length() < 10) return false;
    
    // Helper lambda to get nth comma-separated value
    auto getField = [](const String& str, int index) -> String {
        int commaCount = 0;
        int startIndex = 0;
        for (int i = 0; i < str.length(); i++) {
            if (str[i] == ',') {
                if (commaCount == index) {
                    String val = str.substring(startIndex, i);
                    val.trim();
                    return val;
                }
                commaCount++;
                startIndex = i + 1;
            }
        }
        // Handle last field
        if (commaCount == index) {
            String val = str.substring(startIndex);
            val.trim();
            return val;
        }
        return "";
    };
    
    // หาช่อง lat ที่ตามด้วย N/S และ lon ที่ตามด้วย E/W
    // (เฟิร์มแวร์แต่ละรุ่นจำนวนช่องไม่เท่ากัน จึงไม่ fix index)
    for (int i = 0; i < 20; i++) {
        String latDir = getField(response, i + 1);
        String lonDir = getField(response, i + 3);
        if ((latDir != "N" && latDir != "S") || (lonDir != "E" && lonDir != "W")) continue;

        String latStr = getField(response, i);
        String lonStr = getField(response, i + 2);
        if (latStr.length() == 0 || lonStr.length() == 0) return false;

        double latRaw = latStr.toDouble();
        double lonRaw = lonStr.toDouble();
        if (latRaw <= 90 && lonRaw <= 180) {
            // A7670 V1.11 ส่งเป็นองศาทศนิยมอยู่แล้ว (ยืนยันจากบอร์ดจริง)
            lat = latRaw;
            lon = lonRaw;
        } else {
            // รูปแบบ DDMM.MMMM (เฟิร์มแวร์รุ่นอื่น)
            int latDeg = (int)(latRaw / 100);
            int lonDeg = (int)(lonRaw / 100);
            lat = latDeg + (latRaw - latDeg * 100) / 60.0;
            lon = lonDeg + (lonRaw - lonDeg * 100) / 60.0;
        }
        if (latDir == "S") lat = -lat;
        if (lonDir == "W") lon = -lon;
        return true;
    }
    return false; // ยังไม่ fix
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
