
#ifndef AS7331_H
#define AS7331_H

// --- Definice konstant pro AS7331 ---
// Adresa senzoru (7-bit) – v HAL se používá 8-bit adresa (shift vlevo)
#define AS7331_ADDRESS (0x74 << 1)

// Registrační mapy (pozn. v konfiguraci vs. výstupní bankě)
// Registrační banka konfigurace:
#define OSR_REG 0x00 // Operational State Register (obsahuje i bity pro soft reset, režim, …)
#define CREG1   0x06 // Konfigurační registr 1 (integrace a zisk)
#define CREG2   0x07 // Konfigurační registr 2 (divider, měření času apod.)
#define CREG3   0x08 // Konfigurační registr 3 (režim měření, standby, interní hodiny)

// Výstupní registrační banka (při měření – automaticky aktivní):
#define OUT_REG_STATUS 0x00 // Status (obsahuje bit NOTREADY)
#define OUT_REG_TEMP   0x01 // Teplota (raw, 16 bitů)
#define OUT_REG_MRES1  0x02 // Měření UVA (raw, 16 bitů)
#define OUT_REG_MRES2  0x03 // Měření UVB (raw, 16 bitů)
#define OUT_REG_MRES3  0x04 // Měření UVC (raw, 16 bitů)

// Masky pro OSR registr
#define OSR_MASK_DOS    0x07 // Mask pro stav zařízení (bits 0..2)
#define OSR_MASK_SW_RES 0x08 // Software reset
#define OSR_MASK_PD     0x40 // Power Down (není v této ukázce použit)
#define OSR_MASK_SS     0x80 // Start/Stop měření (spouští jednorázové měření)

// Stavy zařízení (uložené v OSR – bity DOS)
#define DEVICE_STATE_CONFIGURATION 0x02
#define DEVICE_STATE_MEASUREMENT   0x03

// Maska status registru (výstupní banka)
#define STATUS_MASK_NOTREADY 0x04 // Pokud je nastaven, data ještě nejsou připravena

// Výchozí konfigurace – inspirováno funkcí set_default_config v Pythonu
// V CREG1 je nižší nibble integ. čas, vyšší nibble zisk.
// Volíme INTEGRATION_TIME_256MS = 0x08 a GAIN_16X = 0x07, tedy:
#define CREG1_DEFAULT 0x78 // (0x07 << 4) | 0x08
#define CREG2_DEFAULT 0x00 // divider disabled, měření času vypnuto
// V CREG3 nastavíme režim měření na "command mode" (0x40), standby VYP a cclk = 1024kHz (0)
#define CREG3_DEFAULT 0x40

// Konverzní konstanty – fullscale hodnoty pro jednotlivé kanály (viz Python kód)
#define FSRA 348160.0f
#define FSRB 387072.0f
#define FSRC 169984.0f

// Výchozí nastavení senzorových parametrů (jsou použity při výpočtu převodních konstant)
// Při default konfiguraci (viz CREG1_DEFAULT, CREG3_DEFAULT):
//  - Gain: GAIN_16X = 0x07  => gain_to_value = 1 << (11 - 7) = 16
//  - Integration time: INTEGRATION_TIME_256MS = 0x08 => 1 << 8 = 256
//  - CCLK: CCLK_FREQ_1024KHZ = 0x00 => 1024 * (1 << 0) = 1024
// Tedy společný faktor = 1 / (16 * 256 * 1024) = 1/4194304
//
// Převodní faktory pak:
//   UVA = raw_uva * (FSRA / 4194304)
//   UVB = raw_uvb * (FSRB / 4194304)
//   UVC = raw_uvc * (FSRC / 4194304)
//   Teplota = 0.05 * raw_temp - 66.9

#endif /* AS7331_H */
