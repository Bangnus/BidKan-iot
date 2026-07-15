#include "BleManager.h"

BleManager::BleManager()
    : _onCommand(nullptr)
    , _deviceConnected(false)
{
}

void BleManager::begin(const String& bikeId, CommandCallback onCommand) {
    _onCommand = onCommand;
    
    // ตั้งชื่ออุปกรณ์ BLE เป็น BidKan-<IMEI>
    String deviceName = "BidKan-" + bikeId;
    NimBLEDevice::init(deviceName.c_str());

    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks(this));

    NimBLEService* pService = pServer->createService(BLE_SERVICE_UUID);

    NimBLECharacteristic* pCharacteristic = pService->createCharacteristic(
                                         BLE_CHARACTERISTIC_UUID,
                                         NIMBLE_PROPERTY::WRITE | 
                                         NIMBLE_PROPERTY::WRITE_NR
                                       );

    pCharacteristic->setCallbacks(new CharacteristicCallbacks(this));

    pService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    Serial.println("[BLE] ✅ ระบบ Bluetooth พร้อมรับคำสั่ง — ชื่อ: " + deviceName);
}

bool BleManager::isConnected() {
    return _deviceConnected;
}

void BleManager::handleCommand(const String& command) {
    Serial.println("[BLE] 📩 ได้รับคำสั่งผ่านบลูทูธ: " + command);
    if (_onCommand) {
        _onCommand(command);
    }
}

void BleManager::setConnected(bool connected) {
    _deviceConnected = connected;
}

// --- Server Callbacks ---
void BleManager::ServerCallbacks::onConnect(NimBLEServer* pServer) {
    Serial.println("[BLE] 📱 มีอุปกรณ์เชื่อมต่อเข้ามาแล้ว");
    _manager->setConnected(true);
}

void BleManager::ServerCallbacks::onDisconnect(NimBLEServer* pServer) {
    Serial.println("[BLE] 📱 อุปกรณ์ตัดการเชื่อมต่อ — เปิดโฆษณา (Advertise) ใหม่...");
    _manager->setConnected(false);
    NimBLEDevice::startAdvertising(); // กลับมารอรับ connection ใหม่
}

// --- Characteristic Callbacks ---
void BleManager::CharacteristicCallbacks::onWrite(NimBLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
        String cmd = String(value.c_str());
        cmd.trim();
        _manager->handleCommand(cmd);
    }
}
