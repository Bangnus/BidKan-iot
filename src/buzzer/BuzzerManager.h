#ifndef BUZZER_MANAGER_H
#define BUZZER_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "pins.h"

/// รูปแบบเสียงสำหรับลำโพงเสริม IoT (เฉพาะฟังก์ชันที่ตัวรถเดิมไม่มี)
enum BuzzerSoundType {
    SOUND_IDLE,
    SOUND_GEOFENCE_ALERT,   // เสียงเตือนออกนอกเขตให้บริการ (Geofence Breach)
    SOUND_LOCATE_BEACON,    // เสียงส่งสัญญาณหาตำแหน่งรถ (Find Bike / Remote Locate)
    SOUND_NO_PARK_ALERT,    // เสียงเตือนจอดผิดจุด / นอกโซนจอด
    SOUND_LOW_BATTERY_WARN  // เสียงเตือนแบตเตอรี่วิกฤต (<15%)
};

/// โครงสร้างโน้ตเสียงแต่ละสเต็ป
struct AudioToneStep {
    uint16_t freq;        // ความถี่ (Hz), 0 = หยุดเสียงชั่วคราว (silence)
    uint16_t durationMs;  // ระยะเวลาส่งเสียง (ms)
};

/**
 * BuzzerManager — จัดการลำโพงแจ้งเตือน IoT บนรถ BidKan
 * ทำงานแบบ Non-blocking 100% ผ่าน State Machine และ millis()
 * ให้บริการเฉพาะเสียงเสริมที่ตัวรถเดิมไม่มี (เช่น ออกนอกเขต, หาตำแหน่งรถ, ห้ามจอด)
 */
class BuzzerManager {
public:
    BuzzerManager();

    void init(uint8_t pin = BUZZER_PIN, uint8_t channel = BUZZER_PWM_CHANNEL);
    void update();

    /// ส่งเสียงเตือนออกนอกเขตให้บริการ (หวูดเตือนฉุกเฉินถี่ 2400Hz/1300Hz)
    void playGeofenceAlert();

    /// ส่งเสียงสัญญาณหาตำแหน่งรถ (Find Bike Beacon — เสียงกังวานนำทางระยะไกล)
    void playLocateBeacon();

    /// ส่งเสียงเตือนจุดห้ามจอด / นอกโซนที่กำหนด
    void playNoParkingAlert();

    /// ส่งเสียงเตือนแบตเตอรี่ต่ำวิกฤต
    void playLowBattery();

    /// สั่งหยุดเสียงทันที
    void stop();

    bool isPlaying() const;
    BuzzerSoundType getCurrentSound() const;

private:
    uint8_t         _pin;
    uint8_t         _channel;
    BuzzerSoundType _currentSound;
    uint8_t         _stepIndex;
    uint8_t         _repeatLeft;
    unsigned long   _stepStartTime;
    bool            _active;

    void _startTone(uint16_t freq);
    void _stopTone();
    void _setPattern(BuzzerSoundType type, uint8_t repeats = 0);
};

#endif // BUZZER_MANAGER_H
