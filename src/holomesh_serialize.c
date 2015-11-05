#include "common.h"
#include "holomesh.h"

#if defined(WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#   define ASSERT(x) if (x) {} else { DebugBreak(); }

#   include <memory.h>
#   include <string.h>

#define HOLOMESH_ALIGN_POINTER(x) ASSERT(((size_t)ptr & 3) == 0)
#define HOLOMESH_ARRAY_SIZE(x) (((sizeof(*(x).ptr) * (x).size) + 3) & ~3)
#define HOLOMESH_COPY_ARRAY(dest, src) { \
    memcpy((dest).ptr, (src).ptr, HOLOMESH_ARRAY_SIZE(src)); \
    (dest).size = (src).size; }
#define HOLOMESH_SET_AND_COPY_ARRAY(dest, ptr, src) { \
    HOLOMESH_ALIGN_POINTER(ptr); \
    if (src.ptr == NULL) { dest.offset = 0; dest.size = 0; } else { \
    dest.offset = (uint32_t) ptr; \
    HOLOMESH_COPY_ARRAY(dest, src); \
    ptr += HOLOMESH_ARRAY_SIZE(src); } }
#define HOLOMESH_OFFSET_ARRAY(dst, offset) (dst).offset -= offset;

void holomesh_offset_pointers(holomesh* mesh, int32_t offset);

uint32_t holomesh_get_size(const holomesh* sourceMesh, int include_scratch) {
    // First tier size
    uint32_t size = sizeof(holomesh);

    // Get second tier sizes
    size += HOLOMESH_ARRAY_SIZE(sourceMesh->hulls);
    size += HOLOMESH_ARRAY_SIZE(sourceMesh->tags);
    size += HOLOMESH_ARRAY_SIZE(sourceMesh->tag_groups);
    size += HOLOMESH_ARRAY_SIZE(sourceMesh->textures);
    size += HOLOMESH_ARRAY_SIZE(sourceMesh->string_table);

    // Third tier sizes
    for (uint32_t i = 0; i < sourceMesh->hulls.size; ++i) {
        size += HOLOMESH_ARRAY_SIZE(sourceMesh->hulls.ptr[i].vertices);
        size += HOLOMESH_ARRAY_SIZE(sourceMesh->hulls.ptr[i].uvs);
        size += HOLOMESH_ARRAY_SIZE(sourceMesh->hulls.ptr[i].edges);
        size += HOLOMESH_ARRAY_SIZE(sourceMesh->hulls.ptr[i].faces);
    }
    for (uint32_t i = 0; i < sourceMesh->textures.size; ++i) {
        size += HOLOMESH_ARRAY_SIZE(sourceMesh->textures.ptr[i].data);
    }
    for (uint32_t i = 0; i < sourceMesh->string_table.size; ++i) {
        size += HOLOMESH_ARRAY_SIZE(sourceMesh->string_table.ptr[i].str);
    }
    for (uint32_t i = 0; i < sourceMesh->tag_groups.size; ++i) {
        size += HOLOMESH_ARRAY_SIZE(sourceMesh->tag_groups.ptr[i].points);
    }

    if (include_scratch) {
        for (uint32_t i = 0; i < sourceMesh->hulls.size; ++i) {
            size += HOLOMESH_ARRAY_SIZE(sourceMesh->hulls.ptr[i].vertices);
        }
    }

    return size;
}

holomesh_result holomesh_serialize(const holomesh* sourceMesh, holomesh* destMesh, uint32_t destMeshSize) {
    uint32_t fileSize = holomesh_get_size(sourceMesh, 0);
    if (fileSize > destMeshSize)
        return hmresult_buffer_too_small;

    uint32_t totalSizeWithScratch = holomesh_get_size(sourceMesh, 1);

    // Set up the header -- shallow copy at first
    *destMesh = *sourceMesh;
    destMesh->magic = HOLOMESH_MAGIC;
    destMesh->version = HOLOMESH_VERSION;
    destMesh->file_size = fileSize;
    destMesh->scratch_size = totalSizeWithScratch - fileSize;

    // Set up the write pointer
    uint8_t* base = ((uint8_t*) destMesh);
    uint8_t* ptr = base + sizeof(holomesh);
    uint8_t* end = base + fileSize;

    // Copy the second tier structures into the mesh
    HOLOMESH_SET_AND_COPY_ARRAY(destMesh->hulls, ptr, sourceMesh->hulls);
    HOLOMESH_SET_AND_COPY_ARRAY(destMesh->tags, ptr, sourceMesh->tags);
    HOLOMESH_SET_AND_COPY_ARRAY(destMesh->tag_groups, ptr, sourceMesh->tag_groups);
    HOLOMESH_SET_AND_COPY_ARRAY(destMesh->textures, ptr, sourceMesh->textures);
    HOLOMESH_SET_AND_COPY_ARRAY(destMesh->string_table, ptr, sourceMesh->string_table);

    // Copy the third tier structures into the mesh
    for (uint32_t i = 0; i < destMesh->hulls.size; ++i) {
        holomesh_hull_t* dst = destMesh->hulls.ptr + i;
        const holomesh_hull_t* src = sourceMesh->hulls.ptr + i;
        HOLOMESH_SET_AND_COPY_ARRAY(dst->vertices, ptr, src->vertices);
        HOLOMESH_SET_AND_COPY_ARRAY(dst->uvs, ptr, src->uvs);
        HOLOMESH_SET_AND_COPY_ARRAY(dst->edges, ptr, src->edges);
        HOLOMESH_SET_AND_COPY_ARRAY(dst->faces, ptr, src->faces);
    }
    for (uint32_t i = 0; i < destMesh->textures.size; ++i) {
        holomesh_texture_t* dst = destMesh->textures.ptr + i;
        const holomesh_texture_t* src = sourceMesh->textures.ptr + i;
        HOLOMESH_SET_AND_COPY_ARRAY(dst->data, ptr, src->data);
    }
    for (uint32_t i = 0; i < destMesh->string_table.size; ++i) {
        holomesh_string_t* dst = destMesh->string_table.ptr + i;
        const holomesh_string_t* src = sourceMesh->string_table.ptr + i;
        HOLOMESH_SET_AND_COPY_ARRAY(dst->str, ptr, src->str);
    }
    for (uint32_t i = 0; i < destMesh->tag_groups.size; ++i) {
        holomesh_tag_group_t* dst = destMesh->tag_groups.ptr + i;
        const holomesh_tag_group_t* src = sourceMesh->tag_groups.ptr + i;
        HOLOMESH_SET_AND_COPY_ARRAY(dst->points, ptr, src->points);
    }

    ASSERT(ptr == end);

    //
    // De-offset
    //
    uint32_t offset = (uint32_t) base;
    for (uint32_t i = 0; i < destMesh->hulls.size; ++i) {
        HOLOMESH_OFFSET_ARRAY(destMesh->hulls.ptr[i].vertices, offset);
        HOLOMESH_OFFSET_ARRAY(destMesh->hulls.ptr[i].uvs, offset);
        HOLOMESH_OFFSET_ARRAY(destMesh->hulls.ptr[i].edges, offset);
        HOLOMESH_OFFSET_ARRAY(destMesh->hulls.ptr[i].faces, offset);
    }
    for (uint32_t i = 0; i < destMesh->textures.size; ++i) {
        HOLOMESH_OFFSET_ARRAY(destMesh->textures.ptr[i].data, offset);
    }
    for (uint32_t i = 0; i < destMesh->string_table.size; ++i) {
        HOLOMESH_OFFSET_ARRAY(destMesh->string_table.ptr[i].str, offset);
    }
    for (uint32_t i = 0; i < destMesh->tag_groups.size; ++i) {
        HOLOMESH_OFFSET_ARRAY(destMesh->tag_groups.ptr[i].points, offset);
    }

    HOLOMESH_OFFSET_ARRAY(destMesh->hulls, offset);
    HOLOMESH_OFFSET_ARRAY(destMesh->tags, offset);
    HOLOMESH_OFFSET_ARRAY(destMesh->tag_groups, offset);
    HOLOMESH_OFFSET_ARRAY(destMesh->textures, offset);
    HOLOMESH_OFFSET_ARRAY(destMesh->string_table, offset);

    return hmresult_ok;
}

void holomesh_set_vec2(holomesh_vec2_t* vec, uint32_t u, uint32_t v) {
    vec->x = u;
    vec->y = v;
}

void holomesh_set_vec3(holomesh_vec3_t* vec, uint32_t x, uint32_t y, uint32_t z) {
    vec->x = x;
    vec->y = y;
    vec->z = z;
}

void holomesh_set_edge(holomesh_edge_t* e, uint8_t a, uint8_t b) {
    e->a = a;
    e->b = b;
}

void holomesh_set_face(holomesh_face_t* f, uint8_t pa, uint8_t pb, uint8_t pc, uint8_t ta, uint8_t tb, uint8_t tc, uint8_t texture) {
    f->positions.a = pa;
    f->positions.b = pb;
    f->positions.c = pc;
    f->uvs.a = ta;
    f->uvs.b = tb;
    f->uvs.c = tc;
    f->texture = texture;
}

void holomesh_set_string(holomesh_string_t* out, char* str) {
    if (str == NULL) {
        out->str.ptr = NULL;
        out->str.size = 0;
    }
    else {
        out->str.ptr = str;
        out->str.size = strlen(str);
    }
}

uint32_t holomesh_texture_data_stride(uint16_t pixelsPerRow) {
    return 4 * ((pixelsPerRow + 15) / 16);
}

uint32_t holomesh_texture_data_size(uint16_t w, uint16_t h) {
    return holomesh_texture_data_stride(w) * h;
}

holomesh_result holomesh_pack_texture(uint8_t* dst, uint32_t dstSize, const uint8_t* texture, uint16_t width, uint16_t height) {
    uint32_t stride = holomesh_texture_data_stride(width);
    ASSERT(stride >= 4);
    ASSERT((stride & 3) == 0);

    uint32_t sizeNeeded = stride * height;
    if (sizeNeeded < dstSize)
        return hmresult_buffer_too_small;

    for (uint32_t y = 0; y < height; ++y) {
        uint8_t* ptr = dst;
        const uint8_t* end = ptr + stride;
        const uint8_t* texPtr = texture;
        for (uint32_t x = 0; x < width; x += 4) {
            for (uint32_t i = 0; i < 8; i += 2) {
                uint8_t src = *texPtr;
                uint16_t r = (src >> 4) & 3;
                uint16_t g = (src >> 2) & 3;
                uint16_t b = (src) & 3;
                uint16_t a = (r + g + b) / 3;
                *ptr |= (a & 0b11) << i;

                // Clamp to width
                if (x + i + 1 < width)
                    texPtr++;
            }
            ptr++;
        }
        ASSERT(ptr <= end);
        dst += stride;
        texture += width;
    }

    return hmresult_ok;
}

holomesh_result holomesh_unpack_texture(uint8_t* dst, uint32_t dstSize, const uint8_t* texture, uint16_t width, uint16_t height) {
    uint32_t sizeNeeded = width * height;
    if (sizeNeeded < dstSize)
        return hmresult_buffer_too_small;

    uint8_t* end = dst + dstSize;

    for (uint32_t y = 0; y < height; ++y) {
        uint8_t* ptr = dst;
        for (uint32_t x = 0; x < width; x += 4) {
            uint8_t src = *texture++;
            for (uint32_t i = 0; (i < 4) && (i + x) < width; ++i, ++ptr, src >>= 2) {
                uint8_t a = src & 3;
                *ptr = 0b11000000 | (a << 4) | (a << 2) | a;
            }
        }
        ASSERT(ptr <= dst + width);
        dst += width;
    }

    ASSERT(dst <= end);

    return hmresult_ok;
}

void holomesh_set_texture(holomesh_texture_t* texture, uint8_t* data, uint32_t dataSize, uint16_t width, uint16_t height) {
    texture->height = height;
    texture->width = width;
    texture->scale_u = fix16_from_int(texture->width - 1);
    texture->scale_v = fix16_from_int(texture->height - 1);
    texture->stride = (uint16_t)holomesh_texture_data_stride(width);
    ASSERT((texture->stride & 3) == 0);
    ASSERT(texture->stride >= 4);
    texture->data.ptr = data;
    texture->data.size = dataSize;
}

void holomesh_set_tag_group(holomesh_tag_group_t* tagGroup, holomesh_vec3_t* points, uint32_t numPoints) {
    if (points == NULL) {
        tagGroup->points.ptr = NULL;
        tagGroup->points.size = 0;
    }
    else {
        tagGroup->points.ptr = points;
        tagGroup->points.size = numPoints;
    }
}

void holomesh_set_transform(holomesh_transform_t* t, const uint32_t m[16]) {
    memcpy(t->m, m, sizeof(uint32_t) * 16);
}

void holomesh_set_hull(
    holomesh_hull_t* hull,
    holomesh_vec3_t* vertices, uint32_t numVertices,
    holomesh_vec2_t* uvs, uint32_t numUVs,
    holomesh_edge_t* edges, uint32_t numEdges,
    holomesh_face_t* faces, uint32_t numFaces) {
    hull->vertices.ptr = vertices; hull->vertices.size = numVertices;
    hull->uvs.ptr = uvs; hull->uvs.size = numUVs;
    hull->edges.ptr = edges; hull->edges.size = numEdges;
    hull->faces.ptr = faces; hull->faces.size = numFaces;
}

void holomesh_set_hulls(holomesh* mesh, holomesh_hull_t* hulls, uint32_t numHulls) {
    if (hulls == NULL) {
        mesh->hulls.ptr = NULL;
        mesh->hulls.size = 0;
    } else {
        mesh->hulls.ptr = hulls;
        mesh->hulls.size = numHulls;
    }
}

void holomesh_set_tags(holomesh* mesh, holomesh_tag_t* tags, uint32_t numTags) {
    if (tags == NULL) {
        mesh->tags.ptr = NULL;
        mesh->tags.size = 0;
    }
    else {
        mesh->tags.ptr = tags;
        mesh->tags.size = numTags;
    }
}

void holomesh_set_tag_groups(holomesh* mesh, holomesh_tag_group_t* tagGroups, uint32_t numTagGroups) {
    if (tagGroups == NULL) {
        mesh->tag_groups.ptr = NULL;
        mesh->tag_groups.size = 0;
    }
    else {
        mesh->tag_groups.ptr = tagGroups;
        mesh->tag_groups.size = numTagGroups;
    }
}

void holomesh_set_textures(holomesh* mesh, holomesh_texture_t* textures, uint32_t numTextures) {
    if (textures == NULL) {
        mesh->textures.ptr = NULL;
        mesh->textures.size = 0;
    }
    else {
        mesh->textures.ptr = textures;
        mesh->textures.size = numTextures;
    }
}

void holomesh_set_strings(holomesh* mesh, holomesh_string_t* strings, uint32_t numStrings) {
    if (strings == NULL) {
        mesh->string_table.ptr = NULL;
        mesh->string_table.size = 0;
    }
    else {
        mesh->string_table.ptr = strings;
        mesh->string_table.size = numStrings;
    }
}

#endif
