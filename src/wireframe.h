#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _wireframe_context {
    viewport viewport;
    const holomesh* mesh;
    const vec3* transformed_points;
} wireframe_context;

void wireframe_draw(wireframe_context* context);

#ifdef __cplusplus
}
#endif
