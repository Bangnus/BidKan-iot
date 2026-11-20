#ifndef BIDKAN_PINS_H
#define BIDKAN_PINS_H

// =============================================
// BidKan IoT — Pin Definitions
// Board: LilyGO T-A7670SA (ESP32)
// =============================================

// --- 4G Modem (SIMCOM A7670SA) ---
// ยืนยันจากบอร์ดจริงแล้ว: ESP32 RX=27, TX=26
#define MODEM_RX_PIN     27
#define MODEM_TX_PIN     26
#define MODEM_PWRKEY_PIN  4
#define MODEM_RESET_PIN   5   // active HIGH — ห้ามปล่อยลอย
#define MODEM_DTR_PIN    25   // ต้อง LOW ไม่งั้นโมเด็มหลับ

// --- Power Control ---
// เปิด DC boost จ่ายไฟให้โมเด็ม (ห้ามต่ออุปกรณ์อื่นที่ขานี้)
#define BOARD_POWERON_PIN 12

// --- Relay Outputs ---
// ย้ายจากขา 12/14 มาที่ 18/19 เพื่อหลีกเลี่ยง Strapping Pin และ SD Card Clock
// Relay 1: ควบคุมสวิตช์กุญแจ (จ่ายไฟให้รถ)
#define RELAY_IGNITION_PIN  18

// Relay 2: ควบคุมล็อกล้อ (ต่อตรง Controller)
#define RELAY_LOCK_PIN      19

// --- Analog Input ---
// วัดแรงดันแบตเตอรี่ผ่าน voltage divider (20K + 1K)
#define BATTERY_ADC_PIN     32

#endif // BIDKAN_PINS_H
