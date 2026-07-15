// =============================================
// BidKan IoT — Smart Bike Control System
// Board: LilyGO T-A7670SA (ESP32 + 4G + GPS)
// =============================================

#include "config.h"
#include "pins.h"
#include "modem/ModemManager.h"
#include "mqtt/MqttService.h"
#include "vehicle/VehicleControl.h"
#include "battery/BatteryMonitor.h"
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include "ota/OtaManager.h"
#include "ble/BleManager.h"

// --- Modules ---
ModemManager   modemMgr;
MqttService    mqttSvc;
BleManager     bleMgr;
VehicleControl vehicle;
BatteryMonitor battery;

// --- Timing ---
unsigned long lastReportTime = 0;

// --- MQTT Command Handler ---
void onCommand(const String& command) {
    if (command == "UNLOCK") {
        vehicle.unlock();
    } else if (command == "LOCK") {
        vehicle.lock();
    } else if (command.startsWith("UPDATE ")) {
        String url = command.substring(7); // ดึงข้อความหลัง "UPDATE "
        url.trim();
        Serial.println("[Main] ได้รับคำสั่งอัปเดต Firmware จาก: " + url);
        OtaManager::startUpdate(modemMgr.getClient(), url);
    } else {
        Serial.println("[Main] ⚠️ คำสั่งไม่รู้จัก: " + command);
    }
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(10);

    Serial.println("\n=============================================");
    Serial.println("  BidKan IoT — Smart Bike Control System");
    Serial.println("=============================================");

    // 0. ตั้งค่า Watchdog Timer
    esp_task_wdt_init(WDT_TIMEOUT_SECONDS, true); // true = Panic/Reboot on timeout
    esp_task_wdt_add(NULL); // Add current loop() task to WDT

    // 1. ตั้งค่า Vehicle Control (relays)
    vehicle.init();

    // 2. ตั้งค่า Battery Monitor
    battery.init(BATTERY_ADC_PIN, BATTERY_ADC_MIN, BATTERY_ADC_MAX);

    // 3. เริ่มโมดูล 4G + GPS
    if (!modemMgr.begin()) {
        Serial.println("[Main] ❌ เริ่ม 4G ไม่สำเร็จ — รีบูต...");
        ESP.restart();
    }

    // 4. ตั้งค่า MQTT
    String bikeId = modemMgr.getIMEI();
    mqttSvc.begin(modemMgr.getClient(), onCommand);

    if (!mqttSvc.connect(bikeId)) {
        Serial.println("[Main] ⚠️ MQTT เชื่อมต่อไม่ได้ — จะ retry ใน loop");
    }

    // 5. เริ่มต้นระบบ Bluetooth สำรอง
    bleMgr.begin(bikeId, onCommand);

    Serial.println("[Main] ✅ ระบบพร้อมทำงาน!");
}

void loop() {
    // 1. ตรวจสอบ MQTT connection
    mqttSvc.reconnectIfNeeded();
    mqttSvc.loop();

    // 2. อัปเดต Vehicle state machine (non-blocking)
    vehicle.update();

    // 3. ส่งข้อมูลสถานะตาม interval
    if (millis() - lastReportTime > GPS_REPORT_INTERVAL_MS) {
        lastReportTime = millis();

        // สร้าง JSON payload ด้วย ArduinoJson
        JsonDocument doc;

        // GPS data
        float lat = 0, lon = 0, speed = 0;
        if (modemMgr.getGPSData(lat, lon, speed)) {
            doc["lat"]   = serialized(String(lat, 6));
            doc["lon"]   = serialized(String(lon, 6));
            doc["speed"] = serialized(String(speed, 1));
        } else {
            Serial.println("[Main] ⏳ รอสัญญาณ GPS...");
            doc["lat"]   = nullptr;
            doc["lon"]   = nullptr;
            doc["speed"] = nullptr;
        }

        // Battery
        int batPct = battery.getPercent();
        doc["battery"] = batPct;

        // Check Low Voltage Cutoff
        if (batPct <= BATTERY_SHUTDOWN_PERCENT) {
            Serial.println("[Main] ⚠️ แบตเตอรี่ต่ำมาก! กำลังปิดระบบเพื่อป้องกันแบตเตอรี่เสื่อม...");
            doc["status"] = "LOW_BATTERY_SHUTDOWN";
            if (mqttSvc.isConnected()) {
                mqttSvc.publishStatus(doc);
                delay(1000); // รอให้ส่งข้อความเสร็จ
            }
            modemMgr.powerOff();
            Serial.println("[Main] 💤 เข้าสู่ Deep Sleep ถาวร (ต้องกด Reset เพื่อปลุก)...");
            esp_deep_sleep_start();
        }

        // Vehicle status
        doc["status"] = vehicle.getStateString();

        // Publish
        if (mqttSvc.isConnected()) {
            mqttSvc.publishStatus(doc);
        }
    }

    // 4. Feed the Watchdog
    esp_task_wdt_reset();
}