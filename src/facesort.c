#include "common.h"
#include "rasterizer.h"

void swap_face_keys(rasterizer_face_kickoff* a, rasterizer_face_kickoff* b) {
    rasterizer_face_kickoff t = *a;
    *a = *b;
    *b = t;
}

int partition(rasterizer_face_kickoff* faces, int low, int high) {
    rasterizer_face_kickoff* pivot = &faces[high];
    int i = low;
    for (int j = low; j < high; ++j) {
        if (faces[j].y <= pivot->y) {
            swap_face_keys(&faces[i], &faces[j]);
            ++i;
        }
    }
    swap_face_keys(&faces[i], &faces[high]);
    return i;
}

void quicksort(rasterizer_face_kickoff* faces, int low, int high) {
    if (low < high) {
        int pivot = partition(faces, low, high);
        quicksort(faces, low, pivot - 1);
        quicksort(faces, pivot + 1, high);
    }
}

void rasterizer_sort_face_kickoffs(rasterizer_face_kickoff* faces, size_t num_faces) {
#ifndef SANDBOX
    ASSERT(sizeof(rasterizer_face_kickoff) == 4);
#endif
    if (num_faces > 1) {
        quicksort(faces, 0, (int) num_faces - 1);
    }
}
