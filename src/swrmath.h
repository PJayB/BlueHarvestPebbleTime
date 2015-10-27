#pragma once

#ifdef __cplusplus
extern "C" {
#endif

static inline void vec3_floor(vec3 out, const vec3 in) {
    out.v[0] = fix16_floor(in.v[0]);
    out.v[1] = fix16_floor(in.v[1]);
    out.v[2] = fix16_floor(in.v[2]);
}

static inline void vec2_floor(vec2 out, const vec2 in) {
    out.v[0] = fix16_floor(in.v[0]);
    out.v[1] = fix16_floor(in.v[1]);
}

void matrix_create_rotation_z(matrix m, fix16_t angle);
void matrix_multiply(matrix out, const matrix a, const matrix b);
void matrix_vector_transform(vec3 v, const matrix m);

#ifdef __cplusplus
}
#endif
