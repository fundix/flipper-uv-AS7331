#include "AS7331.h"

// --- Implementace nízkoúrovňových funkcí ---

bool as7331_write_reg(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    bool success = false;
    furi_hal_i2c_acquire(I2C_BUS);

    // Use 8-bit addressing
    success = furi_hal_i2c_tx(I2C_BUS, AS7331_ADDRESS, data, 2, furi_ms_to_ticks(I2C_TIMEOUT_MS));

    furi_hal_i2c_release(I2C_BUS);
    return success;
}

bool as7331_read_reg(uint8_t reg, uint8_t* value) {
    bool success = false;
    furi_hal_i2c_acquire(I2C_BUS);

    // Use the 8-bit address format
    success = furi_hal_i2c_tx(I2C_BUS, AS7331_ADDRESS, &reg, 1, furi_ms_to_ticks(I2C_TIMEOUT_MS));

    furi_delay_ms(1); // Small delay before reading

    if(success) {
        success =
            furi_hal_i2c_rx(I2C_BUS, AS7331_ADDRESS, value, 1, furi_ms_to_ticks(I2C_TIMEOUT_MS));
    }

    furi_hal_i2c_release(I2C_BUS);
    return success;
}

void as7331_dump_registers(void) {
    uint8_t val;
    FURI_LOG_I("AS7331", "--- AS7331 REGISTER DUMP ---");

    if(as7331_read_reg(OSR_REG, &val)) {
        FURI_LOG_I("AS7331", "OSR_REG: 0x%02X", val);
    } else {
        FURI_LOG_E("AS7331", "Failed to read OSR_REG");
    }

    if(as7331_read_reg(CREG1, &val)) {
        FURI_LOG_I("AS7331", "CREG1: 0x%02X", val);
    } else {
        FURI_LOG_E("AS7331", "Failed to read CREG1");
    }

    if(as7331_read_reg(CREG2, &val)) {
        FURI_LOG_I("AS7331", "CREG2: 0x%02X", val);
    } else {
        FURI_LOG_E("AS7331", "Failed to read CREG2");
    }

    if(as7331_read_reg(CREG3, &val)) {
        FURI_LOG_I("AS7331", "CREG3: 0x%02X", val);
    } else {
        FURI_LOG_E("AS7331", "Failed to read CREG3");
    }

    if(as7331_read_reg(OUT_REG_STATUS, &val)) {
        FURI_LOG_I("AS7331", "OUT_REG_STATUS: 0x%02X", val);
    } else {
        FURI_LOG_E("AS7331", "Failed to read OUT_REG_STATUS");
    }

    if(as7331_read_reg(0x02, &val)) { // AGEN - ID registr
        FURI_LOG_I("AS7331", "AGEN (Device ID): 0x%02X", val);
    } else {
        FURI_LOG_E("AS7331", "Failed to read AGEN (Device ID)");
    }

    FURI_LOG_I("AS7331", "--- END OF DUMP ---");
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

void i2c_scan(void) {
    furi_hal_i2c_acquire(I2C_BUS);
    // scan
    for(uint8_t addr = 0x01; addr <= 0x7F << 1; addr++) {
        // Check for peripherals
        if(furi_hal_i2c_is_device_ready(I2C_BUS, addr, 3)) {
            // skip even 8-bit addr
            if(addr % 2 != 0) {
                continue;
            }

            FURI_LOG_I("I2C_SCAN", "Device found at: 0x%02X", addr >> 1);
        }
    }
    furi_hal_i2c_release(I2C_BUS);
}

void as7331_test_device_id(void) {
    uint8_t device_id = 0;
    if(as7331_read_reg(0x02, &device_id)) {
        FURI_LOG_I("AS7331", "Device ID: 0x%02X (Expected: 0x21)", device_id);
    } else {
        FURI_LOG_E("AS7331", "Failed to read Device ID!");
    }
}

// --- Inicializace senzoru ---
void as7331_init(void) {
    uint8_t osr;

    i2c_scan();

    FURI_LOG_I("AS7331", "Starting initialization...");

    // // 2. Proveď soft reset
    // if(as7331_read_reg(OSR_REG, &osr)) {
    //     FURI_LOG_I("AS7331", "OSR_REG before soft reset: 0x%02X", osr);
    //     osr |= OSR_MASK_SW_RES; // Soft Reset
    //     if(!as7331_write_reg(OSR_REG, osr)) {
    //         FURI_LOG_E("AS7331", "Failed to write soft reset to OSR_REG.");
    //         return;
    //     }
    //     furi_delay_ms(100); // Wait for reset to complete
    // } else {
    //     FURI_LOG_E("AS7331", "Failed to read OSR_REG before soft reset.");
    //     return;
    // }

    uint8_t device_id = 0;
    if(as7331_read_reg(0x02, &device_id)) {
        FURI_LOG_I("AS7331", "Device ID: 0x%02X", device_id);
    } else {
        FURI_LOG_E("AS7331", "Failed to read Device ID!");
    }

    // 3. Přepni do konfiguračního režimu
    if(as7331_read_reg(OSR_REG, &osr)) {
        osr = (osr & ~OSR_MASK_PD) | DEVICE_STATE_CONFIGURATION; // PD = 0, DOS = 010
        if(!as7331_write_reg(OSR_REG, osr)) {
            FURI_LOG_E("AS7331", "Failed to write CONFIG mode to OSR_REG.");
            return;
        }
        furi_delay_ms(10); // Wait for transition
        FURI_LOG_I("AS7331", "OSR_REG after setting CONFIG: 0x%02X", osr);
    } else {
        FURI_LOG_E("AS7331", "Failed to read OSR_REG before CONFIG mode.");
        return;
    }

    // 4. Nastav konfigurační registry
    if(!as7331_write_reg(CREG1, CREG1_DEFAULT)) {
        FURI_LOG_E("AS7331", "Failed to write CREG1.");
        return;
    }
    if(!as7331_write_reg(CREG2, CREG2_DEFAULT)) {
        FURI_LOG_E("AS7331", "Failed to write CREG2.");
        return;
    }
    if(!as7331_write_reg(CREG3, CREG3_CONT)) {
        FURI_LOG_E("AS7331", "Failed to write CREG3.");
        return;
    }

    // 5. Přepni do měřicího režimu
    if(as7331_read_reg(OSR_REG, &osr)) {
        osr = (osr & ~OSR_MASK_PD) | DEVICE_STATE_MEASUREMENT; // PD = 0, DOS = 011
        if(!as7331_write_reg(OSR_REG, osr)) {
            FURI_LOG_E("AS7331", "Failed to write MEASUREMENT mode to OSR_REG.");
            return;
        }
        furi_delay_ms(10); // Wait for transition
        FURI_LOG_I("AS7331", "OSR_REG after setting MEASUREMENT: 0x%02X", osr);
    } else {
        FURI_LOG_E("AS7331", "Failed to read OSR_REG before MEASUREMENT mode.");
        return;
    }

    // 6. Spuštění kontinuálního měření (nastavení SS bitu)
    if(as7331_read_reg(OSR_REG, &osr)) {
        osr |= OSR_MASK_SS; // SS = 1
        if(!as7331_write_reg(OSR_REG, osr)) {
            FURI_LOG_E("AS7331", "Failed to write START to OSR_REG.");
            return;
        }
        furi_delay_ms(10); // Wait for start
        FURI_LOG_I("AS7331", "OSR_REG after forcing START: 0x%02X", osr);
    } else {
        FURI_LOG_E("AS7331", "Failed to read OSR_REG before forcing START.");
        return;
    }

    FURI_LOG_I("AS7331", "Initialization complete.");
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

bool as7331_read_all_raw(uint8_t* data) {
    uint8_t reg = OUT_REG_TEMP; // Uložení registru do proměnné
    furi_hal_i2c_acquire(I2C_BUS);
    bool success =
        furi_hal_i2c_tx(I2C_BUS, AS7331_ADDRESS, &reg, 1, furi_ms_to_ticks(I2C_TIMEOUT_MS));
    if(success) {
        success =
            furi_hal_i2c_rx(I2C_BUS, AS7331_ADDRESS, data, 8, furi_ms_to_ticks(I2C_TIMEOUT_MS));
    }
    furi_hal_i2c_release(I2C_BUS);
    return success;
}

// --- Čtení měřených hodnot a jejich převod ---
int8_t as7331_read_measurements(float* uva, float* uvb, float* uvc, float* temp) {
    uint8_t raw_data[8];
    if(as7331_read_all_raw(raw_data)) {
        FURI_LOG_I(
            "AS7331",
            "Raw data: %02X %02X %02X %02X %02X %02X %02X %02X",
            raw_data[0],
            raw_data[1],
            raw_data[2],
            raw_data[3],
            raw_data[4],
            raw_data[5],
            raw_data[6],
            raw_data[7]);
    } else {
        FURI_LOG_E("AS7331", "Failed to read raw data.");
    }

    // Spočítejte UVA, UVB, UVC a teplotu
    uint16_t raw_temp = ((uint16_t)raw_data[1] << 8) | raw_data[0];
    uint16_t raw_uva = ((uint16_t)raw_data[3] << 8) | raw_data[2];
    uint16_t raw_uvb = ((uint16_t)raw_data[5] << 8) | raw_data[4];
    uint16_t raw_uvc = ((uint16_t)raw_data[7] << 8) | raw_data[6];

    // Výpočet konverzních faktorů
    float common_factor = 1.0f / (16.0f * 256.0f * 1024.0f); // 1/4194304
    *uva = raw_uva * FSRA * common_factor;
    *uvb = raw_uvb * FSRB * common_factor;
    *uvc = raw_uvc * FSRC * common_factor;
    *temp = 0.05f * raw_temp - 66.9f;

    return 1;
}
