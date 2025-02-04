/* AS7331 UV senzor – Flipper aplikace v jazyce C
 *
 * Tato ukázka komunikuje se senzorem AS7331 přes I2C.
 * Na základě příkladu pro teploměr přes I2C a inspirováno vaší Python knihovnou.
 *
 * Poznámka: Adresa senzoru se při použití Flipper I2C HAL definuje jako 7-bit adresa
 * posunutá doleva (viz např. HTU2XD příklad).
 */

#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_i2c.h>
#include <gui/gui.h>
#include <input/input.h>
#include <notification/notification_messages.h>
#include <math.h>
#include <string.h>
#include "AS7331.h"

// I2C timeout (v ms)
#define I2C_TIMEOUT_MS 100

// Definice externího I2C sběrnice
#define I2C_BUS &furi_hal_i2c_handle_external

// --- Deklarace funkcí ---
// Nízká vrstva pro I2C přenosy
bool as7331_write_reg(uint8_t reg, uint8_t value);
bool as7331_read_reg(uint8_t reg, uint8_t* value);
bool as7331_read_reg16(uint8_t reg, uint16_t* value);

// Inicializace senzoru (soft reset, nastavení konfiguračních registrů)
void as7331_init(void);

// Spuštění měření (nastavení bitu start/stop)
bool as7331_start_measurement(void);

// Čekání (polling) na ukončení měření
bool as7331_wait_for_measurement(uint32_t timeout_ms);

// Čtení všech měřených hodnot (UVA, UVB, UVC a teplota)
bool as7331_read_measurements(float* uva, float* uvb, float* uvc, float* temp);

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

// --- Aplikační logika a zobrazení ---

// Globální buffery pro textové řetězce s výsledky
static char uva_str[16] = {0};
static char uvb_str[16] = {0};
static char uvc_str[16] = {0};
static char temp_str[16] = {0};

// Stav senzoru (pro zobrazení)
typedef enum {
    SensorStatusInitializing,
    SensorStatusNoSensor,
    SensorStatusDataReady,
} SensorStatus;

static SensorStatus sensor_status = SensorStatusInitializing;

// Typ události – timer nebo vstup
typedef enum {
    SensorEventTypeTick,
    SensorEventTypeInput,
} SensorEventType;

typedef struct {
    SensorEventType type;
    InputEvent input;
} SensorEvent;

// --- Callback pro vstup (klávesy) ---
static void sensor_input_callback(InputEvent* input_event, void* ctx) {
    FuriMessageQueue* event_queue = ctx;
    SensorEvent event = {.type = SensorEventTypeInput, .input = *input_event};
    furi_message_queue_put(event_queue, &event, FuriWaitForever);
}

// --- Timer callback (spouští aktualizaci dat) ---
static void sensor_timer_callback(void* context) {
    // Cast context to the expected type
    FuriMessageQueue* event_queue = (FuriMessageQueue*)context;
    SensorEvent event = {.type = SensorEventTypeTick};
    furi_message_queue_put(event_queue, &event, 0);
}

// --- Callback pro vykreslení obrazovky ---
static void sensor_draw_callback(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "AS7331 UV senzor");

    canvas_set_font(canvas, FontSecondary);
    switch(sensor_status) {
    case SensorStatusInitializing:
        canvas_draw_str(canvas, 2, 30, "Inicializace...");
        break;
    case SensorStatusNoSensor:
        canvas_draw_str(canvas, 2, 30, "Sensor not found!");
        break;
    case SensorStatusDataReady:
        canvas_draw_str(canvas, 2, 20, "UVA:");
        canvas_draw_str(canvas, 40, 20, uva_str);
        canvas_draw_str(canvas, 2, 30, "UVB:");
        canvas_draw_str(canvas, 40, 30, uvb_str);
        canvas_draw_str(canvas, 2, 40, "UVC:");
        canvas_draw_str(canvas, 40, 40, uvc_str);
        canvas_draw_str(canvas, 2, 50, "Teplota:");
        canvas_draw_str(canvas, 60, 50, temp_str);
        break;
    }
    canvas_draw_str(canvas, 2, 60, "Stiskni BACK pro ukončení.");
}

// --- Hlavní funkce aplikace ---
int32_t uv_sensor_app(void* p) {
    UNUSED(p);

    // Inicializace senzoru
    as7331_init();

    // Vytvoříme frontu událostí
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(SensorEvent));

    // Vytvoříme viewport a nastavíme callbacky pro kreslení a vstup
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, sensor_draw_callback, NULL);
    view_port_input_callback_set(view_port, sensor_input_callback, event_queue);

    // Vytvoříme periodický timer (např. 1 Hz) pro čtení dat ze senzoru
    FuriTimer* timer = furi_timer_alloc(sensor_timer_callback, FuriTimerTypePeriodic, event_queue);
    furi_timer_start(timer, furi_kernel_get_tick_frequency());

    // Přidáme viewport do GUI
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    // Otevřeme notifikační systém (pro blikání při úspěchu nebo chybě)
    NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);

    // Hlavní cyklus událostí
    SensorEvent event;
    while(1) {
        furi_message_queue_get(event_queue, &event, FuriWaitForever);
        if(event.type == SensorEventTypeInput) {
            // Pokud je stisknuto tlačítko BACK, ukončíme aplikaci
            if(event.input.key == InputKeyBack) break;
        } else if(event.type == SensorEventTypeTick) {
            float uva, uvb, uvc, temp;
            if(as7331_read_measurements(&uva, &uvb, &uvc, &temp)) {
                sensor_status = SensorStatusDataReady;
                snprintf(uva_str, sizeof(uva_str), "%.2f", (double)uva);
                snprintf(uvb_str, sizeof(uvb_str), "%.2f", (double)uvb);
                snprintf(uvc_str, sizeof(uvc_str), "%.2f", (double)uvc);
                snprintf(temp_str, sizeof(temp_str), "%.2f", (double)temp);
                // Notifikace – bliknutí modrou při úspěchu
                notification_message(notifications, &sequence_blink_blue_100);
            } else {
                sensor_status = SensorStatusNoSensor;
                // Notifikace – bliknutí červenou při chybě
                notification_message(notifications, &sequence_blink_red_100);
            }
            view_port_update(view_port);
        }
        furi_delay_ms(100);
    }

    // Uvolnění prostředků před ukončením
    furi_timer_stop(timer);
    furi_timer_free(timer);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);
    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_GUI);

    return 0;
}
