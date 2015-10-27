#pragma once

#ifdef __cplusplus
extern "C" {
#endif

static inline void vec3_copy(vec3 out, const vec3 in) {
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
}
static inline void vec3_floor(vec3 out, const vec3 in) {
    out[0] = fix16_floor(in[0]);
    out[1] = fix16_floor(in[1]);
    out[2] = fix16_floor(in[2]);
}

static inline void vec2_copy(vec2 out, const vec2 in) {
    out[0] = in[0];
    out[1] = in[1];
}
static inline void vec2_floor(vec2 out, const vec2 in) {
    out[0] = fix16_floor(in[0]);
    out[1] = fix16_floor(in[1]);
}

void matrix_create_rotation_z(matrix m, fix16_t angle);
void matrix_multiply(matrix out, const matrix a, const matrix b);
void matrix_vector_transform(vec3 v, const matrix m);

#ifdef __cplusplus
}
#endif
