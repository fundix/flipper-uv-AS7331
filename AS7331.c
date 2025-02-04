
#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_i2c.h>
#include "AS7331.h"

// --- Implementace nízkoúrovňových funkcí ---

bool as7331_write_reg(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    bool success = false;
    furi_hal_i2c_acquire(I2C_BUS);
    success = furi_hal_i2c_tx(I2C_BUS, AS7331_ADDRESS, data, 2, furi_ms_to_ticks(I2C_TIMEOUT_MS));
    furi_hal_i2c_release(I2C_BUS);
    return success;
}

bool as7331_read_reg(uint8_t reg, uint8_t* value) {
    bool success = false;
    furi_hal_i2c_acquire(I2C_BUS);
    success = furi_hal_i2c_tx(I2C_BUS, AS7331_ADDRESS, &reg, 1, furi_ms_to_ticks(I2C_TIMEOUT_MS));
    if(success) {
        success =
            furi_hal_i2c_rx(I2C_BUS, AS7331_ADDRESS, value, 1, furi_ms_to_ticks(I2C_TIMEOUT_MS));
    }
    furi_hal_i2c_release(I2C_BUS);
    return success;
}

bool as7331_read_reg16(uint8_t reg, uint16_t* value) {
    uint8_t buf[2];
    bool success = false;
    furi_hal_i2c_acquire(I2C_BUS);
    success = furi_hal_i2c_tx(I2C_BUS, AS7331_ADDRESS, &reg, 1, furi_ms_to_ticks(I2C_TIMEOUT_MS));
    if(success) {
        success =
            furi_hal_i2c_rx(I2C_BUS, AS7331_ADDRESS, buf, 2, furi_ms_to_ticks(I2C_TIMEOUT_MS));
    }
    furi_hal_i2c_release(I2C_BUS);
    if(success) {
        // V datech jsou nejspíš uloženy jako [LSB, MSB]
        *value = ((uint16_t)buf[1] << 8) | buf[0];
    }
    return success;
}

// --- Inicializace senzoru ---
void as7331_init(void) {
    uint8_t osr;
    // Proveď soft reset
    if(as7331_read_reg(OSR_REG, &osr)) {
        osr |= OSR_MASK_SW_RES;
        as7331_write_reg(OSR_REG, osr);
        furi_delay_ms(100);
    }
    // Přepni do konfiguračního režimu
    if(as7331_read_reg(OSR_REG, &osr)) {
        osr = (osr & ~OSR_MASK_DOS) | DEVICE_STATE_CONFIGURATION;
        as7331_write_reg(OSR_REG, osr);
    }
    // Nastav konfigurační registry na výchozí hodnoty
    as7331_write_reg(CREG1, CREG1_DEFAULT);
    as7331_write_reg(CREG2, CREG2_DEFAULT);
    as7331_write_reg(CREG3, CREG3_DEFAULT);
    // Přepni do měřicího režimu
    if(as7331_read_reg(OSR_REG, &osr)) {
        osr = (osr & ~OSR_MASK_DOS) | DEVICE_STATE_MEASUREMENT;
        as7331_write_reg(OSR_REG, osr);
    }
}

// --- Spuštění měření ---
bool as7331_start_measurement(void) {
    uint8_t osr;
    if(!as7331_read_reg(OSR_REG, &osr)) return false;
    osr |= OSR_MASK_SS;
    return as7331_write_reg(OSR_REG, osr);
}

// --- Čekání na dokončení měření (polling status registru) ---
bool as7331_wait_for_measurement(uint32_t timeout_ms) {
    uint32_t start = furi_get_tick();
    uint8_t status;
    while(furi_get_tick() - start < furi_ms_to_ticks(timeout_ms)) {
        if(as7331_read_reg(OUT_REG_STATUS, &status)) {
            if(!(status & STATUS_MASK_NOTREADY)) {
                return true;
            }
        }
        furi_delay_ms(10);
    }
    return false;
}

// --- Čtení měřených hodnot a jejich převod ---
bool as7331_read_measurements(float* uva, float* uvb, float* uvc, float* temp) {
    if(!as7331_start_measurement()) return false;
    // Požadovaný integrační čas je 256 ms; přidáme malou rezervu.
    furi_delay_ms(300);
    if(!as7331_wait_for_measurement(500)) return false;
    uint16_t raw_temp, raw_uva, raw_uvb, raw_uvc;
    if(!as7331_read_reg16(OUT_REG_TEMP, &raw_temp)) return false;
    if(!as7331_read_reg16(OUT_REG_MRES1, &raw_uva)) return false;
    if(!as7331_read_reg16(OUT_REG_MRES2, &raw_uvb)) return false;
    if(!as7331_read_reg16(OUT_REG_MRES3, &raw_uvc)) return false;
    // Převodní výpočty – výchozí konfigurace:
    // gain_to_value = 16, integration_time = 256, cclk = 1024  => společný faktor = 1/4194304
    float common_factor = 1.0f / (16.0f * 256.0f * 1024.0f); // 1/4194304
    float conv_factor_a = FSRA * common_factor;
    float conv_factor_b = FSRB * common_factor;
    float conv_factor_c = FSRC * common_factor;
    *uva = raw_uva * conv_factor_a;
    *uvb = raw_uvb * conv_factor_b;
    *uvc = raw_uvc * conv_factor_c;
    *temp = 0.05f * raw_temp - 66.9f;
    return true;
}
