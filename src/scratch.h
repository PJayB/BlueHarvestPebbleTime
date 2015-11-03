#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void scratch_set(void* ptr, size_t size);
void scratch_init(size_t size);
void scratch_free(void);
void* scratch_alloc(size_t size);
void scratch_clear(void);
size_t scratch_used_bytes(void);
size_t scratch_size_bytes(void);

#ifdef RASTERIZER_CHECKS
size_t scratch_get_high_watermark_bytes(void);
#endif

#ifdef __cplusplus
}
#endif
