#include "common.h"
#include "matrix.h"

void matrix_create_rotation_z(matrix_t* M, fix16_t angle) {
#ifdef PEBBLE
    fix16_t sa = sin_lookup(angle & 0xFFFF);
    fix16_t ca = cos_lookup(angle & 0xFFFF);
#else
    fix16_t sa = fix16_sin(angle);
    fix16_t ca = fix16_cos(angle);
#endif

    M->m[0][0] = ca;
    M->m[0][1] = sa;
    M->m[0][2] = 0;
    M->m[0][3] = 0;

    M->m[1][0] = -sa;
    M->m[1][1] = ca;
    M->m[1][2] = 0;
    M->m[1][3] = 0;

    M->m[2][0] = 0;
    M->m[2][1] = 0;
    M->m[2][2] = fix16_one;
    M->m[2][3] = 0;

    M->m[3][0] = 0;
    M->m[3][1] = 0;
    M->m[3][2] = 0;
    M->m[3][3] = fix16_one;
}

void matrix_multiply(matrix_t* out, const matrix_t* a, const matrix_t* b) {
    ASSERT((size_t)out != (size_t)a);
    ASSERT((size_t)out != (size_t)b);
    fix16_t x = a->m[0][0];
    fix16_t y = a->m[0][1];
    fix16_t z = a->m[0][2];
    fix16_t w = a->m[0][3];
    out->m[0][0] = fix16_add(fix16_add(fix16_mul(b->m[0][0], x), fix16_mul(b->m[1][0], y)), fix16_add(fix16_mul(b->m[2][0], z), fix16_mul(b->m[3][0], w)));
    out->m[0][1] = fix16_add(fix16_add(fix16_mul(b->m[0][1], x), fix16_mul(b->m[1][1], y)), fix16_add(fix16_mul(b->m[2][1], z), fix16_mul(b->m[3][1], w)));
    out->m[0][2] = fix16_add(fix16_add(fix16_mul(b->m[0][2], x), fix16_mul(b->m[1][2], y)), fix16_add(fix16_mul(b->m[2][2], z), fix16_mul(b->m[3][2], w)));
    out->m[0][3] = fix16_add(fix16_add(fix16_mul(b->m[0][3], x), fix16_mul(b->m[1][3], y)), fix16_add(fix16_mul(b->m[2][3], z), fix16_mul(b->m[3][3], w)));
    x = a->m[1][0];
    y = a->m[1][1];
    z = a->m[1][2];
    w = a->m[1][3];
    out->m[1][0] = fix16_add(fix16_add(fix16_mul(b->m[0][0], x), fix16_mul(b->m[1][0], y)), fix16_add(fix16_mul(b->m[2][0], z), fix16_mul(b->m[3][0], w)));
    out->m[1][1] = fix16_add(fix16_add(fix16_mul(b->m[0][1], x), fix16_mul(b->m[1][1], y)), fix16_add(fix16_mul(b->m[2][1], z), fix16_mul(b->m[3][1], w)));
    out->m[1][2] = fix16_add(fix16_add(fix16_mul(b->m[0][2], x), fix16_mul(b->m[1][2], y)), fix16_add(fix16_mul(b->m[2][2], z), fix16_mul(b->m[3][2], w)));
    out->m[1][3] = fix16_add(fix16_add(fix16_mul(b->m[0][3], x), fix16_mul(b->m[1][3], y)), fix16_add(fix16_mul(b->m[2][3], z), fix16_mul(b->m[3][3], w)));
    x = a->m[2][0];
    y = a->m[2][1];
    z = a->m[2][2];
    w = a->m[2][3];
    out->m[2][0] = fix16_add(fix16_add(fix16_mul(b->m[0][0], x), fix16_mul(b->m[1][0], y)), fix16_add(fix16_mul(b->m[2][0], z), fix16_mul(b->m[3][0], w)));
    out->m[2][1] = fix16_add(fix16_add(fix16_mul(b->m[0][1], x), fix16_mul(b->m[1][1], y)), fix16_add(fix16_mul(b->m[2][1], z), fix16_mul(b->m[3][1], w)));
    out->m[2][2] = fix16_add(fix16_add(fix16_mul(b->m[0][2], x), fix16_mul(b->m[1][2], y)), fix16_add(fix16_mul(b->m[2][2], z), fix16_mul(b->m[3][2], w)));
    out->m[2][3] = fix16_add(fix16_add(fix16_mul(b->m[0][3], x), fix16_mul(b->m[1][3], y)), fix16_add(fix16_mul(b->m[2][3], z), fix16_mul(b->m[3][3], w)));
    x = a->m[3][0];
    y = a->m[3][1];
    z = a->m[3][2];
    w = a->m[3][3];
    out->m[3][0] = fix16_add(fix16_add(fix16_mul(b->m[0][0], x), fix16_mul(b->m[1][0], y)), fix16_add(fix16_mul(b->m[2][0], z), fix16_mul(b->m[3][0], w)));
    out->m[3][1] = fix16_add(fix16_add(fix16_mul(b->m[0][1], x), fix16_mul(b->m[1][1], y)), fix16_add(fix16_mul(b->m[2][1], z), fix16_mul(b->m[3][1], w)));
    out->m[3][2] = fix16_add(fix16_add(fix16_mul(b->m[0][2], x), fix16_mul(b->m[1][2], y)), fix16_add(fix16_mul(b->m[2][2], z), fix16_mul(b->m[3][2], w)));
    out->m[3][3] = fix16_add(fix16_add(fix16_mul(b->m[0][3], x), fix16_mul(b->m[1][3], y)), fix16_add(fix16_mul(b->m[2][3], z), fix16_mul(b->m[3][3], w)));
}

void matrix_vector_transform(vec3_t* v_out, const vec3_t* V, const matrix_t* M) {
    //fW = (M->m[0][3] * V->x) + (M->m[1][3] * V->y) + (M->m[2][3] * V->z) + M->m[3][3];
    fix16_t fW = 
        fix16_add(fix16_mul(M->m[0][3], V->x),
            fix16_add(fix16_mul(M->m[1][3], V->y), 
                fix16_add(fix16_mul(M->m[2][3], V->z),
                    M->m[3][3])));

#ifndef FIXMATH_NO_OVERFLOW
    ASSERT(fW != fix16_overflow);
#endif

    if (fW == 0)
    {
        v_out->x = v_out->y = v_out->z = 0;
        return;
    }

    // Invert W
    fix16_t iW = fix16_div(fix16_one, fW); 

    //Fix16 fX = (M->M->m[0][0] * V->x) / fW + (M->M->m[1][0] * V->y) / fW + (M->M->m[2][0] * V->z) / fW + M->M->m[3][0] / fW;
    fix16_t fX0 = fix16_mul(fix16_mul(M->m[0][0], V->x), iW);
    fix16_t fX1 = fix16_mul(fix16_mul(M->m[1][0], V->y), iW);
    fix16_t fX2 = fix16_mul(fix16_mul(M->m[2][0], V->z), iW);
    fix16_t fX3 = fix16_mul(M->m[3][0], iW);
    fix16_t fX  = fix16_add(fix16_add(fX0, fX1), fix16_add(fX2, fX3));

    //Fix16 fY = (M->M->m[0][1] * V->x) / fW + (M->M->m[1][1] * V->y) / fW + (M->M->m[2][1] * V->z) / fW + M->M->m[3][1] / fW;
    fix16_t fY0 = fix16_mul(fix16_mul(M->m[0][1], V->x), iW);
    fix16_t fY1 = fix16_mul(fix16_mul(M->m[1][1], V->y), iW);
    fix16_t fY2 = fix16_mul(fix16_mul(M->m[2][1], V->z), iW);
    fix16_t fY3 = fix16_mul(M->m[3][1], iW);
    fix16_t fY  = fix16_add(fix16_add(fY0, fY1), fix16_add(fY2, fY3));

    //Fix16 fZ = (M->M->m[0][2] * V->x) + (M->M->m[1][2] * V->y) + (M->M->m[2][2] * V->z) + M->M->m[3][2];
    fix16_t fZ0 = fix16_mul(M->m[0][2], V->x);
    fix16_t fZ1 = fix16_mul(M->m[1][2], V->y);
    fix16_t fZ2 = fix16_mul(M->m[2][2], V->z);
    fix16_t fZ3 = M->m[3][2];
    fix16_t fZ  = fix16_add(fix16_add(fZ0, fZ1), fix16_add(fZ2, fZ3));

#ifndef FIXMATH_NO_OVERFLOW
    ASSERT(fX != fix16_overflow);
    ASSERT(fY != fix16_overflow);
    ASSERT(fZ != fix16_overflow);
#endif

    v_out->x = fX;
    v_out->y = fY;
    v_out->z = fix16_div(fW, fZ); // Invert Z
}

