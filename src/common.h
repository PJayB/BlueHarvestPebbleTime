#pragma once

#ifndef WIN32
#   include <pebble.h>
// TODO: don't define this for the shipping version!
#   define RASTERIZER_CHECKS
#   ifdef RASTERIZER_CHECKS
#       define ASSERT(x) if (x) {} else { APP_LOG(APP_LOG_LEVEL_ERROR, "Assert! " #x); }
#   else
#       define ASSERT(x) 
#   endif
#else
#   include <stdint.h>
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#   define ASSERT(x) if (x) {} else { DebugBreak(); }
#   define SANDBOX
#   define RASTERIZER_CHECKS
#endif

#include "libfixmath/fix16.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct matrix_s {
    fix16_t m[4][4];
} matrix_t;

typedef struct vec3_s {
    union {
        fix16_t v[3];
        struct {
            fix16_t x, y, z;
        };
    };
} vec3_t;

typedef struct vec2_s {
    union {
        fix16_t v[2];
        struct {
            fix16_t x, y;
        };
    };
} vec2_t;

typedef struct edge_s {
    uint8_t a, b;
} edge_t;

typedef struct face_indices_s {
    uint8_t a, b, c;
} face_indices_t;

typedef struct face_s {
    face_indices_t positions;
    face_indices_t uvs;
    uint8_t texture;
} face_t;

typedef struct viewport_s {
    uint16_t width, height;
    fix16_t fwidth, fheight;
} viewport_t;

typedef struct holomesh_s holomesh_t;

void transform_points(
    vec3_t* out_points,
    const matrix_t transform,
    const vec3_t* in_points,
    size_t num_points);

#ifdef __cplusplus
}
#endif
