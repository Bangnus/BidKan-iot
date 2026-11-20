#include "BuzzerManager.h"

// กำหนดท่วงทำนองโน้ตเสียงแต่ละสเต็ป {ความถี่ Hz, ระยะเวลา ms}
static const AudioToneStep UNLOCK_STEPS[] = {
    {523, 100}, {659, 100}, {784, 250}
};
static const uint8_t UNLOCK_STEPS_COUNT = sizeof(UNLOCK_STEPS) / sizeof(UNLOCK_STEPS[0]);

static const AudioToneStep LOCK_STEPS[] = {
    {784, 120}, {523, 280}
};
static const uint8_t LOCK_STEPS_COUNT = sizeof(LOCK_STEPS) / sizeof(LOCK_STEPS[0]);

static const AudioToneStep BEEP_STEPS[] = {
    {2200, 120}, {0, 70}, {2200, 180}
};
static const uint8_t BEEP_STEPS_COUNT = sizeof(BEEP_STEPS) / sizeof(BEEP_STEPS[0]);

static const AudioToneStep GEOFENCE_STEPS[] = {
    {2400, 110}, {1300, 110}
};
static const uint8_t GEOFENCE_STEPS_COUNT = sizeof(GEOFENCE_STEPS) / sizeof(GEOFENCE_STEPS[0]);

static const AudioToneStep THEFT_STEPS[] = {
    {2800, 140}, {1800, 140}
};
static const uint8_t THEFT_STEPS_COUNT = sizeof(THEFT_STEPS) / sizeof(THEFT_STEPS[0]);

static const AudioToneStep LOW_BAT_STEPS[] = {
    {440, 150}, {0, 100}, {440, 150}
};
static const uint8_t LOW_BAT_STEPS_COUNT = sizeof(LOW_BAT_STEPS) / sizeof(LOW_BAT_STEPS[0]);

BuzzerManager::BuzzerManager()
    : _pin(BUZZER_PIN)
    , _channel(BUZZER_PWM_CHANNEL)
    , _currentSound(SOUND_IDLE)
    , _stepIndex(0)
    , _repeatLeft(0)
    , _stepStartTime(0)
    , _active(false)
{
}

void BuzzerManager::init(uint8_t pin, uint8_t channel) {
    _pin = pin;
    _channel = channel;
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);

    // ติดตั้งช่องสัญญาณ PWM ความถี่เริ่มต้น 2000Hz ความละเอียด 8-bit
    ledcSetup(_channel, 2000, BUZZER_PWM_RESOLUTION);
    ledcAttachPin(_pin, _channel);
    _stopTone();
    Serial.println("[Buzzer] 🔊 พร้อมใช้งานบน GPIO " + String(_pin));
}

void BuzzerManager::_startTone(uint16_t freq) {
    if (freq > 0) {
        ledcWriteTone(_channel, freq);
    } else {
        _stopTone();
    }
}

void BuzzerManager::_stopTone() {
    ledcWriteTone(_channel, 0);
    digitalWrite(_pin, LOW);
}

void BuzzerManager::_setPattern(BuzzerSoundType type, uint8_t repeats) {
    _currentSound = type;
    _stepIndex = 0;
    _repeatLeft = repeats;
    _stepStartTime = millis();
    _active = true;

    // เริ่มเล่นโน้ตแรกทันที
    update();
}

void BuzzerManager::playUnlock() {
    Serial.println("[Buzzer] 🎵 ปลดล็อกสำเร็จ (Unlock Chime)");
    _setPattern(SOUND_UNLOCK_CHIME, 0);
}

void BuzzerManager::playLock() {
    Serial.println("[Buzzer] 🔒 ล็อกสำเร็จ (Lock Chime)");
    _setPattern(SOUND_LOCK_CHIME, 0);
}

void BuzzerManager::playBeep() {
    Serial.println("[Buzzer] 🔔 บี๊บส่งสัญญาณหาตำแหน่งรถ (Remote Beep)");
    _setPattern(SOUND_BEEP_LOCATE, 0);
}

void BuzzerManager::playGeofenceAlert() {
    Serial.println("[Buzzer] ⚠️ แจ้งเตือนออกนอกเขตให้บริการ (Geofence Breach Alarm)");
    _setPattern(SOUND_GEOFENCE_ALERT, 5); // ส่งเสียงหวูดเตือน 6 รอบ
}

void BuzzerManager::playTheftAlarm() {
    Serial.println("[Buzzer] 🚨 สัญญาณไซเรนกันขโมย (Theft Alarm Siren)");
    _setPattern(SOUND_THEFT_SIREN, 8); // ไซเรน 9 รอบ
}

void BuzzerManager::playLowBattery() {
    Serial.println("[Buzzer] 🪫 แจ้งเตือนแบตเตอรี่วิกฤต (Low Battery)");
    _setPattern(SOUND_LOW_BATTERY_WARN, 1);
}

void BuzzerManager::stop() {
    _stopTone();
    _currentSound = SOUND_IDLE;
    _active = false;
    _repeatLeft = 0;
}

bool BuzzerManager::isPlaying() const {
    return _active;
}

BuzzerSoundType BuzzerManager::getCurrentSound() const {
    return _currentSound;
}

void BuzzerManager::update() {
    if (!_active || _currentSound == SOUND_IDLE) {
        return;
    }

    const AudioToneStep* steps = nullptr;
    uint8_t stepCount = 0;

    switch (_currentSound) {
        case SOUND_UNLOCK_CHIME:    steps = UNLOCK_STEPS;    stepCount = UNLOCK_STEPS_COUNT; break;
        case SOUND_LOCK_CHIME:      steps = LOCK_STEPS;      stepCount = LOCK_STEPS_COUNT;   break;
        case SOUND_BEEP_LOCATE:     steps = BEEP_STEPS;      stepCount = BEEP_STEPS_COUNT;   break;
        case SOUND_GEOFENCE_ALERT:  steps = GEOFENCE_STEPS;  stepCount = GEOFENCE_STEPS_COUNT; break;
        case SOUND_THEFT_SIREN:     steps = THEFT_STEPS;     stepCount = THEFT_STEPS_COUNT;    break;
        case SOUND_LOW_BATTERY_WARN:steps = LOW_BAT_STEPS;   stepCount = LOW_BAT_STEPS_COUNT;  break;
        default: stop(); return;
    }

    unsigned long elapsed = millis() - _stepStartTime;

    // ยังอยู่ในสเต็ปปัจจุบัน
    if (elapsed < steps[_stepIndex].durationMs) {
        return;
    }

    // ก้าวไปยังสเต็ปถัดไป
    _stepIndex++;
    if (_stepIndex < stepCount) {
        _stepStartTime = millis();
        _startTone(steps[_stepIndex].freq);
    } else {
        // ครบรอบรูปแบบโน้ต
        if (_repeatLeft > 0) {
            _repeatLeft--;
            _stepIndex = 0;
            _stepStartTime = millis();
            _startTone(steps[0].freq);
        } else {
            // จบเสียงทั้งหมด
            stop();
        }
    }
}
