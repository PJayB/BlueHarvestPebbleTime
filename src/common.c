#include "common.h"

void viewport_init(viewport_t* viewport, uint16_t width, uint16_t height) {
    viewport->width = width;
    viewport->height = height;
    viewport->fwidth = fix16_from_int(width);
    viewport->fheight = fix16_from_int(height);
}

#ifdef PEBBLE
void assert(const char* file, int line, const char* condition) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "ASSERT: %s(%d): %s", file, line, condition);

    // Hang for 10 seconds (so logs can finish transferring)
    psleep(10000);
    
    // Crash
    *((uint32_t*)0) = 0;
}
#endif

