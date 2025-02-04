#ifndef AS7331_H
#define AS7331_H

// --- AS7331 Constants Definition ---
// Sensor address (7-bit) - HAL uses 8-bit address (shift left)
#define AS7331_ADDRESS (0x74 << 1)

// Register maps (note: in configuration vs. output bank)
// Configuration register bank:
#define OSR_REG 0x00 // Operational State Register (also contains bits for soft reset, mode, ...)
#define CREG1   0x06 // Configuration Register 1 (integration and gain)
#define CREG2   0x07 // Configuration Register 2 (divider, time measurement etc.)
#define CREG3   0x08 // Configuration Register 3 (measurement mode, standby, internal clock)

// Output register bank (active during measurement - automatically active):
#define OUT_REG_STATUS 0x00 // Status (contains NOTREADY bit)
#define OUT_REG_TEMP   0x01 // Temperature (raw, 16 bits)
#define OUT_REG_MRES1  0x02 // UVA Measurement (raw, 16 bits)
#define OUT_REG_MRES2  0x03 // UVB Measurement (raw, 16 bits)
#define OUT_REG_MRES3  0x04 // UVC Measurement (raw, 16 bits)

// Masks for OSR register
#define OSR_MASK_DOS    0x07 // Mask for device state (bits 0..2)
#define OSR_MASK_SW_RES 0x08 // Software reset
#define OSR_MASK_PD     0x40 // Power Down (not used in this example)
#define OSR_MASK_SS     0x80 // Start/Stop measurement (triggers single measurement)

// Device states (stored in OSR - DOS bits)
#define DEVICE_STATE_CONFIGURATION 0x02
#define DEVICE_STATE_MEASUREMENT   0x03

// Status register mask (output bank)
#define STATUS_MASK_NOTREADY 0x04 // If set, data is not ready yet

// Default configuration - inspired by set_default_config function in Python
// In CREG1, lower nibble is integration time, higher nibble is gain.
// We choose INTEGRATION_TIME_256MS = 0x08 and GAIN_16X = 0x07, therefore:
#define CREG1_DEFAULT 0x78 // (0x07 << 4) | 0x08
#define CREG2_DEFAULT 0x00 // divider disabled, time measurement off
// In CREG3 we set measurement mode to "command mode" (0x40), standby OFF and cclk = 1024kHz (0)
#define CREG3_DEFAULT 0x40

// Conversion constants - fullscale values for individual channels (see Python code)
#define FSRA 348160.0f
#define FSRB 387072.0f
#define FSRC 169984.0f

// I2C timeout (v ms)
#define I2C_TIMEOUT_MS 100

// Definice externího I2C sběrnice
#define I2C_BUS &furi_hal_i2c_handle_external

// Default sensor parameter settings (used in conversion constant calculations)
// With default configuration (see CREG1_DEFAULT, CREG3_DEFAULT):
//  - Gain: GAIN_16X = 0x07  => gain_to_value = 1 << (11 - 7) = 16
//  - Integration time: INTEGRATION_TIME_256MS = 0x08 => 1 << 8 = 256
//  - CCLK: CCLK_FREQ_1024KHZ = 0x00 => 1024 * (1 << 0) = 1024
// Thus common factor = 1 / (16 * 256 * 1024) = 1/4194304
//
// Conversion factors then:
//   UVA = raw_uva * (FSRA / 4194304)
//   UVB = raw_uvb * (FSRB / 4194304)
//   UVC = raw_uvc * (FSRC / 4194304)
//   Temperature = 0.05 * raw_temp - 66.9

// --- Function Declarations ---
// Low level I2C transfers
// Sensor initialization (soft reset, configuration registers setup)
void as7331_init(void);
bool as7331_write_reg(uint8_t reg, uint8_t value);
bool as7331_read_reg(uint8_t reg, uint8_t* value);
bool as7331_read_reg16(uint8_t reg, uint16_t* value);

// Start measurement (set start/stop bit)
bool as7331_start_measurement(void);

// Wait (polling) for measurement completion
bool as7331_wait_for_measurement(uint32_t timeout_ms);

// Read all measured values (UVA, UVB, UVC and temperature)
bool as7331_read_measurements(float* uva, float* uvb, float* uvc, float* temp);

#endif /* AS7331_H */
