#include "common.h"
#include "rasterizer.h"

typedef struct clip_point_s {
    vec3_t p;
    vec2_t t;
} clip_point_t;

typedef struct clip_plane_s {
    fix16_t nx, ny, d;
    fix16_t nxd, nyd;
} clip_plane_t;

fix16_t get_signed_distance_to_plane(const clip_point_t* v, const clip_plane_t* p) {
    fix16_t dx = fix16_sub(p->nxd, v->p.x);
    fix16_t dy = fix16_sub(p->nyd, v->p.y);
    return fix16_add(fix16_mul(dx, p->nx), fix16_mul(dy, p->ny));
}

void emit_intersection(clip_point_t* outputs, int* num_outputs, const clip_point_t* a, const clip_point_t* b, const clip_plane_t* p, fix16_t d) {
    fix16_t edx = fix16_sub(b->p.x, a->p.x);
    fix16_t edy = fix16_sub(b->p.y, a->p.y);
    fix16_t eDotN = fix16_add(fix16_mul(edx, p->nx), fix16_mul(edy, p->ny));
    if (eDotN == 0)
        return;

    fix16_t t = fix16_div(d, eDotN);
    if (t >= 0 && t <= fix16_one) {
        vec3_t ip;
        vec2_t it;

        // We already have deltas for this, so just scale
        ip.x = fix16_round(fix16_add(a->p.x, fix16_mul(edx, t)));
        ip.y = fix16_round(fix16_add(a->p.y, fix16_mul(edy, t)));

        // Interpolate the other parameters
        ip.z = fix16_lerp16(a->p.z, b->p.z, (uint16_t) t);
        it.x = fix16_lerp16(a->t.x, b->t.x, (uint16_t) t);
        it.y = fix16_lerp16(a->t.y, b->t.y, (uint16_t) t);

        outputs[*num_outputs].p = ip;
        outputs[*num_outputs].t = it;
        (*num_outputs)++;
    }
}

const clip_point_t* get_clip_point(const clip_point_t* arr, int num, int i) {
    int j = (i < 0) ? num + i : (i >= num) ? i - num : i;
    ASSERT(j >= 0);
    ASSERT(j < num);
    return &arr[j];
}

rasterizer_stepping_span_t* rasterizer_clip_spans_for_triangle(rasterizer_stepping_span_t* span_list, const viewport_t* viewport, const texture_t* texture, const vec3_t* a, const vec2_t* uva, const vec3_t* b, const vec2_t* uvb, const vec3_t* c, const vec2_t* uvc, int16_t start_y) {
    clip_point_t scratch1[7] =
    {
        {*a, *uva},
        {*b, *uvb},
        {*c, *uvc}
    };
    clip_point_t scratch2[7];
    clip_point_t* inputs = scratch1;
    clip_point_t* outputs = scratch2;
    int num_inputs = 3, num_outputs = 0;

    fix16_t right = viewport->fwidth;
    fix16_t bottom = viewport->fheight;

    // No need to clip the bottom plane -- we handle that by just terminating the scanline rasterization
    const clip_plane_t c_cuttingPlanes[] =
    {
        {fix16_one, 0, 0, fix16_one, 0},
        {-fix16_one, 0, -right, right, 0},
        {0, fix16_one, 0, 0, fix16_one},
        {0, -fix16_one, -bottom, 0, bottom}
    };

    static const size_t c_numCuttingPlanes = 4;

    for (size_t planei = 0; planei < c_numCuttingPlanes; ++planei) {
        const clip_plane_t* plane = &c_cuttingPlanes[planei];
        num_outputs = 0;

        for (int i = 0; i < num_inputs; ++i) {
            const clip_point_t* p = &inputs[i];
            fix16_t d = get_signed_distance_to_plane(p, plane);
            if (d > 0) {
                // The point is outside
                const clip_point_t* prev = get_clip_point(inputs, num_inputs, i - 1);
                const clip_point_t* next = get_clip_point(inputs, num_inputs, i + 1);
                emit_intersection(outputs, &num_outputs, p, prev, plane, d);
                emit_intersection(outputs, &num_outputs, p, next, plane, d);
            } else {
                // Point is inside, so just copy
                outputs[num_outputs++] = *p;
            }
        }

        ASSERT(num_outputs <= 7);

        // Swap around ready for the next cut
        clip_point_t* tmp_points = outputs;
        outputs = inputs;
        inputs = tmp_points;
        num_inputs = num_outputs;
    }

    size_t num_generated_faces = 0;

    // If there are points, draw the faces
    const clip_point_t* clip_a = &inputs[0];
    for (int i = 1; i < num_inputs; ++i) {
        const clip_point_t* clip_b = &inputs[i];
        const clip_point_t* clip_c = &inputs[(i + 1) % num_inputs];

        fix16_t min_y, max_y, min_x, max_x;
        min_x = fix16_min(clip_a->p.x, fix16_min(clip_b->p.x, clip_c->p.x));
        max_x = fix16_max(clip_a->p.x, fix16_max(clip_b->p.x, clip_c->p.x));
        min_y = fix16_min(clip_a->p.y, fix16_min(clip_b->p.y, clip_c->p.y));
        max_y = fix16_max(clip_a->p.y, fix16_max(clip_b->p.y, clip_c->p.y));

        // Cull zero size
        if (min_x == max_x || min_y == max_y)
            continue;

        // Cull out of bounds
        if (max_x < 0)
            continue;
        if (min_x >= right)
            continue;
        if (max_y < 0)
            continue;
        if (min_y >= bottom)
            continue;

        ASSERT(clip_a->p.x >= 0);
        ASSERT(clip_a->p.y >= 0);
        ASSERT(clip_b->p.x >= 0);
        ASSERT(clip_b->p.y >= 0);
        ASSERT(clip_c->p.x >= 0);
        ASSERT(clip_c->p.y >= 0);
        ASSERT(clip_a->p.x <= right);
        ASSERT(clip_a->p.y <= bottom);
        ASSERT(clip_b->p.x <= right);
        ASSERT(clip_b->p.y <= bottom);
        ASSERT(clip_c->p.x <= right);
        ASSERT(clip_c->p.y <= bottom);

        span_list = rasterizer_create_spans_for_triangle(
            span_list,
            texture,
            &clip_a->p, &clip_a->t,
            &clip_b->p, &clip_b->t,
            &clip_c->p, &clip_c->t,
            start_y);

        num_generated_faces++;
    }

    ASSERT(num_generated_faces < 5);

    return span_list;
}

