#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <furi_hal.h>
#include <stdlib.h> // For generating random values
#include "views/main_view.h"

// cmd+shift+b -> menu
// ufbg - cli command

int32_t uv_sensor_app(void* p) {
    UNUSED(p);

    // Opening GUI and allocating view port
    Gui* gui = furi_record_open(RECORD_GUI);
    ViewPort* view_port = view_port_alloc();

    // Initializing display context with default values
    MainViewContext context = {
        .running = true,
        .uva = 2.5f,
        .uvb = 1.8f,
        .uvc = 0.4f,
        .view_port = view_port,
    };

    // Setting callbacks for rendering and input processing
    view_port_draw_callback_set(view_port, main_view_render, &context);
    view_port_input_callback_set(view_port, main_view_input, &context);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    // Main application loop
    while(context.running) {
        furi_delay_ms(100);
    }

    // Cleanup and application exit
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);

    return 0;
}
