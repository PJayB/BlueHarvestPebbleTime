#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void matrix_create_rotation_z(matrix m, fix16_t angle);
void matrix_multiply(matrix out, const matrix a, const matrix b);
void matrix_vector_transform(vec3 v, const matrix m);

#ifdef __cplusplus
}
#endif
