#ifndef BIDKAN_CONFIG_H
#define BIDKAN_CONFIG_H

// =============================================
// BidKan IoT — Central Configuration
// =============================================

// --- Modem Type (ต้อง define ก่อน include TinyGSM) ---
// A7670SA ใช้ define A7672X ใน TinyGSM v0.12.0
#ifndef TINY_GSM_MODEM_A7672X
#define TINY_GSM_MODEM_A7672X
#endif
#ifndef TINY_GSM_RX_BUFFER
#define TINY_GSM_RX_BUFFER 1024
#endif

// --- APN Settings ---
#define APN_NAME        "internet"
#define APN_USER        ""
#define APN_PASS        ""

// --- MQTT Server ---
#define MQTT_SERVER     "broker.hivemq.com" // ใช้ Public Broker ฟรีสำหรับเทสก่อน (เปลี่ยนเป็น IP ของคุณตอนลง Production)
#define MQTT_PORT       1883
#define MQTT_USER       ""                  // ใส่ถ้าเซิร์ฟเวอร์ต้องการ auth
#define MQTT_PASS       ""

// --- MQTT Topics (prefix — จะต่อด้วย IMEI อัตโนมัติ) ---
#define MQTT_TOPIC_PREFIX   "bidkan"

// --- Timing Intervals (ms & sec) ---
#define GPS_REPORT_INTERVAL_MS   15000   // ส่งพิกัดทุก 15 วินาที
#define MQTT_RECONNECT_DELAY_MS  5000    // รอ 5 วิก่อน retry MQTT
#define GPRS_RETRY_DELAY_MS      5000    // รอ 5 วิก่อน retry GPRS
#define GPRS_CONNECT_MAX_RETRIES 5       // ลอง connect GPRS สูงสุด 5 ครั้ง
#define MODEM_BOOT_DELAY_MS      5000    // รอโมดูลบูต
#define WDT_TIMEOUT_SECONDS      30      // Watchdog timeout 30 วินาที

// --- Battery ADC Calibration ---
// ค่าจาก voltage divider จริง (20K + 1K)
// ADC 2304 ≈ แบตหมด, ADC 3226 ≈ แบตเต็ม
#define BATTERY_ADC_MIN  2304
#define BATTERY_ADC_MAX  3226
#define BATTERY_SHUTDOWN_PERCENT 15 // ตัดการทำงาน 4G ถาวรเมื่อแบตเหลือต่ำกว่า 15%

// --- Serial Baud Rate ---
#define SERIAL_BAUD      115200
#define MODEM_BAUD       115200

#endif // BIDKAN_CONFIG_H
