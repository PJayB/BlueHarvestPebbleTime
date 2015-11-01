#include "common.h"
#include "scratch.h"

static uint8_t* g_scratch = NULL;
static size_t g_scratch_offset = 0;
static size_t g_scratch_size = 0;

#ifdef RASTERIZER_CHECKS
static size_t g_scratch_hwm = 0;
#endif

void scratch_init(size_t size) {
    if (g_scratch != NULL) {
        free(g_scratch);
    }

    g_scratch = (uint8_t*) malloc(size);
    ASSERT(g_scratch);

    g_scratch_offset = 0;
    g_scratch_size = size;

#ifdef RASTERIZER_CHECKS
    g_scratch_hwm = 0;
#endif
}

void scratch_free(void) {
    if (g_scratch != NULL) {
        free(g_scratch);
        g_scratch_offset = 0;
        g_scratch_size = 0;

#ifdef RASTERIZER_CHECKS
        g_scratch_hwm = 0;
#endif
    }
}

void* scratch_alloc(size_t size) {
    size_t offset = g_scratch_offset;
    ASSERT(offset + size <= g_scratch_size);
    g_scratch_offset += size;

#ifdef RASTERIZER_CHECKS
    g_scratch_hwm = max(g_scratch_hwm, g_scratch_offset);
#endif

    return g_scratch + offset;
}

void scratch_clear(void) {
    g_scratch_offset = 0;
}

#ifdef RASTERIZER_CHECKS
size_t scratch_get_high_watermark(void) {
    return g_scratch_hwm;
}
#endif
