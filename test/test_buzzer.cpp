// =============================================
// BidKan IoT — Hardware Unit Test: Buzzer / Speaker
// ทดสอบเฉพาะเสียงเสริมที่ตัวรถเดิมไม่มี (ออกจากเขต, หาตำแหน่ง, ห้ามจอด, แบตต่ำ)
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

    if (millis() - lastActionTime > 4000) {
        lastActionTime = millis();
        testStep = (testStep + 1) % 4;

        switch (testStep) {
            case 0:
                Serial.println("\n[Test 1/4] >> Testing Geofence Breach Warning...");
                buzzer.playGeofenceAlert();
                break;
            case 1:
                Serial.println("\n[Test 2/4] >> Testing Remote Locate Beacon...");
                buzzer.playLocateBeacon();
                break;
            case 2:
                Serial.println("\n[Test 3/4] >> Testing No-Parking Warning...");
                buzzer.playNoParkingAlert();
                break;
            case 3:
                Serial.println("\n[Test 4/4] >> Testing Low Battery Warning...");
                buzzer.playLowBattery();
                break;
        }
    }
}
