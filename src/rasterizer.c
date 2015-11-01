#include "common.h"
#include "rasterizer.h"
#include "holomesh.h"

void rasterizer_draw_span(
    rasterizer_context* ctx,
    const holomesh_texture* texture,
    int16_t ia, int16_t ib,
    int16_t iy,
    fix16_t az, fix16_t bz,
    fix16_t ua, fix16_t ub,
    fix16_t va, fix16_t vb) {
    
    // Skip zero size lines
    if (ia == ib) return;

    ASSERT(ia <= ib);
    ASSERT(ia < MAX_VIEWPORT_X);
    ASSERT(ib >= 0);
    ASSERT(iy >= 0);
    ASSERT(iy < MAX_VIEWPORT_Y);
    ASSERT(texture != NULL);

    // Get the interpolants
    fix16_t g = fix16_rcp(fix16_from_int(ib - ia));
    fix16_t step_z = fix16_mul(fix16_sub(bz, az), g);
    fix16_t z = az;

    fix16_t step_u = fix16_mul(fix16_sub(ub, ua), g);
    fix16_t step_v = fix16_mul(fix16_sub(vb, va), g);
    fix16_t base_u = ua;
    fix16_t base_v = va;

    // Iterate over the scanline
    for (int16_t ix = ia; ix < ib; ++ix) {
        ASSERT(ix >= 0 && ix < MAX_VIEWPORT_X);

        fix16_t oldZ = ctx->depths[ix];
        if (z > 0 && oldZ < z) 
        {
            // TODO: only interpolate this occasionally
            fix16_t iz = fix16_rcp(z);

            // Get the texture coordinates in [0,1]
            fix16_t u = fix16_ufrac(fix16_mul(base_u, iz));
            fix16_t v = fix16_ufrac(fix16_mul(base_v, iz));

            // Scale the UVs to texture size
            u = fix16_mul(u, texture->scale_u);
            v = fix16_mul(v, texture->scale_v);

            // Get the texel 
            uint16_t iu = (uint16_t) fix16_to_int_floor(u);
            uint16_t iv = (uint16_t) fix16_to_int_floor(v);
            uint8_t p = rasterizer_decode_texel_2bit(
                texture->data.ptr,
                texture->stride,
                iu, iv);

            // Set the pixel
            rasterizer_set_pixel(ctx->user_ptr, ix, iy, p);

            ctx->depths[ix] = z;
        }

        z += step_z;
        base_u += step_u;
        base_v += step_v;
    }
}

void rasterizer_advance_stepping_edge(rasterizer_stepping_edge* e, fix16_t y0, fix16_t y) {
    fix16_t my = fix16_sub(y, y0);
    if (my == 0)
        return;

    e->x += fix16_mul(e->step_x, my);
    e->z += fix16_mul(e->step_z, my);
    e->u += fix16_mul(e->step_u, my);
    e->v += fix16_mul(e->step_v, my);
}

size_t rasterizer_create_face_kickoffs(rasterizer_face_kickoff* kickoffs, size_t max_kickoffs, const viewport_t* viewport, uint32_t hull_index, const vec3* points, const face_t* faces, size_t num_faces) {
    rasterizer_face_kickoff* current_kickoff = kickoffs;
    rasterizer_face_kickoff* end_kickoff = kickoffs + max_kickoffs;

    ASSERT(hull_index < 256);
    ASSERT(num_faces < 256);

    const face_t* face = faces;
    for (size_t i = 0; i < num_faces && current_kickoff < end_kickoff; ++i, ++face) {
        const vec3 a = points[face->positions.a];
        const vec3 b = points[face->positions.b];
        const vec3 c = points[face->positions.c];

        // Cross product gives us z direction: if it's facing away from us, ignore it
        fix16_t ux = fix16_sub(b.x, a.x);
        fix16_t uy = fix16_sub(b.y, a.y);
        fix16_t vx = fix16_sub(c.x, a.x);
        fix16_t vy = fix16_sub(c.y, a.y);
        if (fix16_sub(fix16_mul(ux, vy), fix16_mul(uy, vx)) < 0)
            continue;

        fix16_t min_y, max_y, min_x, max_x;
        min_x = fix16_min(a.x, fix16_min(b.x, c.x));
        max_x = fix16_max(a.x, fix16_max(b.x, c.x));
        min_y = fix16_min(a.y, fix16_min(b.y, c.y));
        max_y = fix16_max(a.y, fix16_max(b.y, c.y));

        // Cull zero size
        if (min_x == max_x || min_y == max_y)
            continue;

        // Cull out of bounds
        if (max_x < 0)
            continue;
        if (min_x >= viewport->fwidth)
            continue;
        if (max_y < 0)
            continue;
        if (min_y >= viewport->fheight)
            continue;

        // Setup clipping flags
        uint8_t clip = 0;
        if (min_x < 0) clip |= rasterizer_clip_left;
        if (max_x > viewport->fwidth) clip |= rasterizer_clip_right;
        if (min_y < 0) clip |= rasterizer_clip_top;
        if (max_y > viewport->fheight) clip |= rasterizer_clip_bottom;

        int16_t kickoff_y = fix16_to_int_floor(min_y);

        current_kickoff->hull_index = (uint8_t) hull_index;
        current_kickoff->face_index = (uint8_t) i;
        current_kickoff->y = kickoff_y < 0 ? 0 : kickoff_y;
        current_kickoff->clip = clip;
#ifdef SANDBOX
        current_kickoff->min_x = min_x;
        current_kickoff->max_x = max_x;
        current_kickoff->min_y = min_y;
        current_kickoff->max_y = max_y;
#endif
        current_kickoff++;
    }

    return (size_t)(current_kickoff - kickoffs);
}
