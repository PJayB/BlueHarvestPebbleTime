#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _wireframe_context {
    const vec3_t* points;
    const uint8_t* edge_indices;
    size_t num_edges;
    void* user_ptr;
} wireframe_context;

void wireframe_draw(wireframe_context* context);

#ifdef __cplusplus
}
#endif
