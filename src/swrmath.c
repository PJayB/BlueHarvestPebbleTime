#include "common.h"
#include "swrmath.h"

void matrix_create_rotation_z(matrix m, fix16_t angle) {
    fix16_t sa = fix16_sin(angle);
    fix16_t ca = fix16_cos(angle);

    m[0][0] = ca;
    m[0][1] = sa;
    m[0][2] = 0;
    m[0][3] = 0;

    m[1][0] = -sa;
    m[1][1] = ca;
    m[1][2] = 0;
    m[1][3] = 0;

    m[2][0] = 0;
    m[2][1] = 0;
    m[2][2] = fix16_one;
    m[2][3] = 0;

    m[3][0] = 0;
    m[3][1] = 0;
    m[3][2] = 0;
    m[3][3] = fix16_one;
}

void matrix_multiply(matrix out, const matrix a, const matrix b) {
    ASSERT((size_t)out != (size_t)a);
    ASSERT((size_t)out != (size_t)b);
    fix16_t x = a[0][0];
    fix16_t y = a[0][1];
    fix16_t z = a[0][2];
    fix16_t w = a[0][3];
    out[0][0] = fix16_add(fix16_add(fix16_mul(b[0][0], x), fix16_mul(b[1][0], y)), fix16_add(fix16_mul(b[2][0], z), fix16_mul(b[3][0], w)));
    out[0][1] = fix16_add(fix16_add(fix16_mul(b[0][1], x), fix16_mul(b[1][1], y)), fix16_add(fix16_mul(b[2][1], z), fix16_mul(b[3][1], w)));
    out[0][2] = fix16_add(fix16_add(fix16_mul(b[0][2], x), fix16_mul(b[1][2], y)), fix16_add(fix16_mul(b[2][2], z), fix16_mul(b[3][2], w)));
    out[0][3] = fix16_add(fix16_add(fix16_mul(b[0][3], x), fix16_mul(b[1][3], y)), fix16_add(fix16_mul(b[2][3], z), fix16_mul(b[3][3], w)));
    x = a[1][0];
    y = a[1][1];
    z = a[1][2];
    w = a[1][3];
    out[1][0] = fix16_add(fix16_add(fix16_mul(b[0][0], x), fix16_mul(b[1][0], y)), fix16_add(fix16_mul(b[2][0], z), fix16_mul(b[3][0], w)));
    out[1][1] = fix16_add(fix16_add(fix16_mul(b[0][1], x), fix16_mul(b[1][1], y)), fix16_add(fix16_mul(b[2][1], z), fix16_mul(b[3][1], w)));
    out[1][2] = fix16_add(fix16_add(fix16_mul(b[0][2], x), fix16_mul(b[1][2], y)), fix16_add(fix16_mul(b[2][2], z), fix16_mul(b[3][2], w)));
    out[1][3] = fix16_add(fix16_add(fix16_mul(b[0][3], x), fix16_mul(b[1][3], y)), fix16_add(fix16_mul(b[2][3], z), fix16_mul(b[3][3], w)));
    x = a[2][0];
    y = a[2][1];
    z = a[2][2];
    w = a[2][3];
    out[2][0] = fix16_add(fix16_add(fix16_mul(b[0][0], x), fix16_mul(b[1][0], y)), fix16_add(fix16_mul(b[2][0], z), fix16_mul(b[3][0], w)));
    out[2][1] = fix16_add(fix16_add(fix16_mul(b[0][1], x), fix16_mul(b[1][1], y)), fix16_add(fix16_mul(b[2][1], z), fix16_mul(b[3][1], w)));
    out[2][2] = fix16_add(fix16_add(fix16_mul(b[0][2], x), fix16_mul(b[1][2], y)), fix16_add(fix16_mul(b[2][2], z), fix16_mul(b[3][2], w)));
    out[2][3] = fix16_add(fix16_add(fix16_mul(b[0][3], x), fix16_mul(b[1][3], y)), fix16_add(fix16_mul(b[2][3], z), fix16_mul(b[3][3], w)));
    x = a[3][0];
    y = a[3][1];
    z = a[3][2];
    w = a[3][3];
    out[3][0] = fix16_add(fix16_add(fix16_mul(b[0][0], x), fix16_mul(b[1][0], y)), fix16_add(fix16_mul(b[2][0], z), fix16_mul(b[3][0], w)));
    out[3][1] = fix16_add(fix16_add(fix16_mul(b[0][1], x), fix16_mul(b[1][1], y)), fix16_add(fix16_mul(b[2][1], z), fix16_mul(b[3][1], w)));
    out[3][2] = fix16_add(fix16_add(fix16_mul(b[0][2], x), fix16_mul(b[1][2], y)), fix16_add(fix16_mul(b[2][2], z), fix16_mul(b[3][2], w)));
    out[3][3] = fix16_add(fix16_add(fix16_mul(b[0][3], x), fix16_mul(b[1][3], y)), fix16_add(fix16_mul(b[2][3], z), fix16_mul(b[3][3], w)));
}

void matrix_vector_transform(vec3 v, const matrix m) {
    //fW = (M.m[0][3] * V->x) + (M.m[1][3] * V->y) + (M.m[2][3] * V->z) + M.m[3][3];
    fix16_t fW = 
        fix16_add(fix16_mul(m[0][3], v[0]),
            fix16_add(fix16_mul(m[1][3], v[1]), 
                fix16_add(fix16_mul(m[2][3], v[2]),
                    m[3][3])));

#ifndef FIXMATH_NO_OVERFLOW
    ASSERT(fW != fix16_overflow);
#endif

    if (fW == 0)
    {
        v[0] = v[1] = v[2] = 0;
        return;
    }

    // Invert W
    fix16_t iW = fix16_div(fix16_one, fW); 

    //Fix16 fX = (M.m[0][0] * V->x) / fW + (M.m[1][0] * V->y) / fW + (M.m[2][0] * V->z) / fW + M.m[3][0] / fW;
    fix16_t fX0 = fix16_mul(fix16_mul(m[0][0], v[0]), iW);
    fix16_t fX1 = fix16_mul(fix16_mul(m[1][0], v[1]), iW);
    fix16_t fX2 = fix16_mul(fix16_mul(m[2][0], v[2]), iW);
    fix16_t fX3 = fix16_mul(m[3][0], iW);
    fix16_t fX  = fix16_add(fix16_add(fX0, fX1), fix16_add(fX2, fX3));

    //Fix16 fY = (M.m[0][1] * V->x) / fW + (M.m[1][1] * V->y) / fW + (M.m[2][1] * V->z) / fW + M.m[3][1] / fW;
    fix16_t fY0 = fix16_mul(fix16_mul(m[0][1], v[0]), iW);
    fix16_t fY1 = fix16_mul(fix16_mul(m[1][1], v[1]), iW);
    fix16_t fY2 = fix16_mul(fix16_mul(m[2][1], v[2]), iW);
    fix16_t fY3 = fix16_mul(m[3][1], iW);
    fix16_t fY  = fix16_add(fix16_add(fY0, fY1), fix16_add(fY2, fY3));

    //Fix16 fZ = (M.m[0][2] * V->x) + (M.m[1][2] * V->y) + (M.m[2][2] * V->z) + M.m[3][2];
    fix16_t fZ0 = fix16_mul(m[0][2], v[0]);
    fix16_t fZ1 = fix16_mul(m[1][2], v[1]);
    fix16_t fZ2 = fix16_mul(m[2][2], v[2]);
    fix16_t fZ3 = m[3][2];
    fix16_t fZ  = fix16_add(fix16_add(fZ0, fZ1), fix16_add(fZ2, fZ3));

#ifndef FIXMATH_NO_OVERFLOW
    ASSERT(fX != fix16_overflow);
    ASSERT(fY != fix16_overflow);
    ASSERT(fZ != fix16_overflow);
#endif

    v[0] = fX;
    v[1] = fY;
    v[2] = fix16_div(fW, fZ); // Invert Z
}
