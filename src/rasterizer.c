#include "common.h"
#include "rasterizer.h"
#include "holomesh.h"

void rasterizer_draw_span(
    rasterizer_context* ctx,
    const texture_t* texture,
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

void rasterizer_draw_span_between_edges(rasterizer_context* ctx, const texture_t* texture, rasterizer_stepping_edge* edge1, rasterizer_stepping_edge* edge2, int16_t iy) {
    if (edge1->x > edge2->x) {
        rasterizer_stepping_edge* t = edge1;
        edge1 = edge2;
        edge2 = t;
    }

    fix16_t x0 = edge1->x;
    fix16_t x1 = edge2->x;

    ASSERT(x0 >= edge1->min_x && x0 <= edge1->max_x);
    ASSERT(x1 >= edge2->min_x && x1 <= edge2->max_x);

    rasterizer_draw_span(
        ctx,
        texture,
        fix16_to_int_floor(x0),
        fix16_to_int_floor(x1),
        iy,
        edge1->z, edge2->z,
        edge1->u, edge2->u,
        edge1->v, edge2->v);

    edge1->x += edge1->step_x;
    edge2->x += edge2->step_x;
    edge1->z += edge1->step_z;
    edge2->z += edge2->step_z;
    edge1->u += edge1->step_u;
    edge2->u += edge2->step_u;
    edge1->v += edge1->step_v;
    edge2->v += edge2->step_v;
}

rasterizer_stepping_span* rasterizer_draw_active_spans(rasterizer_context* ctx, rasterizer_stepping_span* active_span_list, uint16_t y) {
    rasterizer_stepping_span* new_active_span_list = NULL;
    rasterizer_stepping_span* next_span = active_span_list;
    while (next_span != NULL) {
        rasterizer_stepping_span* span = next_span;
        next_span = span->next_span;

        // If the span intersects this scanline, draw it
        if (span->y0 <= y && span->y1 > y) {
            const texture_t* texture = span->texture;
            rasterizer_draw_span_between_edges(
                ctx,
                texture,
                &span->e0,
                &span->e1,
                y);
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
    return new_active_span_list;
}

void rasterizer_init_stepping_edge(rasterizer_stepping_edge* e, rasterizer_stepping_edge_y* ys, const vec3_t* a, const vec3_t* b, const vec2_t* uva, const vec2_t* uvb) {

    // Sort by Y
    if (a->y > b->y) {
        const vec3_t* t3 = a;
        a = b;
        b = t3;

        const vec2_t* t2 = uva;
        uva = uvb;
        uvb = t2;
    }

    ys->y0 = a->y;
    ys->y1 = b->y;
    ys->d = fix16_sub(b->y, a->y);

    fix16_t dx = fix16_sub(b->x, a->x);
    fix16_t step_t = fix16_rcp(ys->d);

#ifdef RASTERIZER_CHECKS
    e->min_x = fix16_min(a->x, b->x);
    e->max_x = fix16_max(b->x, a->x);
#endif

    e->step_x = fix16_mul(dx, step_t);
    e->x = a->x;
    e->step_z = fix16_mul(fix16_sub(b->z, a->z), step_t);
    e->z = a->z;
    e->step_u = fix16_mul(fix16_sub(uvb->x, uva->x), step_t);
    e->u = uva->x;
    e->step_v = fix16_mul(fix16_sub(uvb->y, uva->y), step_t);
    e->v = uva->y;
}

rasterizer_stepping_span* rasterizer_create_stepping_span(rasterizer_stepping_span* span_list, const texture_t* texture, rasterizer_stepping_edge* e0, const rasterizer_stepping_edge_y* ey0, rasterizer_stepping_edge* e1, const rasterizer_stepping_edge_y* ey1, int16_t start_y) {
    rasterizer_stepping_span* span = rasterizer_allocate_stepping_span();
    span->e0 = *e0;
    span->e1 = *e1;
    span->y0 = fix16_to_int_floor(ey1->y0);
    span->y1 = fix16_to_int_floor(ey1->y1);
    span->texture = texture;
    span->next_span = span_list;

    ASSERT(start_y >= 0);
    ASSERT(span->y0 < span->y1);
    ASSERT(span->y0 >= start_y);

    rasterizer_advance_stepping_edge(&span->e0, ey0->y0, ey1->y0);

    return span;
}

rasterizer_stepping_span* rasterizer_create_spans_for_triangle(rasterizer_stepping_span* span_list, const texture_t* texture, const vec3_t* a, const vec2_t* uva, const vec3_t* b, const vec2_t* uvb, const vec3_t* c, const vec2_t* uvc, int16_t start_y) {

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

    rasterizer_stepping_edge edges[3];
    rasterizer_stepping_edge_y edgeYs[3];
    rasterizer_init_stepping_edge(&edges[0], &edgeYs[0], a, b, uva, uvb);
    rasterizer_init_stepping_edge(&edges[1], &edgeYs[1], b, c, uvb, uvc);
    rasterizer_init_stepping_edge(&edges[2], &edgeYs[2], a, c, uva, uvc);

    // Which edge is the longest? We'll be stepping along that edge
    int longest_edge_index = 0;
    fix16_t max_length = 0;
    for (int i = 0; i < 3; ++i)
    {
        if (edgeYs[i].d > max_length)
        {
            max_length = edgeYs[i].d;
            longest_edge_index = i;
        }
    }

    if (max_length == 0)
        return span_list;

    rasterizer_stepping_edge* e0 = &edges[longest_edge_index];
    rasterizer_stepping_edge* e1 = &edges[(longest_edge_index + 1) % 3];
    rasterizer_stepping_edge* e2 = &edges[(longest_edge_index + 2) % 3];

    rasterizer_stepping_edge_y* ey0 = &edgeYs[longest_edge_index];
    rasterizer_stepping_edge_y* ey1 = &edgeYs[(longest_edge_index + 1) % 3];
    rasterizer_stepping_edge_y* ey2 = &edgeYs[(longest_edge_index + 2) % 3];

    // Draw the spans
    if (ey1->d)
    {
        span_list = rasterizer_create_stepping_span(span_list, texture, e0, ey0, e1, ey1, start_y);
    }

    if (ey2->d)
    {
        span_list = rasterizer_create_stepping_span(span_list, texture, e0, ey0, e2, ey2, start_y);
    }

    return span_list;
}

rasterizer_stepping_span* rasterizer_create_face_spans(rasterizer_stepping_span* span_list, const viewport_t* viewport, const face_t* face, const vec3_t* points, const vec2_t* texcoords, const texture_t* texture, uint16_t y, uint8_t needs_clip) {
    vec2_t uva = texcoords[face->uvs.a];
    vec2_t uvb = texcoords[face->uvs.b];
    vec2_t uvc = texcoords[face->uvs.c];
    vec3_t a = points[face->positions.a];
    vec3_t b = points[face->positions.b];
    vec3_t c = points[face->positions.c];

    // Do perspective correction on the UVs
    uva.x = fix16_mul(uva.x, a.z);
    uva.y = fix16_mul(uva.y, a.z);
    uvb.x = fix16_mul(uvb.x, b.z);
    uvb.y = fix16_mul(uvb.y, b.z);
    uvc.x = fix16_mul(uvc.x, c.z);
    uvc.y = fix16_mul(uvc.y, c.z);

    if (needs_clip != 0) {
        span_list = rasterizer_clip_spans_for_triangle(
            span_list,
            viewport,
            texture,
            &a, &uva,
            &b, &uvb,
            &c, &uvc,
            y);
    } else {
        span_list = rasterizer_create_spans_for_triangle(
            span_list,
            texture,
            &a, &uva,
            &b, &uvb,
            &c, &uvc,
            y);
    }

    return span_list;
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

size_t rasterizer_create_face_kickoffs(rasterizer_face_kickoff* kickoffs, size_t max_kickoffs, const viewport_t* viewport, uint32_t hull_index, const vec3_t* points, const face_t* faces, size_t num_faces) {
    rasterizer_face_kickoff* current_kickoff = kickoffs;
    rasterizer_face_kickoff* end_kickoff = kickoffs + max_kickoffs;

    ASSERT(hull_index < 256);
    ASSERT(num_faces < 256);

    const face_t* face = faces;
    for (size_t i = 0; i < num_faces && current_kickoff < end_kickoff; ++i, ++face) {
        const vec3_t a = points[face->positions.a];
        const vec3_t b = points[face->positions.b];
        const vec3_t c = points[face->positions.c];

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
