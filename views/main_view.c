#include "main_view.h"
#include <stdio.h>
#include <stdlib.h>

void main_view_scan(MainViewContext* context) {
    // Simulace skenování: náhodně generujeme UV hodnoty v rozsahu 0.00 - 5.00.
    context->uva = (float)(rand() % 500) / 100.0f;
    context->uvb = (float)(rand() % 500) / 100.0f;
    context->uvc = (float)(rand() % 500) / 100.0f;
}

void main_view_render(Canvas* canvas, void* ctx) {
    MainViewContext* context = ctx;

    canvas_clear(canvas);
    canvas_draw_rframe(canvas, 0, 0, 128, 64, 3);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 5, 12, "UV Sensor");

    char buffer[32];
    canvas_set_font(canvas, FontSecondary);

    snprintf(buffer, sizeof(buffer), "UVA: %.2f", (double)context->uva);
    canvas_draw_str(canvas, 5, 25, buffer);

    snprintf(buffer, sizeof(buffer), "UVB: %.2f", (double)context->uvb);
    canvas_draw_str(canvas, 5, 35, buffer);

    snprintf(buffer, sizeof(buffer), "UVC: %.2f", (double)context->uvc);
    canvas_draw_str(canvas, 5, 45, buffer);

    canvas_draw_str(canvas, 84, 12, "C0->SCL");
    canvas_draw_str(canvas, 84, 22, "C1->SDA");

    canvas_draw_rbox(canvas, 45, 48, 45, 13, 3);
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_circle(canvas, 54, 54, 4);
    canvas_draw_disc(canvas, 54, 54, 2);
    canvas_draw_str_aligned(canvas, 62, 51, AlignLeft, AlignTop, "Scan");
}

void main_view_input(InputEvent* event, void* ctx) {
    MainViewContext* context = ctx;

    if(event->type == InputTypeShort) {
        if(event->key == InputKeyBack) {
            // Ukončení aplikace.
            context->running = false;
        } else if(event->key == InputKeyOk) {
            // Simulace nového skenu a aktualizace obrazovky.
            main_view_scan(context);
            view_port_update(context->view_port);
        }
    }
}
