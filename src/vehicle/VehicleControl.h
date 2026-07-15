#ifndef VEHICLE_CONTROL_H
#define VEHICLE_CONTROL_H

#include "config.h"
#include "pins.h"
#include <Arduino.h>

/// สถานะรถ
enum VehicleState {
    VEHICLE_LOCKED,         // ล็อกอยู่ — กุญแจ OFF, ล้อล็อก
    VEHICLE_UNLOCKING,      // กำลังปลดล็อก (transition)
    VEHICLE_UNLOCKED,       // ปลดล็อกแล้ว — กุญแจ ON, ล้อฟรี
    VEHICLE_LOCKING         // กำลังล็อก (transition)
};

/**
 * VehicleControl — ควบคุมการล็อก/ปลดล็อกรถ
 * 
 * ใช้ state machine + millis() แทน delay()
 * เพื่อไม่ให้ blocking MQTT loop
 * 
 * รองรับ 2 relay:
 * - RELAY_IGNITION_PIN (pin 12) — สวิตช์กุญแจ
 * - RELAY_LOCK_PIN (pin 14) — ล็อกล้อ
 */
class VehicleControl {
public:
    VehicleControl();

    /// ตั้งค่า pin + default state (locked)
    void init();

    /// สั่งปลดล็อก (non-blocking)
    void unlock();

    /// สั่งล็อก (non-blocking)
    void lock();

    /// ต้องเรียกทุก loop() เพื่อจัดการ state transitions
    void update();

    /// อ่านสถานะปัจจุบัน
    VehicleState getState() const;

    /// สถานะเป็น string สำหรับส่ง MQTT
    const char* getStateString() const;

private:
    VehicleState   _state;
    unsigned long  _transitionStart;
    static const unsigned long UNLOCK_DELAY_MS = 500;  // รอล้อคลายก่อนเปิดกุญแจ
    static const unsigned long LOCK_DELAY_MS   = 1000; // รอเครื่องดับก่อนล็อกล้อ
};

#endif // VEHICLE_CONTROL_H
