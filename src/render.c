#include "common.h"
#include "rasterizer.h"
#include "holomesh.h"
#include "render.h"
#include "matrix.h"

#ifdef RASTERIZER_CHECKS
static size_t g_active_span_high_watermark = 0;
#endif

void render_init(void) {
    rasterizer_init_span_pool();
}

void render_create_3d_transform(matrix_t* out, const matrix_t* proj, fix16_t angle) {
    // Rotate the transform
    matrix_t rotation;
    matrix_create_rotation_z(&rotation, angle);

    // Create the final transform
    matrix_multiply(out, &rotation, proj);
}



#ifdef RASTERIZER_CHECKS
size_t render_get_span_high_watermark(void) {
    return g_active_span_high_watermark;
}
#endif
