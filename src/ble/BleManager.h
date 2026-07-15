#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <NimBLEDevice.h>

// Service and Characteristic UUIDs for BidKan Bike
#define BLE_SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define BLE_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

typedef void (*CommandCallback)(const String& command);

/**
 * BleManager — จัดการการเชื่อมต่อและรับคำสั่งผ่าน Bluetooth (BLE)
 * 
 * ใช้เป็นระบบสำรอง (Backup) ในกรณีที่รถจอดอยู่ในที่อับสัญญาณ 4G
 */
class BleManager {
public:
    BleManager();

    /// เริ่มต้นระบบ BLE และตั้งชื่อตาม Bike ID
    void begin(const String& bikeId, CommandCallback onCommand);

    /// ตรวจสอบว่ามีอุปกรณ์เชื่อมต่ออยู่หรือไม่
    bool isConnected();

private:
    CommandCallback _onCommand;
    bool _deviceConnected;

    class ServerCallbacks : public NimBLEServerCallbacks {
        BleManager* _manager;
    public:
        ServerCallbacks(BleManager* manager) : _manager(manager) {}
        void onConnect(NimBLEServer* pServer) override;
        void onDisconnect(NimBLEServer* pServer) override;
    };

    class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
        BleManager* _manager;
    public:
        CharacteristicCallbacks(BleManager* manager) : _manager(manager) {}
        void onWrite(NimBLECharacteristic* pCharacteristic) override;
    };

    friend class ServerCallbacks;
    friend class CharacteristicCallbacks;

    void handleCommand(const String& command);
    void setConnected(bool connected);
};

#endif // BLE_MANAGER_H
