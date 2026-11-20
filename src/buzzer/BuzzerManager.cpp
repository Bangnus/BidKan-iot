#include "BuzzerManager.h"

// 1. เสียงเตือนออกนอกเขตให้บริการ (Geofence Breach): หวูดฉุกเฉินสลับความถี่ 2400Hz / 1300Hz
static const AudioToneStep GEOFENCE_STEPS[] = {
    {2400, 110}, {1300, 110}
};
static const uint8_t GEOFENCE_STEPS_COUNT = sizeof(GEOFENCE_STEPS) / sizeof(GEOFENCE_STEPS[0]);

// 2. เสียงส่งสัญญาณหาตำแหน่งรถ (Find Bike Beacon): 3 โน้ตขาขึ้นใสกังวาน ได้ยินชัดในลานจอด
static const AudioToneStep LOCATE_STEPS[] = {
    {2400, 90}, {0, 60}, {2700, 110}, {0, 60}, {3100, 220}
};
static const uint8_t LOCATE_STEPS_COUNT = sizeof(LOCATE_STEPS) / sizeof(LOCATE_STEPS[0]);

// 3. เสียงเตือนห้ามจอด / นอกโซนจอด (No-Parking Warning): โน้ตเตือน 2 จังหวะ
static const AudioToneStep NO_PARK_STEPS[] = {
    {1100, 180}, {0, 80}, {850, 250}
};
static const uint8_t NO_PARK_STEPS_COUNT = sizeof(NO_PARK_STEPS) / sizeof(NO_PARK_STEPS[0]);

// 4. เสียงเตือนแบตเตอรี่วิกฤต (Low Battery Warning): โน้ตทุ้มเตือน 2 บี๊บ
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
    update();
}

void BuzzerManager::playGeofenceAlert() {
    Serial.println("[Buzzer] ⚠️ ส่งเสียงเตือนออกนอกเขตให้บริการ (Geofence Alert)");
    _setPattern(SOUND_GEOFENCE_ALERT, 6); // หวูดเตือน 7 รอบ
}

void BuzzerManager::playLocateBeacon() {
    Serial.println("[Buzzer] 📍 ส่งเสียงสัญญาณหาตำแหน่งรถ (Locate Beacon)");
    _setPattern(SOUND_LOCATE_BEACON, 2); // บี๊บนำทาง 3 รอบ
}

void BuzzerManager::playNoParkingAlert() {
    Serial.println("[Buzzer] 🚫 ส่งเสียงเตือนพื้นที่ห้ามจอด (No-Parking Alert)");
    _setPattern(SOUND_NO_PARK_ALERT, 3);
}

void BuzzerManager::playLowBattery() {
    Serial.println("[Buzzer] 🪫 ส่งเสียงเตือนแบตเตอรี่วิกฤต (Low Battery)");
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
        case SOUND_GEOFENCE_ALERT:   steps = GEOFENCE_STEPS;  stepCount = GEOFENCE_STEPS_COUNT; break;
        case SOUND_LOCATE_BEACON:    steps = LOCATE_STEPS;    stepCount = LOCATE_STEPS_COUNT;   break;
        case SOUND_NO_PARK_ALERT:    steps = NO_PARK_STEPS;   stepCount = NO_PARK_STEPS_COUNT;  break;
        case SOUND_LOW_BATTERY_WARN: steps = LOW_BAT_STEPS;   stepCount = LOW_BAT_STEPS_COUNT;  break;
        default: stop(); return;
    }

    unsigned long elapsed = millis() - _stepStartTime;

    if (elapsed < steps[_stepIndex].durationMs) {
        return;
    }

    _stepIndex++;
    if (_stepIndex < stepCount) {
        _stepStartTime = millis();
        _startTone(steps[_stepIndex].freq);
    } else {
        if (_repeatLeft > 0) {
            _repeatLeft--;
            _stepIndex = 0;
            _stepStartTime = millis();
            _startTone(steps[0].freq);
        } else {
            stop();
        }
    }
}
