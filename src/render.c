#include "common.h"
#include "rasterizer.h"
#include "holomesh.h"
#include "render.h"
#include "matrix.h"
#include "wireframe.h"

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

void render_transform_hull(const holomesh_hull_t* hull, const matrix_t* transform, fix16_t viewport_half_width, fix16_t viewport_half_height, vec3_t* transformed_vertices) {
    for (size_t i = 0; i < hull->vertices.size; ++i) {
        vec3_t p = *((vec3_t*) &hull->vertices.ptr[i]);
        matrix_vector_transform(&p, transform);

        // Move to NDC
        p.x = fix16_floor(fix16_add(fix16_mul(p.x, viewport_half_width), viewport_half_width));
        p.y = fix16_floor(fix16_add(fix16_mul(p.y, viewport_half_height), viewport_half_height));

        transformed_vertices[i] = p;
    }
}

void render_draw_hull_wireframe(void* user_ptr, const holomesh_hull_t* hull, const vec3_t* transformed_vertices) {
    wireframe_context ctx;
    ctx.edge_indices = (const uint8_t*) hull->edges.ptr;
    ctx.num_edges = hull->edges.size;
    ctx.points = (vec3_t*) transformed_vertices;
    ctx.user_ptr = user_ptr;
    wireframe_draw(&ctx);
}

void render_draw_mesh_wireframe(void* user_ptr, const viewport_t* viewport, const holomesh_t* mesh, const matrix_t* transform, vec3_t* transformed_vertices) {
    fix16_t half_width = viewport->fwidth >> 1;
    fix16_t half_height = viewport->fheight >> 1;

    for (size_t i = 0; i < mesh->hulls.size; ++i) {
        const holomesh_hull_t* hull = &mesh->hulls.ptr[i];
        render_transform_hull(hull, transform, half_width, half_height, transformed_vertices);
        render_draw_hull_wireframe(user_ptr, hull, transformed_vertices);
    }
}

#ifdef RASTERIZER_CHECKS
size_t render_get_span_high_watermark(void) {
    return g_active_span_high_watermark;
}
#endif
