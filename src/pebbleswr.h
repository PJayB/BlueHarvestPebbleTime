#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _pswr_context {
    // TODO
    int width;
    int height;
} pswr_context;

void pswr_init_context(pswr_context* context, int width, int height);

#ifdef __cplusplus
}
#endif
