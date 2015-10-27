#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _holomesh holomesh;

typedef struct _wireframe_context {
    int width;
    int height;
    const holomesh* mesh;
} wireframe_context;

void wireframe_init_context(wireframe_context* context, const holomesh* holomesh, int width, int height);
void wireframe_draw(wireframe_context* context);

#ifdef __cplusplus
}
#endif
