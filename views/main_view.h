#ifndef MAIN_VIEW_H
#define MAIN_VIEW_H

#include <stdbool.h>
#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>

// Kontext, který uchovává data potřebná pro vykreslování a ovládání.
typedef struct {
    bool running;
    float uva;
    float uvb;
    float uvc;
    ViewPort* view_port; // Pro možnost aktualizace obrazovky z callbacku.
} MainViewContext;

// Deklarace funkcí pro vykreslování, zpracování vstupu a simulaci skenování.
void main_view_render(Canvas* canvas, void* context);
void main_view_input(InputEvent* event, void* context);
void main_view_scan(MainViewContext* context);

#endif // MAIN_VIEW_H
