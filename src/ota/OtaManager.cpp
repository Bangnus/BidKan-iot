#include "OtaManager.h"
#include <esp_task_wdt.h>

void OtaManager::printProgress(size_t written, size_t total) {
    static int lastPercent = 0;
    int percent = (written * 100) / total;
    if (percent != lastPercent) {
        Serial.printf("[OTA] Progress: %d%%\n", percent);
        lastPercent = percent;
    }
}

bool OtaManager::startUpdate(TinyGsmClient& client, const String& url) {
    Serial.println("[OTA] เริ่มการทำ FOTA...");
    Serial.println("[OTA] URL: " + url);

    // 1. แยก Host, Port, Path จาก URL
    String protocol = "http://";
    String host = "";
    String path = "/";
    int port = 80;

    String tempUrl = url;
    if (tempUrl.startsWith(protocol)) {
        tempUrl = tempUrl.substring(protocol.length());
    } else {
        Serial.println("[OTA] ❌ รองรับแค่ http:// เท่านั้น");
        return false;
    }

    int slashIndex = tempUrl.indexOf('/');
    if (slashIndex >= 0) {
        host = tempUrl.substring(0, slashIndex);
        path = tempUrl.substring(slashIndex);
    } else {
        host = tempUrl;
    }

    int colonIndex = host.indexOf(':');
    if (colonIndex >= 0) {
        port = host.substring(colonIndex + 1).toInt();
        host = host.substring(0, colonIndex);
    }

    Serial.println("[OTA] Host: " + host + ", Port: " + String(port) + ", Path: " + path);

    // 2. ใช้ ArduinoHttpClient ดึงข้อมูล
    HttpClient http(client, host, port);
    http.connectionKeepAlive(); // ช่วยไม่ให้เน็ตตัดระหว่างทาง
    
    Serial.println("[OTA] กำลังเชื่อมต่อเซิร์ฟเวอร์...");
    int err = http.get(path);
    if (err != 0) {
        Serial.printf("[OTA] ❌ เชื่อมต่อล้มเหลว (Error %d)\n", err);
        return false;
    }

    int statusCode = http.responseStatusCode();
    Serial.printf("[OTA] Status code: %d\n", statusCode);
    if (statusCode != 200) {
        Serial.println("[OTA] ❌ ดาวน์โหลดไฟล์ไม่สำเร็จ (HTTP Status != 200)");
        return false;
    }

    long contentLength = http.contentLength();
    Serial.printf("[OTA] ขนาดไฟล์: %ld bytes\n", contentLength);
    if (contentLength <= 0) {
        Serial.println("[OTA] ❌ ไม่พบขนาดไฟล์ (Content-Length)");
        return false;
    }

    // 3. เริ่มกระบวนการเขียนลง Flash
    bool canBegin = Update.begin(contentLength, U_FLASH);
    if (!canBegin) {
        Serial.printf("[OTA] ❌ ไม่สามารถเริ่ม Update ได้: %s\n", Update.errorString());
        return false;
    }

    Serial.println("[OTA] กำลังดาวน์โหลดและเขียนลง Flash...");
    
    size_t written = 0;
    uint8_t buffer[1024]; // รับทีละ 1KB
    int timeoutCount = 0;

    while (http.connected() || http.available()) {
        esp_task_wdt_reset(); // เลี้ยง Watchdog ตลอดการโหลด

        if (http.available()) {
            int len = http.read(buffer, sizeof(buffer));
            if (len > 0) {
                Update.write(buffer, len);
                written += len;
                printProgress(written, contentLength);
                timeoutCount = 0;
            }
        } else {
            delay(10);
            timeoutCount++;
            if (timeoutCount > 1000) { // 10 วินาทีถ้าข้อมูลไม่มาเลย
                Serial.println("[OTA] ❌ ข้อมูลขาดหาย (Timeout)");
                break;
            }
        }

        if (written >= contentLength) {
            break; // ครบแล้ว
        }
    }

    // 4. สรุปผล
    if (written == contentLength) {
        if (Update.end()) {
            if (Update.isFinished()) {
                Serial.println("[OTA] ✅ อัปเดตเสร็จสมบูรณ์! กำลังรีสตาร์ท...");
                delay(1000);
                ESP.restart(); // รีสตาร์ททันที
                return true;
            } else {
                Serial.println("[OTA] ❌ อัปเดตไม่สมบูรณ์");
            }
        } else {
            Serial.printf("[OTA] ❌ ผิดพลาดตอนจบ: %s\n", Update.errorString());
        }
    } else {
        Serial.printf("[OTA] ❌ โหลดมาได้แค่ %d จาก %ld bytes\n", written, contentLength);
        Update.abort(); // ยกเลิก
    }

    return false;
}
