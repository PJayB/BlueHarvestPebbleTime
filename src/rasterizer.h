#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// For debugging
#define MAX_VIEWPORT_X 144
#define MAX_VIEWPORT_Y 144

#ifdef _WIN32
#   pragma warning(disable: 4214)
#endif

typedef struct _holomesh_face face_t;
typedef struct _holomesh_texture texture_t;

typedef enum _rasterizer_clip_plane_e {
    rasterizer_clip_left = 1,
    rasterizer_clip_right = 2,
    rasterizer_clip_top = 4,
    rasterizer_clip_bottom = 8
} rasterizer_clip_plane_e;

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

typedef struct _rasterizer_face_kickoff {
    uint16_t y : 12;
    uint16_t clip : 4;
    uint8_t hull_index;
    uint8_t face_index;

#ifdef SANDBOX
    fix16_t min_x, max_x, min_y, max_y;
#endif
} rasterizer_face_kickoff;

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

size_t rasterizer_create_face_kickoffs(rasterizer_face_kickoff* kickoffs, size_t max_kickoffs, const viewport_t* viewport, uint32_t hull_index, const vec3* points, const face_t* faces, size_t num_faces);
void rasterizer_sort_face_kickoffs(rasterizer_face_kickoff* faces, size_t num_faces);

void rasterizer_draw_span(rasterizer_context* ctx, const texture_t* texture, int16_t ia, int16_t ib, int16_t iy, fix16_t az, fix16_t bz, fix16_t ua, fix16_t ub, fix16_t va, fix16_t vb);
void rasterizer_advance_stepping_edge(rasterizer_stepping_edge* e, fix16_t y0, fix16_t y);

void rasterizer_init_span_pool(void);
rasterizer_stepping_span* rasterizer_allocate_stepping_span(void);
void rasterizer_free_stepping_span(rasterizer_stepping_span* span);
size_t rasterizer_get_active_stepping_span_count(void);

#ifdef __cplusplus
}
#endif

