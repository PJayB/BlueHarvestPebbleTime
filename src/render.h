#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct render_transform_hulls_info_s {
    vec3_t** transformed_points; // outputs from scratch
    fix16_t min_x, min_y, max_x, max_y; // output bounding box in screen space
} render_transform_hulls_info_t;

typedef struct render_frame_buffer_s {
    uint8_t* data;
    uint32_t row_stride;
} render_frame_buffer_t;

extern int8_t get_color_mod(uint8_t y);

void render_init(void);
void render_prep_frame(void);
void render_create_3d_transform(matrix_t* out, const matrix_t* proj, fix16_t angle_x, fix16_t angle_z);
void render_transform_hulls(const holomesh_hull_t* hulls, size_t num_hulls, const viewport_t* viewport, const matrix_t* transform, render_transform_hulls_info_t* info);
void render_draw_hull_wireframe(void* user_ptr, const holomesh_hull_t* hull, const vec3_t* transformed_vertices);
void render_draw_mesh_wireframe(void* user_ptr, const holomesh_t* mesh, const vec3_t* const* transformed_points);
void render_draw_mesh_solid(render_frame_buffer_t* frame_buffer, const viewport_t* viewport, const holomesh_t* mesh, const vec3_t* const* transformed_points);
void render_transform_point(vec3_t* out_p, const vec3_t* p, const matrix_t* transform, fix16_t viewport_half_width, fix16_t viewport_half_height);

uint32_t render_get_overdraw(void);
uint32_t render_get_pixels_out(void);

#ifdef __cplusplus
}
#endif
