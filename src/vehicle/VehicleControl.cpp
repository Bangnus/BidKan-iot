#include "VehicleControl.h"

VehicleControl::VehicleControl()
    : _state(VEHICLE_LOCKED)
    , _transitionStart(0)
{
}

void VehicleControl::init() {
    // กำหนดระดับสัญญาณก่อนตั้ง pinMode(OUTPUT) เพื่อป้องกันรีเลย์กระตุกตอนบูต
    // สถานะเริ่มต้น: รถล็อก (กุญแจ OFF, ล้อล็อก ON)
    digitalWrite(RELAY_IGNITION_PIN, RELAY_OFF);
    digitalWrite(RELAY_LOCK_PIN, RELAY_ON);

    pinMode(RELAY_IGNITION_PIN, OUTPUT);
    pinMode(RELAY_LOCK_PIN, OUTPUT);

    _state = VEHICLE_LOCKED;
    Serial.println("[Vehicle] 🔒 เริ่มต้น: รถล็อก");
}

void VehicleControl::unlock() {
    if (_state == VEHICLE_UNLOCKED || _state == VEHICLE_UNLOCKING) {
        Serial.println("[Vehicle] ⚠️ รถปลดล็อกอยู่แล้ว");
        return;
    }

    Serial.println("[Vehicle] 🔓 เริ่มปลดล็อก...");

    // Step 1: คลายล็อกล้อก่อน
    digitalWrite(RELAY_LOCK_PIN, RELAY_OFF);

    _state = VEHICLE_UNLOCKING;
    _transitionStart = millis();
}

void VehicleControl::lock() {
    if (_state == VEHICLE_LOCKED || _state == VEHICLE_LOCKING) {
        Serial.println("[Vehicle] ⚠️ รถล็อกอยู่แล้ว");
        return;
    }

    Serial.println("[Vehicle] 🔒 เริ่มล็อก...");

    // Step 1: ดับเครื่องก่อน
    digitalWrite(RELAY_IGNITION_PIN, RELAY_OFF);

    _state = VEHICLE_LOCKING;
    _transitionStart = millis();
}

void VehicleControl::update() {
    unsigned long elapsed = millis() - _transitionStart;

    switch (_state) {
        case VEHICLE_UNLOCKING:
            // รอล้อคลายตัว แล้วเปิดกุญแจ
            if (elapsed >= UNLOCK_DELAY_MS) {
                digitalWrite(RELAY_IGNITION_PIN, RELAY_ON);
                _state = VEHICLE_UNLOCKED;
                Serial.println("[Vehicle] ✅ รถพร้อมขับ");
            }
            break;

        case VEHICLE_LOCKING:
            // รอเครื่องดับสนิท แล้วล็อกล้อ
            if (elapsed >= LOCK_DELAY_MS) {
                digitalWrite(RELAY_LOCK_PIN, RELAY_ON);
                _state = VEHICLE_LOCKED;
                Serial.println("[Vehicle] ✅ รถล็อกสมบูรณ์");
            }
            break;

        default:
            // LOCKED หรือ UNLOCKED — ไม่ต้องทำอะไร
            break;
    }
}

VehicleState VehicleControl::getState() const {
    return _state;
}

const char* VehicleControl::getStateString() const {
    switch (_state) {
        case VEHICLE_LOCKED:    return "LOCKED";
        case VEHICLE_UNLOCKING: return "UNLOCKING";
        case VEHICLE_UNLOCKED:  return "ACTIVE";
        case VEHICLE_LOCKING:   return "LOCKING";
        default:                return "UNKNOWN";
    }
}
