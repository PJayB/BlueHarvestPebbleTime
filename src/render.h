#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void render_init(void);
void render_create_3d_transform(matrix_t* out, const matrix_t* proj, fix16_t angle);
void render_transform_hull(const holomesh_hull_t* hull, const matrix_t* transform, fix16_t viewport_half_width, fix16_t viewport_half_height, vec3_t* transformed_vertices);
void render_draw_hull_wireframe(void* user_ptr, const holomesh_hull_t* hull, const vec3_t* transformed_vertices);
void render_draw_mesh_wireframe(void* user_ptr, const viewport_t* viewport, const holomesh_t* mesh, const matrix_t* transform, vec3_t* transformed_vertices);

#ifdef __cplusplus
}
#endif
