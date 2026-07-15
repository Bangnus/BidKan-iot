#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <Update.h>
#include <ArduinoHttpClient.h>
#include <TinyGsmClient.h>

/**
 * OtaManager — จัดการดาวน์โหลดและอัปเดต Firmware (FOTA)
 * 
 * รับผิดชอบ:
 * - เชื่อมต่อ HTTP ไปยัง URL ที่กำหนด
 * - ดาวน์โหลดไฟล์ .bin
 * - เขียนลง Flash Memory ด้วย Update class
 * - รีสตาร์ทระบบเมื่อเสร็จสิ้น
 */
class OtaManager {
public:
    /// เริ่มต้นกระบวนการ OTA
    /// @param client GsmClient ที่ใช้เชื่อมต่ออินเทอร์เน็ต
    /// @param url URL ของไฟล์ firmware (.bin)
    /// @return true ถ้าสำเร็จ (ซึ่งความจริงมันจะ restart ไปก่อน), false ถ้าล้มเหลว
    static bool startUpdate(TinyGsmClient& client, const String& url);

private:
    static void printProgress(size_t written, size_t total);
};

#endif // OTA_MANAGER_H
