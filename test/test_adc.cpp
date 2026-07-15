#include <Arduino.h>
#include "pins.h"
#include "config.h"

// =============================================
// Test: อ่านค่า ADC แบตเตอรี่
// Flash: pio run -e test_adc -t upload
// =============================================

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(100);

    analogReadResolution(12);
    pinMode(BATTERY_ADC_PIN, INPUT);

    Serial.println("\n--- เริ่มทดสอบ ADC แบตเตอรี่ ---");
    Serial.print("Pin: ");
    Serial.println(BATTERY_ADC_PIN);
    Serial.print("Range: ");
    Serial.print(BATTERY_ADC_MIN);
    Serial.print(" - ");
    Serial.println(BATTERY_ADC_MAX);
}

void loop() {
    int rawValue = analogRead(BATTERY_ADC_PIN);

    // แปลงค่าดิบด้วย calibrated range
    int batteryPercent = map(rawValue, BATTERY_ADC_MIN, BATTERY_ADC_MAX, 0, 100);

    // Clamp 0-100
    if (batteryPercent > 100) batteryPercent = 100;
    if (batteryPercent < 0)   batteryPercent = 0;

    Serial.print("ADC Raw: ");
    Serial.print(rawValue);
    Serial.print(" | แบตเตอรี่: ");
    Serial.print(batteryPercent);
    Serial.println(" %");

    delay(2000);
}