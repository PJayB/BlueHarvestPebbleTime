#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// For debugging
#define MAX_VIEWPORT_X 144
#define MAX_VIEWPORT_Y 144

typedef struct _face_kickoff face_kickoff;
typedef struct _holomesh_texture holomesh_texture;

static inline uint8_t rasterizer_decode_texel_2bit(
    const uint8_t* data,
    uint16_t stride,
    uint16_t u, uint16_t v) {
    // Divide the U coordinate by 4
    uint16_t u4 = u >> 2;
    // Sample from the texture
    uint8_t packed4 = data[v * stride + u4];
    // Shift the texel from the group of 4 into place
    uint8_t unpacked = (packed4 >> (2 * (u - u4 * 4)));
    // Mask off higher texels
    return unpacked & 3;
}

extern void rasterizer_set_pixel(
    void* user_ptr,
    int x, int y,
    uint8_t color);

typedef struct _rasterizer_stepping_edge_y {
    fix16_t y0, y1, d;
} rasterizer_stepping_edge_y;

typedef struct _rasterizer_stepping_edge {
    fix16_t x, step_x;
    fix16_t z, step_z;
    fix16_t u, step_u;
    fix16_t v, step_v;

#ifdef RASTERIZER_CHECKS
    fix16_t min_x, max_x;
#endif
} rasterizer_stepping_edge;

typedef struct _rasterizer_stepping_span {
    rasterizer_stepping_edge e0, e1;
    struct _rasterizer_stepping_span* next_span;
    uint16_t y0, y1;
    uint8_t texture_id;
} rasterizer_stepping_span;

typedef struct _rasterizer_context {
    fix16_t* depths;
    void* user_ptr;
} rasterizer_context;

void rasterizer_draw_span(
    rasterizer_context* ctx,
    const holomesh_texture* texture,
    int16_t ia, int16_t ib,
    int16_t iy,
    fix16_t az, fix16_t bz,
    fix16_t ua, fix16_t ub,
    fix16_t va, fix16_t vb);

#ifdef __cplusplus
}
#endif

