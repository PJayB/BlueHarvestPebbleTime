#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _face_sort_key {
    uint8_t y;
    uint8_t hull_index;
    uint8_t face_index;
} face_sort_key;

void face_sort(face_sort_key* faces, size_t num_faces);

#ifdef __cplusplus
}
#endif
