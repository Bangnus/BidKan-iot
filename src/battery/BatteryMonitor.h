#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <Arduino.h>

/**
 * BatteryMonitor — อ่านค่าแบตเตอรี่ผ่าน ADC
 * 
 * ใช้ค่า calibrated จากวงจร voltage divider จริง
 * รองรับ smoothing ด้วยค่าเฉลี่ยหลายรอบ
 */
class BatteryMonitor {
public:
    BatteryMonitor();

    /// ตั้งค่าเริ่มต้น
    /// @param pin — ขา ADC (เช่น 32)
    /// @param adcMin — ค่า ADC ต่ำสุด (แบตหมด)
    /// @param adcMax — ค่า ADC สูงสุด (แบตเต็ม)
    void init(int pin, int adcMin, int adcMax);

    /// อ่านค่า ADC ดิบ
    int getRawValue();

    /// อ่านเปอร์เซ็นต์แบตเตอรี่ (0-100, clamped)
    int getPercent();

private:
    int _pin;
    int _adcMin;
    int _adcMax;
};

#endif // BATTERY_MONITOR_H
