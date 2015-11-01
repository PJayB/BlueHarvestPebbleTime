#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void render_init(void);
void render_create_3d_transform(matrix_t* out, const matrix_t* proj, fix16_t angle);
void render_transform_hull(const holomesh_hull_t* hull, const matrix_t* transform, fix16_t viewport_half_width, fix16_t viewport_half_height, vec3_t* transformed_vertices);

#ifdef __cplusplus
}
#endif
