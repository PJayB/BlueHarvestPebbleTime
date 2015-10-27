#pragma once

#include "libfixmath/fix16.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef fix16_t matrix[4][4];
typedef fix16_t vec3[3];

void matrix_create_rotation_z(matrix m, fix16_t angle);
void matrix_multiply(matrix out, const matrix a, const matrix b);
void matrix_vector_transform(vec3 v, const matrix m);

#ifdef __cplusplus
}
#endif
