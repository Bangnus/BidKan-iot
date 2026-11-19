#ifndef BIDKAN_PINS_H
#define BIDKAN_PINS_H

// =============================================
// BidKan IoT — Pin Definitions
// Board: LilyGO T-A7670SA (ESP32)
// =============================================

// --- 4G Modem (SIMCOM A7670SA) ---
#define MODEM_RX_PIN     26
#define MODEM_TX_PIN     27
#define MODEM_PWRKEY_PIN  4

// --- Power Control ---
// ถ้าไม่มี BOARD_POWERON_PIN ก็ไม่ต้องใช้ (สำหรับ T-A7670)

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
