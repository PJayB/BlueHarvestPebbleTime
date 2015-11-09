#include "common.h"
#include "rasterizer.h"
#include "holomesh.h"

#ifndef PEBBLE
#   pragma optimize("", off)
#endif

//#define PERSPECTIVE_CORRECT
//#define ACCURATE_PERSPECTIVE_CORRECT
//#define DISABLE_LONG_SPAN_OPTIMIZATIONS

FORCE_INLINE uint8_t rasterizer_decode_texel_2bit(
    const uint8_t* data,
    uint16_t stride,
    uint16_t u, uint16_t v) {    
    ASSERT((stride & 0x3) == 0);
    ASSERT(((v * stride) & 0x3) == 0);
    
    const uint32_t* data32 = (const uint32_t*) (data + v * stride);
    
    // Divide the U coordinate by 16
    uint16_t u16 = u >> 4;
    // Sample from the texture
    uint32_t packed16 = data32[u16];
    // Shift the texel from the group of 4 into place
    uint8_t unpacked = (uint8_t)(packed16 >> (2 * (u & 15)));
    // Mask off higher texels
    return unpacked & 3;
}

#ifdef PERSPECTIVE_CORRECT
FORCE_INLINE uint8_t rasterizer_get_fragment_color(fix16_t base_u, fix16_t base_v, fix16_t iz, const texture_t* texture, void* user_ptr) {
    // Get the texture coordinates in [0,1]
    fix16_t u = fixp16_ufrac(fixp16_mul(base_u, iz));
    fix16_t v = fixp16_ufrac(fixp16_mul(base_v, iz));

    // Scale the UVs to texture size
    u = fixp16_mul(u, texture->scale_u);
    v = fixp16_mul(v, texture->scale_v);

    // Get the texel 
    uint16_t iu = (uint16_t) fixp16_to_int_floor(u);
    uint16_t iv = (uint16_t) fixp16_to_int_floor(v);
    return rasterizer_shade(
        user_ptr,
        rasterizer_decode_texel_2bit(
            texture->data.ptr,
            texture->stride,
            iu, iv));
}
#else
FORCE_INLINE uint8_t rasterizer_get_fragment_color(fix16_t base_u, fix16_t base_v, const texture_t* texture, void* user_ptr) {
    fix16_t u = base_u & texture->scale_u;
    fix16_t v = base_v & texture->scale_v;

    // Get the texel 
    uint16_t iu = (uint16_t) fixp16_to_int_floor(u);
    uint16_t iv = (uint16_t) fixp16_to_int_floor(v);
    return rasterizer_shade(
        user_ptr,
        rasterizer_decode_texel_2bit(
            texture->data.ptr,
            texture->stride,
            iu, iv));
}
#endif

FORCE_INLINE void rasterizer_draw_pixel(fix16_t base_u, fix16_t base_v, uint8_t ix, uint8_t iy, fix16_t iz, const texture_t* texture, rasterizer_context_t* ctx) {
#ifdef FULL_COLOR
    uint8_t p = 0b11000000 | (uint8_t) ((size_t) texture >> 2);
#elif defined(PERSPECTIVE_CORRECT)
    uint8_t p = rasterizer_get_fragment_color(base_u, base_v, iz, texture, ctx->user_ptr);
#else
    uint8_t p = rasterizer_get_fragment_color(base_u, base_v, texture, ctx->user_ptr);
#endif

    // Set the pixel
    rasterizer_set_pixel(ctx->user_ptr, ix, iy, p);
}

FORCE_INLINE void rasterizer_draw_short_span(
    uint8_t ia, uint8_t ib,
    uint8_t iy, fix16_t iz,
    fix16_t* p_z, fix16_t step_z,
    fix16_t* p_base_u, fix16_t step_u,
    fix16_t* p_base_v, fix16_t step_v,
    const texture_t* texture,
    rasterizer_context_t* ctx) {

    ASSERT(ia < ib);

    fix16_t z = *p_z;
    fix16_t base_u = *p_base_u;
    fix16_t base_v = *p_base_v;

    // Iterate over the scanline
    for (uint8_t ix = ia; ix < ib; ++ix) {
#ifdef ENABLE_DEPTH_TEST
        fix16_t oldZ = ctx->depths[ix];
        if (z > 0 && oldZ < z)
        {
#endif
            rasterizer_draw_pixel(base_u, base_v, ix, iy, iz, texture, ctx);
#ifdef ENABLE_DEPTH_TEST
            ctx->depths[ix] = z;
        }
#endif

        z += step_z;
        base_u += step_u;
        base_v += step_v;
    }

    *p_z = z;
    *p_base_u = base_u;
    *p_base_v = base_v;
}

#ifndef FULL_COLOR
FORCE_INLINE void rasterizer_draw_long_span(
    uint8_t ia, uint8_t ib,
    uint8_t iy, fix16_t iz,
    fix16_t* p_z, fix16_t step_z,
    fix16_t* p_base_u, fix16_t step_u,
    fix16_t* p_base_v, fix16_t step_v,
    const texture_t* texture, 
    rasterizer_context_t* ctx) {

    ASSERT(ia < ib);

    fix16_t z = *p_z;
    fix16_t base_u = *p_base_u;
    fix16_t base_v = *p_base_v;

    uint8_t ia4 = (ia + 3) & ~3;
    uint8_t ib4 = ib & ~3;

    if (ia4 > ia) {
        // Draw up to ia4
        for (uint8_t ix = ia; ix < ia4; ++ix) {
#ifdef ENABLE_DEPTH_TEST
            fix16_t oldZ = ctx->depths[ix];
            if (z > 0 && oldZ < z) {
#endif
                rasterizer_draw_pixel(base_u, base_v, ix, iy, iz, texture, ctx);
#ifdef ENABLE_DEPTH_TEST
                ctx->depths[ix] = z;
            }
#endif
            z += step_z;
            base_u += step_u;
            base_v += step_v;
        }
    }

    // Iterate over the scanline
    uint8_t ix4 = ia4 >> 2;
    for (uint8_t cx = ia4; cx < ib4; cx += 4, ++ix4) {
        uint8_t mask = 0;
        uint8_t c = 0;
        uint8_t shift = 6;

        uint8_t ex = cx + 4;
        ASSERT(ex <= ib4);
        ASSERT(ex <= ib);
        
#ifdef PERSPECTIVE_CORRECT
#   ifdef ACCURATE_PERSPECTIVE_CORRECT
        iz = fix16_rcp(z);
#   else
        // Interpolate every 16 pixels
        if ((cx & 15) == 0) {
            iz = fix16_rcp(z);
        }
#   endif
#endif
        for (uint8_t ix = cx; ix < ex; ++ix, shift -= 2) {
#ifdef ENABLE_DEPTH_TEST
            fix16_t oldZ = ctx->depths[ix];
            if (z > 0 && oldZ < z) {
#endif
                // Get the texel 
#if defined(PERSPECTIVE_CORRECT)
                uint8_t p = rasterizer_get_fragment_color(base_u, base_v, iz, texture, ctx->user_ptr);
#else
                uint8_t p = rasterizer_get_fragment_color(base_u, base_v, texture, ctx->user_ptr);
#endif

                // Set the pixel
                c |= p << shift;
                mask |= 3 << shift;

#ifdef ENABLE_DEPTH_TEST
                ctx->depths[ix] = z;
            }
#endif

            z += step_z;
            base_u += step_u;
            base_v += step_v;
        }

        if (mask != 0) {
            rasterizer_set_pixel_4(ctx->user_ptr, ix4, iy, c, mask);
        }
    }
    
    if (ib4 < ib) {
        // Draw up to ia4
        for (uint8_t ix = ib4; ix < ib; ++ix) {
#ifdef ENABLE_DEPTH_TEST
            fix16_t oldZ = ctx->depths[ix];
            if (z > 0 && oldZ < z) {
#endif
                rasterizer_draw_pixel(base_u, base_v, ix, iy, iz, texture, ctx);
#ifdef ENABLE_DEPTH_TEST
                ctx->depths[ix] = z;
            }
#endif

            z += step_z;
            base_u += step_u;
            base_v += step_v;
        }
    }

    *p_z = z;
    *p_base_u = base_u;
    *p_base_v = base_v;
}
#endif

FORCE_INLINE void rasterizer_draw_span_segment(
    uint8_t ia, uint8_t ib,
    uint8_t iy,
    fix16_t az, fix16_t bz,
    fix16_t ua, fix16_t ub,
    fix16_t va, fix16_t vb,
    const texture_t* texture,
    rasterizer_context_t* ctx) {

    // Skip zero size lines
    if (ia == ib) return;

    ASSERT(ia <= ib);
    ASSERT(ia < MAX_VIEWPORT_X);
    ASSERT(iy < MAX_VIEWPORT_Y);
    ASSERT(texture != NULL);

    // Get the interpolants
    uint8_t delta = ib - ia;
    fix16_t g = fixp16_rcp(delta);
    fix16_t step_z = fixp16_mul(bz - az, g);
    fix16_t z = az;

    fix16_t step_u = fixp16_mul(ub - ua, g);
    fix16_t step_v = fixp16_mul(vb - va, g);
    fix16_t base_u = ua;
    fix16_t base_v = va;

#ifdef PERSPECTIVE_CORRECT
    fix16_t iz = fix16_rcp(z);
#else
    fix16_t iz = z;
#endif

#if defined(FULL_COLOR) || defined(DISABLE_LONG_SPAN_OPTIMIZATIONS)
    rasterizer_draw_short_span(
        ia, ib,
        iy, iz,
        &z, step_z,
        &base_u, step_u,
        &base_v, step_v,
        texture,
        ctx);
#else
    if (delta < 4 || (delta == 4 && (ia & 3) != 0)) {
        rasterizer_draw_short_span(
            ia, ib,
            iy, iz,
            &z, step_z,
            &base_u, step_u,
            &base_v, step_v,
            texture,
            ctx);
    } else {
        rasterizer_draw_long_span(
            ia, ib,
            iy, iz,
            &z, step_z,
            &base_u, step_u,
            &base_v, step_v,
            texture,
            ctx);
    }
#endif
}

#define rasterizer_step_edge(edge) \
    edge.x += edge.step_x;\
    edge.z += edge.step_z;\
    edge.u += edge.step_u;\
    edge.v += edge.step_v;

FORCE_INLINE void rasterizer_create_span(rasterizer_span_t* span, rasterizer_stepping_edge_t* edge1, rasterizer_stepping_edge_t* edge2) {
    if (edge1->x > edge2->x) {
        rasterizer_stepping_edge_t* t = edge1;
        edge1 = edge2;
        edge2 = t;
    }

    fix16_t x0 = edge1->x;
    fix16_t x1 = edge2->x;

    ASSERT(x0 >= edge1->min_x && x0 <= edge1->max_x);
    ASSERT(x1 >= edge2->min_x && x1 <= edge2->max_x);

    span->x0 = (uint8_t) fixp16_to_int_floor(x0);
    span->x1 = (uint8_t) fixp16_to_int_floor(x1);
    span->z0 = edge1->z;
    span->u0 = edge1->u;
    span->v0 = edge1->v;
    span->z1 = edge2->z;
    span->u1 = edge2->u;
    span->v1 = edge2->v;
}

#ifndef ENABLE_DEPTH_TEST
#define rasterizer_get_sort_key(span) (((span)->e0.z > (span)->e1.z) ? (span)->e0.z : (span)->e1.z)

FORCE_INLINE rasterizer_stepping_span_t* rasterizer_sort_spans(rasterizer_stepping_span_t* span_list) {
    if (span_list == NULL || span_list->next_span == NULL) {
        return span_list;
    }

    // Create a new span list
    rasterizer_stepping_span_t* new_span_list = span_list;
    rasterizer_stepping_span_t* next_span = span_list->next_span;
    new_span_list->next_span = NULL;

    // Loop over the remaining spans
    for (rasterizer_stepping_span_t* span = next_span; span != NULL; span = next_span) {
        next_span = span->next_span; // cache this off so we don't lose it when disconnecting it from the current list

        fix16_t sort_key = span->sort_key;
        
        // Find where to insert this span
        rasterizer_stepping_span_t* insert_here = new_span_list;

        // At the start?
        if (sort_key <= insert_here->sort_key) {
            span->next_span = insert_here;
            new_span_list = span;
            continue;
        }

        // Elsewhere:
        for (rasterizer_stepping_span_t* comp_span = insert_here->next_span; comp_span != NULL; comp_span = comp_span->next_span) {
            if (comp_span->sort_key >= sort_key) {
                break;
            }
            insert_here = comp_span;
        }

        ASSERT(insert_here->sort_key <= span->sort_key);

        // Insert here
        span->next_span = insert_here->next_span;
        insert_here->next_span = span;
    }

#ifdef RASTERIZER_CHECKS
    fix16_t x = 0;
    for (rasterizer_stepping_span_t* span = next_span; span != NULL; span = span->next_span) {
        ASSERT(x <= span->sort_key);
        x = span->sort_key;
    }
#endif

    return new_span_list;
}
#endif // ENABLE_DEPTH_TEST

#ifdef RASTERIZER_CHECKS
int g_active_span_high_water_mark = 0;
int rasterizer_get_active_stepping_span_high_watermark(void) {
    return g_active_span_high_water_mark;
}
#endif

rasterizer_stepping_span_t* rasterizer_draw_active_spans(rasterizer_context_t* ctx, rasterizer_stepping_span_t* active_span_list, uint8_t y) {
    rasterizer_stepping_span_t* new_active_span_list = NULL;

    // Sort the span list
#ifdef ENABLE_DEPTH_TEST
    rasterizer_stepping_span_t* next_span = active_span_list;
#else
    rasterizer_stepping_span_t* next_span = rasterizer_sort_spans(active_span_list);
#endif

    int span_count = 0;
    uint8_t sl_min = MAX_VIEWPORT_X;
    uint8_t sl_max = 0;

    while (next_span != NULL) {
        rasterizer_stepping_span_t* span = next_span;
        next_span = span->next_span;

        // If the span intersects this scanline, draw it
        if (span->y0 <= y && span->y1 > y) {
            rasterizer_span_t clipped_span;
            rasterizer_create_span(&clipped_span, &span->e0, &span->e1);

            rasterizer_draw_span_segment(
                clipped_span.x0,
                clipped_span.x1,
                y,
                clipped_span.z0, clipped_span.z1,
                clipped_span.u0, clipped_span.u1,
                clipped_span.v0, clipped_span.v1,
                span->texture,
                ctx);

            if (sl_min > clipped_span.x0) sl_min = clipped_span.x0;
            if (sl_max < clipped_span.x1) sl_max = clipped_span.x1;

            rasterizer_step_edge(span->e0);
            rasterizer_step_edge(span->e1);

#ifndef ENABLE_DEPTH_TEST
            // Remember the min-z for when we sort the span next time
            span->sort_key = rasterizer_get_sort_key(span);
#endif

            ++span_count;
        }

        // If this span is still relevant, push it to the new list
        // else, retire it
        if (y < span->y1 - 1) {
            span->next_span = new_active_span_list;
            new_active_span_list = span;
        } else {
            rasterizer_free_stepping_span(span);
        }
    }

#ifdef RASTERIZER_CHECKS
    if (span_count > g_active_span_high_water_mark) {
        g_active_span_high_water_mark = span_count;
    }
#endif

    // Rasterize the samples
#ifdef NO_OVERDRAW
    if (sl_min < sl_max) {
        rasterizer_draw_samples(ctx, sl_min, sl_max, y);
    }
#endif

    return new_active_span_list;
}

FORCE_INLINE void rasterizer_init_stepping_edge(rasterizer_stepping_edge_t* e, rasterizer_stepping_edge_y_t* ys, const vec3_t* a, const vec3_t* b, const vec2_t* uva, const vec2_t* uvb) {

    // Sort by Y
    if (a->y > b->y) {
        const vec3_t* t3 = a;
        a = b;
        b = t3;

        const vec2_t* t2 = uva;
        uva = uvb;
        uvb = t2;
    }

    fix16_t dx = b->x - a->x;
    fix16_t dy = b->y - a->y;

    ys->y0 = a->y;
    ys->y1 = b->y;

    fix16_t step_t = fixp16_rcp(dy >> 16);

#ifdef RASTERIZER_CHECKS
    e->min_x = fix16_min(a->x, b->x);
    e->max_x = fix16_max(b->x, a->x);
#endif

    e->step_x = fixp16_mul(dx, step_t);
    e->x = a->x;
    e->step_z = fixp16_mul(b->z - a->z, step_t);
    e->z = a->z;
    e->step_u = fixp16_mul(uvb->x - uva->x, step_t);
    e->u = uva->x;
    e->step_v = fixp16_mul(uvb->y - uva->y, step_t);
    e->v = uva->y;
}

FORCE_INLINE void rasterizer_init_stepping_span(rasterizer_stepping_span_t* span, const texture_t* texture, const rasterizer_stepping_edge_y_t* ey0, const rasterizer_stepping_edge_y_t* ey1, int16_t start_y) {
    span->y0 = fixp16_to_int_floor(ey1->y0);
    span->y1 = fixp16_to_int_floor(ey1->y1);
    span->texture = texture;

    ASSERT(start_y >= 0);
    ASSERT(span->y0 < span->y1);
    ASSERT(span->y0 >= start_y);

    // Advance the edge to the start position
    rasterizer_advance_stepping_edge(&span->e0, ey0->y0, ey1->y0);

#ifndef ENABLE_DEPTH_TEST
    span->sort_key = rasterizer_get_sort_key(span);
#endif
}

int longest_index(fix16_t ab, fix16_t bc, fix16_t ca) {  
#ifdef PEBBLE
    register int i __asm__("r0");
    __asm__(
      "mov      r4, r0" "\n" // Move ab into r4
      "mov      r0, #0" "\n" // Move literal 0 into r0
      "cmp      r1, r4" "\n" // If bc > r4:
      "itt      gt"     "\n"
      "movgt    r4, r1" "\n" //   move bc into r4
      "movgt    r0, #1" "\n" //   move 1 into r0
      "cmp      r2, r4" "\n" // If ca > r4:
      "itt      gt"     "\n"
      "movgt    r4, r2" "\n" //   move ca into r4
      "movgt    r0, #2" "\n" //   move 2 into r0
      "cmp      r4, #0" "\n" // If the max length is 0:
      "it       eq"     "\n"
      "subeq    r0, #1" "\n" //   set the return code to -1
      : : : "r0", "r4");
    return i;
#else
    int longest_edge_index = 0;
    fix16_t max_length = ab;
    if (bc > max_length) { 
        max_length = bc;
        longest_edge_index = 1;
    }
    if (ca > max_length) {
        max_length = ca;
        longest_edge_index = 2;
    }      
    if (max_length == 0) {
        return -1;
    }
    return longest_edge_index;
#endif
}

FORCE_INLINE rasterizer_stepping_span_t* rasterizer_create_spans_for_triangle(rasterizer_stepping_span_t* span_list, const texture_t* texture, const vec3_t* a, const vec2_t* uva, const vec3_t* b, const vec2_t* uvb, const vec3_t* c, const vec2_t* uvc, int16_t start_y) {

    // Must be within bounds
    ASSERT(a->x >= 0);
    ASSERT(a->y >= 0);
    ASSERT(b->x >= 0);
    ASSERT(b->y >= 0);
    ASSERT(c->x >= 0);
    ASSERT(c->y >= 0);

    // Each point's xy coordinate should have been floored
    ASSERT((a->x & 0xFFFF) == 0);
    ASSERT((a->y & 0xFFFF) == 0);
    ASSERT((b->x & 0xFFFF) == 0);
    ASSERT((b->y & 0xFFFF) == 0);
    ASSERT((c->x & 0xFFFF) == 0);
    ASSERT((c->y & 0xFFFF) == 0);
    
    fix16_t edge_deltas[3] = {
        fix16_abs(b->y - a->y),
        fix16_abs(c->y - b->y),
        fix16_abs(a->y - c->y)
    };

    // Which edge is the longest? We'll be stepping along that edge
    int longest_edge_index = longest_index(
        edge_deltas[0],
        edge_deltas[1],
        edge_deltas[2]);
    if (longest_edge_index == -1)
        return span_list;
    
    struct edge_s {
        const vec3_t* a, *b;
        const vec2_t* uva, *uvb;
    } edges[3] = {
        { a, b, uva, uvb },
        { b, c, uvb, uvc },
        { c, a, uvc, uva }
    };
    
    int ei1 = (longest_edge_index + 1) % 3;
    int ei2 = (longest_edge_index + 2) % 3;
    
    fix16_t ey1_dy = edge_deltas[ei1];
    fix16_t ey2_dy = edge_deltas[ei2];

    rasterizer_stepping_edge_t e0;
    rasterizer_stepping_edge_y_t ey0;
    rasterizer_init_stepping_edge(
        &e0, 
        &ey0, 
        edges[longest_edge_index].a, 
        edges[longest_edge_index].b, 
        edges[longest_edge_index].uva, 
        edges[longest_edge_index].uvb);    

    // Draw the spans
    if (ey1_dy != 0)
    {
        rasterizer_stepping_span_t* span = rasterizer_allocate_stepping_span();
        span->e0 = e0; // copy e0
        
        rasterizer_stepping_edge_y_t ey1;
        rasterizer_init_stepping_edge(
            &span->e1, 
            &ey1, 
            edges[ei1].a, 
            edges[ei1].b, 
            edges[ei1].uva, 
            edges[ei1].uvb);  
        
        rasterizer_init_stepping_span(span, texture, &ey0, &ey1, start_y);
        
        span->next_span = span_list;
        span_list = span;
    }
    
    if (ey2_dy != 0)
    {
        rasterizer_stepping_span_t* span = rasterizer_allocate_stepping_span();
        span->e0 = e0; // copy e0
        
        rasterizer_stepping_edge_y_t ey1;
        rasterizer_init_stepping_edge(
            &span->e1, 
            &ey1, 
            edges[ei2].a, 
            edges[ei2].b, 
            edges[ei2].uva, 
            edges[ei2].uvb);  
        
        rasterizer_init_stepping_span(span, texture, &ey0, &ey1, start_y);
        
        span->next_span = span_list;
        span_list = span;
    }

    return span_list;
}

FORCE_INLINE rasterizer_stepping_span_t* rasterizer_create_face_spans(rasterizer_stepping_span_t* span_list, const viewport_t* viewport, const face_t* face, const vec3_t* points, const vec2_t* texcoords, const texture_t* texture, uint16_t y, uint8_t needs_clip) {
    const vec2_t* c_uva = &texcoords[face->uvs.a];
    const vec2_t* c_uvb = &texcoords[face->uvs.b];
    const vec2_t* c_uvc = &texcoords[face->uvs.c];
    const vec3_t* c_a = &points[face->positions.a];
    const vec3_t* c_b = &points[face->positions.b];
    const vec3_t* c_c = &points[face->positions.c];

    // Do perspective correction on the UVs
#ifdef PERSPECTIVE_CORRECT
    vec2_t uva, uvb, uvc;
    uva.x = fixp16_mul(c_uva->x, c_a->z);
    uva.y = fixp16_mul(c_uva->y, c_a->z);
    uvb.x = fixp16_mul(c_uvb->x, c_b->z);
    uvb.y = fixp16_mul(c_uvb->y, c_b->z);
    uvc.x = fixp16_mul(c_uvc->x, c_c->z);
    uvc.y = fixp16_mul(c_uvc->y, c_c->z);
#else
    vec2_t uva, uvb, uvc;
    uva.x = fixp16_mul(c_uva->x, texture->scale_u);
    uva.y = fixp16_mul(c_uva->y, texture->scale_v);
    uvb.x = fixp16_mul(c_uvb->x, texture->scale_u);
    uvb.y = fixp16_mul(c_uvb->y, texture->scale_v);
    uvc.x = fixp16_mul(c_uvc->x, texture->scale_u);
    uvc.y = fixp16_mul(c_uvc->y, texture->scale_v);
#endif

    if (needs_clip != 0) {
        span_list = rasterizer_clip_spans_for_triangle(
            span_list,
            viewport,
            texture,
            c_a, &uva,
            c_b, &uvb,
            c_c, &uvc,
            y);
    } else {
        span_list = rasterizer_create_spans_for_triangle(
            span_list,
            texture,
            c_a, &uva,
            c_b, &uvb,
            c_c, &uvc,
            y);
    }

    return span_list;
}

FORCE_INLINE void rasterizer_advance_stepping_edge(rasterizer_stepping_edge_t* e, fix16_t y0, fix16_t y) {
    fix16_t my = y - y0;
    if (my == 0)
        return;

    e->x += fixp16_mul(e->step_x, my);
    e->z += fixp16_mul(e->step_z, my);
    e->u += fixp16_mul(e->step_u, my);
    e->v += fixp16_mul(e->step_v, my);
}

size_t rasterizer_create_face_kickoffs(rasterizer_face_kickoff_t* kickoffs, size_t max_kickoffs, const viewport_t* viewport, uint32_t hull_index, const vec3_t* points, const face_t* faces, size_t num_faces) {
    rasterizer_face_kickoff_t* current_kickoff = kickoffs;
    rasterizer_face_kickoff_t* end_kickoff = kickoffs + max_kickoffs;

    ASSERT(hull_index < 256);
    ASSERT(num_faces < 256);

    const face_t* face = faces;
    for (size_t i = 0; i < num_faces && current_kickoff < end_kickoff; ++i, ++face) {
        const vec3_t a = points[face->positions.a];
        const vec3_t b = points[face->positions.b];
        const vec3_t c = points[face->positions.c];

        // Cross product gives us z direction: if it's facing away from us, ignore it
        fix16_t ux = b.x - a.x;
        fix16_t uy = b.y - a.y;
        fix16_t vx = c.x - a.x;
        fix16_t vy = c.y - a.y;
        if (fixp16_mul(ux, vy) - fixp16_mul(uy, vx) < 0)
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

        int16_t kickoff_y = fixp16_to_int_floor(min_y);
        ASSERT(kickoff_y < 256);

        current_kickoff->hull_index = (uint8_t) hull_index;
        current_kickoff->face_index = (uint8_t) i;
        current_kickoff->y = kickoff_y < 0 ? 0 : (uint8_t) kickoff_y;
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
