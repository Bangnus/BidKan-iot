#ifndef MODEM_MANAGER_H
#define MODEM_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "pins.h"
#include <TinyGsmClient.h>

/**
 * ModemManager — จัดการโมดูล 4G (SIMCOM A7670SA)
 * 
 * รับผิดชอบ:
 * - Power on/off โมดูล
 * - เชื่อมต่อ GPRS (internet)
 * - เปิด/อ่าน GPS ผ่าน AT command (TinyGSM A7672X ไม่มี GPS built-in)
 * - ดึง IMEI สำหรับ Bike ID
 */
class ModemManager {
public:
    ModemManager();

    /// เริ่มต้นโมดูล: power on + init AT + connect GPRS + enable GPS
    bool begin();

    /// ปิดโมดูลเพื่อประหยัดพลังงาน
    void powerOff();

    /// เชื่อมต่อ GPRS (internet) พร้อม retry
    bool connectGPRS();

    /// เปิด GPS module ผ่าน AT+CGNSSPWR=1
    bool enableGPS();

    /// ดึงพิกัด GPS ผ่าน AT+CGNSSINFO
    /// @return true ถ้ามี GPS fix
    bool getGPSData(float &lat, float &lon);

    /// ดึงเลข IMEI ของโมดูล
    String getIMEI();

    /// เข้าถึง TinyGsmClient สำหรับ MQTT
    TinyGsmClient& getClient();

    /// เข้าถึง modem สำหรับ AT commands
    TinyGsm& getModem();

private:
    HardwareSerial _serialAT;
    TinyGsm        _modem;
    TinyGsmClient   _client;

    void powerOn();
    bool isModemReady();
    bool parseGNSSInfo(const String& response, float &lat, float &lon);
};

#endif // MODEM_MANAGER_H
