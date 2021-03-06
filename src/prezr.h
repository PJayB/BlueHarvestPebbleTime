#pragma once

typedef struct prezr_bitmap_s {
    uint16_t width;
    uint16_t height;
    GBitmap* bitmap;
} prezr_bitmap_t;

typedef struct prezr_pack_s {
    struct prezr_pack_header_s* header;
    uint32_t numResources;
    prezr_bitmap_t* resources;
} prezr_pack_t;

#define PREZR_OK 0
#define PREZR_RESOURCE_LOAD_FAIL -1
#define PREZR_VERSION_FAIL -2
#define PREZR_OUT_OF_MEMORY -3
#define PREZR_ZERO_SIZE_BLOB -4
#define PREZR_CONTAINER_TOO_SMALL -5

void prezr_zero(prezr_pack_t* pack);
int prezr_placement_init(prezr_pack_t* pack, uint32_t h, void* container, size_t container_size);
void prezr_placement_destroy(prezr_pack_t* pack);
int prezr_init(prezr_pack_t* pack, uint32_t h);
void prezr_destroy(prezr_pack_t* pack);

