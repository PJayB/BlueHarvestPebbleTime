#pragma once

#ifndef WIN32
#   include <pebble.h>
#   define ASSERT(x) if (x) {} else { APP_LOG(APP_LOG_LEVEL_ERROR, "Assert! " #x); }
#else
#   include <stdint.h>
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#   define ASSERT(x) if (x) {} else { DebugBreak(); }
#endif

#include "libfixmath/fix16.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef fix16_t matrix[4][4];
typedef fix16_t vec3[3];
typedef fix16_t vec2[2];

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
