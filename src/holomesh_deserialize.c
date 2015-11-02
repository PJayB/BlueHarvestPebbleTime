#include "common.h"
#include "holomesh.h"

#define HOLOMESH_OFFSET_ARRAY(dst, offset) (dst).offset += offset;

holomesh_result holomesh_deserialize(holomesh* mesh, uint32_t dataSize) {
    if (mesh == NULL) return hmresult_bad_parameter;
    if (mesh->magic != HOLOMESH_MAGIC) return hmresult_bad_magic;
    if (mesh->file_size > dataSize) return hmresult_buffer_too_small;

    uint32_t offset = (uint32_t) mesh;

    HOLOMESH_OFFSET_ARRAY(mesh->hulls, offset);
    HOLOMESH_OFFSET_ARRAY(mesh->tags, offset);
    HOLOMESH_OFFSET_ARRAY(mesh->tag_groups, offset);
    HOLOMESH_OFFSET_ARRAY(mesh->textures, offset);
    HOLOMESH_OFFSET_ARRAY(mesh->string_table, offset);

    for (uint32_t i = 0; i < mesh->hulls.size; ++i) {
        HOLOMESH_OFFSET_ARRAY(mesh->hulls.ptr[i].vertices, offset);
        HOLOMESH_OFFSET_ARRAY(mesh->hulls.ptr[i].uvs, offset);
        HOLOMESH_OFFSET_ARRAY(mesh->hulls.ptr[i].edges, offset);
        HOLOMESH_OFFSET_ARRAY(mesh->hulls.ptr[i].faces, offset);
    }
    for (uint32_t i = 0; i < mesh->textures.size; ++i) {
        HOLOMESH_OFFSET_ARRAY(mesh->textures.ptr[i].data, offset);
    }
    for (uint32_t i = 0; i < mesh->string_table.size; ++i) {
        HOLOMESH_OFFSET_ARRAY(mesh->string_table.ptr[i].str, offset);
    }
    for (uint32_t i = 0; i < mesh->tag_groups.size; ++i) {
        HOLOMESH_OFFSET_ARRAY(mesh->tag_groups.ptr[i].points, offset);
    }

    return hmresult_ok;
}
