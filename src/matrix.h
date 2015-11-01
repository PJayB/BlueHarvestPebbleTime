#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void matrix_create_rotation_z(matrix_t* m, fix16_t angle);
void matrix_multiply(matrix_t* out, const matrix_t* a, const matrix_t* b);
void matrix_vector_transform(vec3_t* v, const matrix_t* m);

#ifdef __cplusplus
}
#endif
