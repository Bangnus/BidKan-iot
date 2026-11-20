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
#include "buzzer/BuzzerManager.h"
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
BuzzerManager  buzzer;

// --- Timing ---
unsigned long lastReportTime = 0;

// --- MQTT Command Handler ---
void onCommand(const String& command) {
    String cmd = command;
    String reason = "";
    bool isAlarm = false;

    if (cmd.startsWith("{")) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, cmd);
        if (!error) {
            if (doc["command"].is<const char*>()) cmd = doc["command"].as<String>();
            else if (doc["status"].is<const char*>()) {
                String s = doc["status"].as<String>();
                cmd = (s == "LOCKED") ? "LOCK" : "UNLOCK";
            }
            if (doc["reason"].is<const char*>()) reason = doc["reason"].as<String>();
            if (doc["alarm"].is<bool>()) isAlarm = doc["alarm"].as<bool>();
        }
    }
    cmd.toUpperCase();

    if (cmd == "UNLOCK") {
        vehicle.unlock();
        // เสียงตอนปลดรถ ตัวรถมีเสียงเดิมจากโรงงานอยู่แล้ว ไม่ต้องส่งเสียงซ้อน
    } else if (cmd == "LOCK") {
        vehicle.lock();
        if (reason == "out_of_boundary") {
            buzzer.playGeofenceAlert(); // ลำโพงส่งเสียงเตือนออกนอกเขตทันที
        }
    } else if (cmd == "GEOFENCE_ALERT" || cmd == "OUT_OF_ZONE") {
        buzzer.playGeofenceAlert();
    } else if (cmd == "BEEP") {
        if (reason == "unauthorized_movement" || isAlarm) buzzer.playTheftAlarm();
        else buzzer.playBeep();
    } else if (cmd == "ALARM" || cmd == "THEFT") {
        buzzer.playTheftAlarm();
    } else if (cmd == "STOP_ALARM" || cmd == "MUTE") {
        buzzer.stop();
    } else if (cmd.startsWith("UPDATE ")) {
        String url = cmd.substring(7);
        url.trim();
        Serial.println("[Main] ได้รับคำสั่งอัปเดต Firmware จาก: " + url);
        OtaManager::startUpdate(modemMgr.getClient(), url);
    } else {
        Serial.println("[Main] ⚠️ คำสั่งไม่รู้จัก: " + cmd);
    }
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(10);

    Serial.println("\n=============================================");
    Serial.println("  BidKan IoT — Smart Bike Control System");
    Serial.println("=============================================");

    // 0. ตั้งค่า Watchdog Timer
    esp_task_wdt_init(WDT_TIMEOUT_SECONDS, true);
    esp_task_wdt_add(NULL);

    // 1. ตั้งค่า Vehicle Control & Buzzer / Speaker
    vehicle.init();
    buzzer.init();

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

    // 2. อัปเดต Vehicle & Buzzer state machine (non-blocking)
    vehicle.update();
    buzzer.update();

    // 3. ส่งข้อมูลสถานะตาม interval
    if (millis() - lastReportTime > GPS_REPORT_INTERVAL_MS) {
        lastReportTime = millis();

        JsonDocument doc;
        float lat = 0, lon = 0;
        if (modemMgr.getGPSData(lat, lon)) {
            doc["lat"]   = serialized(String(lat, 6));
            doc["lon"]   = serialized(String(lon, 6));
        } else {
            Serial.println("[Main] ⏳ รอสัญญาณ GPS...");
            doc["lat"]   = nullptr;
            doc["lon"]   = nullptr;
        }

        int batPct = battery.getPercent();
        doc["battery"] = batPct;

        if (batPct <= BATTERY_SHUTDOWN_PERCENT) {
            Serial.println("[Main] ⚠️ แบตเตอรี่ต่ำมาก! กำลังปิดระบบ...");
            buzzer.playLowBattery();
            doc["status"] = "LOW_BATTERY_SHUTDOWN";
            if (mqttSvc.isConnected()) {
                mqttSvc.publishStatus(doc);
                delay(1000);
            }
            modemMgr.powerOff();
            buzzer.stop();
            esp_deep_sleep_start();
        }

        doc["status"] = vehicle.getStateString();
        if (mqttSvc.isConnected()) {
            mqttSvc.publishStatus(doc);
        }
    }

    // 4. Feed the Watchdog
    esp_task_wdt_reset();
}
