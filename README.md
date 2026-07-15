BidKan-iot/
├── include/
│   ├── config.h              ← ค่าตั้ง APN, MQTT, timing, battery cal
│   └── pins.h                ← Pin definitions สำหรับ T-A7670SA
│
├── src/
│   ├── main.cpp              ← ~90 บรรทัด — orchestrator เรียกใช้ modules
│   ├── modem/
│   │   ├── ModemManager.h
│   │   └── ModemManager.cpp  ← 4G + GPS (AT commands)
│   ├── mqtt/
│   │   ├── MqttService.h
│   │   └── MqttService.cpp   ← MQTT + ArduinoJson
│   ├── vehicle/
│   │   ├── VehicleControl.h
│   │   └── VehicleControl.cpp ← Non-blocking lock/unlock state machine
│   └── battery/
│       ├── BatteryMonitor.h
│       └── BatteryMonitor.cpp ← Calibrated ADC reading
│
├── test/                      ← Hardware tests (flash ทีละ env)
│   ├── test_relay.cpp
│   ├── test_gps.cpp
│   ├── test_adc.cpp           ← (แก้ไข: เพิ่ม setup/loop)
│   └── test_wifi.cpp
│
└── platformio.ini             ← Per-env configs


pio run -e production -t upload

pio device monitor -e production -b 115200
