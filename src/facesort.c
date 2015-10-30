#include "common.h"
#include "facesort.h"

void swap_face_keys(face_sort_key* a, face_sort_key* b) {
    face_sort_key t = *a;
    *a = *b;
    *b = t;
}

int partition(face_sort_key* faces, int low, int high) {
    face_sort_key* pivot = &faces[high];
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

void quicksort(face_sort_key* faces, int low, int high) {
    if (low < high) {
        int pivot = partition(faces, low, high);
        quicksort(faces, low, pivot - 1);
        quicksort(faces, pivot + 1, high);
    }
}

void face_sort(face_sort_key* faces, size_t num_faces) {
    if (num_faces > 1) {
        quicksort(faces, 0, (int) num_faces - 1);
    }
}
