#ifndef MQTT_SERVICE_H
#define MQTT_SERVICE_H

#include <Arduino.h>
#include "config.h"
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

/// Callback type สำหรับรับคำสั่งจาก MQTT
typedef void (*CommandCallback)(const String& command);

/**
 * MqttService — จัดการการเชื่อมต่อ MQTT
 * 
 * รับผิดชอบ:
 * - Connect/reconnect กับ MQTT broker
 * - Subscribe topic คำสั่ง
 * - Publish JSON payload
 * - Route callback ไปยังฟังก์ชัน command handler
 */
class MqttService {
public:
    MqttService();

    /// ตั้งค่าเริ่มต้น — ต้องเรียกก่อนใช้งาน
    /// @param gsmClient — TinyGsmClient จาก ModemManager
    /// @param onCommand — callback เมื่อได้รับคำสั่ง
    void begin(TinyGsmClient& gsmClient, CommandCallback onCommand);

    /// เชื่อมต่อ MQTT broker + subscribe command topic
    /// @param bikeId — ใช้เป็น client ID และสร้าง topic
    /// @return true ถ้าเชื่อมต่อสำเร็จ
    bool connect(const String& bikeId);

    /// Publish JSON document ไปยัง status topic
    /// @param doc — ArduinoJson document
    /// @param retain — ตั้งเป็น true หากต้องการให้ broker จำค่าล่าสุด
    /// @return true ถ้าส่งสำเร็จ
    bool publishStatus(const JsonDocument& doc, bool retain = false);

    /// ต้องเรียกทุก loop() เพื่อรับข้อมูล
    void loop();

    /// ตรวจสอบสถานะการเชื่อมต่อ
    bool isConnected();

    /// Reconnect ถ้าหลุด (non-blocking, ลอง 1 ครั้ง)
    /// @return true ถ้าเชื่อมต่อสำเร็จ
    bool reconnectIfNeeded();

private:
    PubSubClient    _mqtt;
    String          _bikeId;
    String          _topicPub;   // bidkan/<IMEI>/status
    String          _topicSub;   // bidkan/<IMEI>/command
    CommandCallback _onCommand;
    unsigned long   _lastReconnectAttempt;

    // คำเตือน: คลาสนี้รองรับเพียง 1 instance (Singleton pattern) เท่านั้น
    // เนื่องจาก PubSubClient ต้องการ static callback C-style function pointer
    static MqttService* _instance;
    static void _mqttCallbackStatic(char* topic, byte* payload, unsigned int length);
    void _handleMessage(char* topic, byte* payload, unsigned int length);
};

#endif // MQTT_SERVICE_H
