#include "common.h"

void viewport_init(viewport_t* viewport, uint16_t width, uint16_t height) {
    viewport->width = width;
    viewport->height = height;
    viewport->fwidth = fix16_from_int(width);
    viewport->fheight = fix16_from_int(height);
}

#ifdef PEBBLE
void assert(const char* file, int line, const char* condition) {
    while (1) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "ASSERT: %s(%d): %s", file, line, condition);
        psleep(1000);
    }
}
#endif

