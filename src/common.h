#pragma once

#ifndef WIN32
#   include <pebble.h>
// TODO: don't define this for the shipping version!
//#   define RASTERIZER_CHECKS
#   ifdef RASTERIZER_CHECKS
#       define ASSERT(x) if (x) {} else { assert(__FILE__, __LINE__, #x); }
#   else
#       define ASSERT(x) 
#   endif
#   define PEBBLE
#   define PROFILE_NAME(x) __profile_##x
#   define DECLARE_PROFILE(x) extern uint32_t PROFILE_NAME(x)
#   define DEFINE_PROFILE(x) uint32_t PROFILE_NAME(x) = 0
#   define RESET_PROFILE(x) PROFILE_NAME(x) = 0
#   define BEGIN_PROFILE(x) PROFILE_NAME(x) -= get_milliseconds()
#   define END_PROFILE(x) PROFILE_NAME(x) += get_milliseconds()
#   define GET_PROFILE(x) PROFILE_NAME(x)
#   define PRINT_PROFILE(x) APP_LOG(APP_LOG_LEVEL_DEBUG, "PROFILE " #x ": %ums", (unsigned) PROFILE_NAME(x))
#   define FORCE_INLINE inline __attribute__((always_inline))
#else
#   include <stdint.h>
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#   include <stdlib.h>
#   define ASSERT(x) if (x) {} else { DebugBreak(); }
#   define SANDBOX
#   define RASTERIZER_CHECKS
#   pragma warning(disable: 4204)
#   pragma warning(disable: 4214)
#   define PROFILE_NAME(x) 
#   define DECLARE_PROFILE(x) 
#   define DEFINE_PROFILE(x) 
#   define START_PROFILE(x) 
#   define END_PROFILE(x) 
#   define GET_PROFILE(x) 0
#   define FULL_COLOR
//#   define DEBUG_COLOR
#   define FORCE_INLINE 
#endif

#include "libfixmath/fix16.h"

#define ENABLE_DEPTH_TEST

#define fixp16_rcp(y) ((y) == 0 ? 0 : 65536 / (y))
#define fixp16_mul(x, y) ((int32_t)(((int64_t)(x)*(int64_t)(y))>>16))
#define fixp16_ufrac(x) (x & 0x0000FFFFU)
#define fixp16_to_int_floor(x) (x >> 16)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PEBBLE
void assert(const char* file, int line, const char* condition);
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

void viewport_init(viewport_t* viewport, uint16_t width, uint16_t height);

#ifdef PEBBLE
inline uint32_t get_milliseconds(void) {
    time_t s;
    uint16_t ms;
    time_ms(&s, &ms);
    return 1000U * (uint32_t) s + (uint32_t) ms;
}
#endif

#ifdef __cplusplus
}
#endif
