#include "common.h"

void viewport_init(viewport_t* viewport, uint16_t width, uint16_t height) {
    viewport->width = width;
    viewport->height = height;
    viewport->fwidth = fix16_from_int(width);
    viewport->fheight = fix16_from_int(height);
}
