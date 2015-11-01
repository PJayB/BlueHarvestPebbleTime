#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void scratch_init(size_t size);
void scratch_free(void);
void* scratch_alloc(size_t size);
void scratch_clear(void);

#ifdef RASTERIZER_CHECKS
size_t scratch_get_high_watermark(void);
#endif

#ifdef __cplusplus
}
#endif
