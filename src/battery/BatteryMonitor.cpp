#include "BatteryMonitor.h"

BatteryMonitor::BatteryMonitor()
    : _pin(0)
    , _adcMin(0)
    , _adcMax(4095)
{
}

void BatteryMonitor::init(int pin, int adcMin, int adcMax) {
    _pin    = pin;
    _adcMin = adcMin;
    _adcMax = adcMax;

    analogReadResolution(12);

    Serial.print("[Battery] ตั้งค่า ADC pin ");
    Serial.print(pin);
    Serial.print(" range: ");
    Serial.print(adcMin);
    Serial.print("-");
    Serial.println(adcMax);
}

int BatteryMonitor::getRawValue() {
    return analogRead(_pin);
}

int BatteryMonitor::getPercent() {
    int raw = analogRead(_pin);
    int percent = map(raw, _adcMin, _adcMax, 0, 100);

    // Clamp 0-100
    if (percent > 100) percent = 100;
    if (percent < 0)   percent = 0;

    return percent;
}
