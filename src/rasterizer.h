#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _face_kickoff face_kickoff;

typedef struct _stepping_edge {
    fix16_t x, step_x;
    fix16_t z, step_z;
    fix16_t u, step_u;
    fix16_t v, step_v;
} stepping_edge;

typedef struct _stepping_span {
    stepping_edge e0, e1;
    struct _stepping_span* next_span;
    uint8_t y0, y1;
    uint8_t texture;
} stepping_span;

typedef struct _rasterizer_context {
    const vec3* points;
    face_kickoff* face_kickoffs;
    stepping_span span_pool;
    size_t span_pool_size;
    size_t face_kickoffs_count;
    fix16_t* scanline_depth_buffer;
    void* user_ptr;
    // todo: hulls, textures
} rasterizer_context;

// Construct face kickoffs and spans from hulls
void create_face_kickoffs(face_kickoff* kickoffs, size_t kickoff_pool_size);

// Do the rasterization
void rasterize(rasterizer_context* context);

#ifdef __cplusplus
}
#endif

