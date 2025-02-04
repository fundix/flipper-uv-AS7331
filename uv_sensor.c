/* AS7331 UV senzor – Flipper aplikace v jazyce C
 *
 * Tato ukázka komunikuje se senzorem AS7331 přes I2C.
 * Na základě příkladu pro teploměr přes I2C a inspirováno vaší Python knihovnou.
 *
 * Poznámka: Adresa senzoru se při použití Flipper I2C HAL definuje jako 7-bit adresa
 * posunutá doleva (viz např. HTU2XD příklad).
 */

#include <furi.h>

#include <gui/gui.h>
#include <input/input.h>
#include <notification/notification_messages.h>
#include <math.h>
#include <string.h>
#include "AS7331.h"

#include "views/main_view.h"

// --- Aplikační logika a zobrazení ---

// Globální buffery pro textové řetězce s výsledky
static char uva_str[8] = {0};
static char uvb_str[8] = {0};
static char uvc_str[8] = {0};
static char temp_str[8] = {0};

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
    canvas_draw_rframe(canvas, 0, 0, 128, 64, 3);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 4, 10, "AS7331 sensor");

    canvas_set_font(canvas, FontSecondary);
    switch(sensor_status) {
    case SensorStatusInitializing:
        canvas_draw_str(canvas, 4, 30, "Initializing...");
        break;
    case SensorStatusNoSensor:
        canvas_draw_str(canvas, 4, 30, "Sensor not found!");
        break;
    case SensorStatusDataReady:
        canvas_draw_str(canvas, 4, 20, "UVA:");
        canvas_draw_str(canvas, 40, 20, uva_str);
        canvas_draw_str(canvas, 4, 30, "UVB:");
        canvas_draw_str(canvas, 40, 30, uvb_str);
        canvas_draw_str(canvas, 4, 40, "UVC:");
        canvas_draw_str(canvas, 40, 40, uvc_str);
        canvas_draw_str(canvas, 4, 50, "Temp:");
        canvas_draw_str(canvas, 40, 50, temp_str);
        break;
    }
    canvas_draw_str(canvas, 4, 60, "Press BACK to exit.");
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
                snprintf(uva_str, sizeof(uva_str), "%.3f", (double)uva);
                snprintf(uvb_str, sizeof(uvb_str), "%.3f", (double)uvb);
                snprintf(uvc_str, sizeof(uvc_str), "%.3f", (double)uvc);
                snprintf(temp_str, sizeof(temp_str), "%.1f", (double)temp);
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
