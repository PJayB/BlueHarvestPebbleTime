#include "common.h"
#include "wireframe.h"

extern void wireframe_draw_line(
    void* user_ptr,
    int x0, 
    int y0,
    int x1,
    int y1);

void wireframe_draw(wireframe_context* ctx) {
    const uint8_t* edge = ctx->edge_indices;
    for (size_t i = 0; i < ctx->num_edges; ++i, edge += 2)
    {
        vec3 a = ctx->points[edge[0]];
        vec3 b = ctx->points[edge[1]];
        wireframe_draw_line(
            ctx->user_ptr, 
            fix16_to_int_floor(a.v[0]), 
            fix16_to_int_floor(a.v[1]), 
            fix16_to_int_floor(b.v[0]), 
            fix16_to_int_floor(b.v[1]));
    }
}
