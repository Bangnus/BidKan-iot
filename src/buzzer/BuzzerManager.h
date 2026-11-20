#ifndef BUZZER_MANAGER_H
#define BUZZER_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "pins.h"

/// รูปแบบเสียงเอฟเฟกต์สำหรับลำโพง/บัซเซอร์
enum BuzzerSoundType {
    SOUND_IDLE,
    SOUND_UNLOCK_CHIME,     // ปลดล็อกสำเร็จ: โน้ตขาขึ้น 3 จังหวะ
    SOUND_LOCK_CHIME,       // ล็อกสำเร็จ: โน้ตขาลง 2 จังหวะ
    SOUND_BEEP_LOCATE,      // บี๊บหาตำแหน่ง: 2 จังหวะสั้น
    SOUND_GEOFENCE_ALERT,   // เตือนออกนอกเขตให้บริการ (ฉุกเฉินสลับความถี่)
    SOUND_THEFT_SIREN,      // ไซเรนกันขโมย: ความถี่สูงสลับไปมา
    SOUND_LOW_BATTERY_WARN  // เตือนแบตเตอรี่วิกฤต
};

/// โครงสร้างโน้ตเสียงแต่ละสเต็ป
struct AudioToneStep {
    uint16_t freq;        // ความถี่ (Hz), 0 = หยุดเสียงชั่วคราว (silence)
    uint16_t durationMs;  // ระยะเวลาส่งเสียง (ms)
};

/**
 * BuzzerManager — ควบคุมลำโพงและบัซเซอร์ส่งเสียงเตือนบนรถ BidKan
 * ออกแบบด้วย State Machine แบบ Non-blocking 100% ไม่กระทบ MQTT loop
 */
class BuzzerManager {
public:
    BuzzerManager();

    /// เริ่มต้นพินลำโพงและตั้งค่า PWM LEDC
    void init(uint8_t pin = BUZZER_PIN, uint8_t channel = BUZZER_PWM_CHANNEL);

    /// ต้องเรียกทุก loop() เพื่อขับสเต็ปเสียงตามเวลา millis()
    void update();

    /// เล่นเสียงปลดล็อกรถ (Chime ขาขึ้น)
    void playUnlock();

    /// เล่นเสียงล็อกรถ (Chime ขาลง)
    void playLock();

    /// เล่นเสียงบี๊บส่งสัญญาณหาตำแหน่งรถ (Remote Beep)
    void playBeep();

    /// เล่นเสียงเตือนออกนอกเขต Riding Limit Zone
    void playGeofenceAlert();

    /// เล่นเสียงไซเรนกันขโมยเมื่อตรวจจับการเคลื่อนย้ายผิดปกติ
    void playTheftAlarm();

    /// เล่นเสียงเตือนแบตเตอรี่ต่ำ
    void playLowBattery();

    /// หยุดเสียงทันที
    void stop();

    /// ตรวจสอบว่ากำลังส่งเสียงอยู่หรือไม่
    bool isPlaying() const;

    /// รูปแบบเสียงปัจจุบัน
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
