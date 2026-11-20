// =============================================
// BidKan IoT — Hardware Unit Test: Buzzer / Speaker
// ทดสอบส่งเสียง: Unlock, Lock, Beep, Geofence Alert, Siren
// =============================================

#include <Arduino.h>
#include "buzzer/BuzzerManager.h"

BuzzerManager buzzer;
unsigned long lastActionTime = 0;
int testStep = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n--- [Test] Buzzer & Speaker Hardware Test Starting ---");
    Serial.println("Pin: GPIO 21 (BUZZER_PIN)");

    buzzer.init(BUZZER_PIN, BUZZER_PWM_CHANNEL);
    lastActionTime = millis();
}

void loop() {
    buzzer.update();

    if (millis() - lastActionTime > 3500) {
        lastActionTime = millis();
        testStep = (testStep + 1) % 6;

        switch (testStep) {
            case 0:
                Serial.println("\n[Test 1/6] >> Testing Unlock Chime...");
                buzzer.playUnlock();
                break;
            case 1:
                Serial.println("\n[Test 2/6] >> Testing Lock Chime...");
                buzzer.playLock();
                break;
            case 2:
                Serial.println("\n[Test 3/6] >> Testing Remote Locate Beep...");
                buzzer.playBeep();
                break;
            case 3:
                Serial.println("\n[Test 4/6] >> Testing Geofence Breach Warning...");
                buzzer.playGeofenceAlert();
                break;
            case 4:
                Serial.println("\n[Test 5/6] >> Testing Theft Alarm Siren...");
                buzzer.playTheftAlarm();
                break;
            case 5:
                Serial.println("\n[Test 6/6] >> Testing Low Battery Warning...");
                buzzer.playLowBattery();
                break;
        }
    }
}
