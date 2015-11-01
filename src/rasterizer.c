#include "common.h"
#include "rasterizer.h"
#include "holomesh.h"
#include "facesort.h"

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
