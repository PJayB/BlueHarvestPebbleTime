#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _face_kickoff {
    uint8_t y;
    uint8_t hull_index;
    uint8_t face_index;
} face_kickoff;

void sort_face_kickoffs(face_kickoff* faces, size_t num_faces);

#ifdef __cplusplus
}
#endif
