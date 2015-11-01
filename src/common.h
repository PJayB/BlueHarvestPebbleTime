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

typedef struct _matrix {
    fix16_t m[4][4];
} matrix;

typedef struct _vec3 {
    union {
        fix16_t v[3];
        struct {
            fix16_t x, y, z;
        };
    };
} vec3;

typedef struct _vec2 {
    union {
        fix16_t v[2];
        struct {
            fix16_t x, y;
        };
    };
} vec2;

typedef struct _holomesh holomesh;

typedef struct _viewport {
    int width, height;
} viewport;

void transform_points(
    vec3* out_points,
    const matrix transform,
    const vec3* in_points,
    size_t num_points);

#ifdef __cplusplus
}
#endif
