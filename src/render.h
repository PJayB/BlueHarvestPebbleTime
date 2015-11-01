#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void render_init(void);
void render_create_3d_transform(matrix_t* out, const matrix_t* proj, fix16_t angle);

#ifdef __cplusplus
}
#endif
