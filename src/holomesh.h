#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    /*---------------------------------

    MODEL LOADING

    ---------------------------------*/
#define HOLOMESH_MAGIC 0x484f4c4fu // 'HOLO'
#define HOLOMESH_VERSION 3

#define HOLOMESH_ARRAY_PTR(type) union { type* ptr; int32_t offset; }
#define HOLOMESH_ARRAY(type) struct { HOLOMESH_ARRAY_PTR(type); uint32_t size; }

#define HOLOMESH_BAD_REF 0xFFFF

    typedef enum _holomesh_transform_index {
        holomesh_transform_perspective_pebble_aspect,
        holomesh_transform_perspective_square_aspect,
        holomesh_transform_left,
        holomesh_transform_front,
        holomesh_transform_top,
        holomesh_transform_count
    } holomesh_transform_index;

    typedef enum _holomesh_craft_affiliation {
        holomesh_craft_affiliation_neutral,
        holomesh_craft_affiliation_rebel,
        holomesh_craft_affiliation_imperial,
        holomesh_craft_affiliation_bounty_hunter
    } holomesh_craft_affiliation;

#pragma pack(push, 1)
    typedef struct _holomesh_vec3 {
        uint32_t x, y, z;
    } holomesh_vec3;

    typedef struct _holomesh_vec2 {
        uint32_t u, v;
    } holomesh_vec2;

    typedef struct _holomesh_edge {
        uint8_t a, b;
    } holomesh_edge;

    typedef struct _holomesh_face_indices {
        uint8_t a, b, c;
    } holomesh_face_indices;

    typedef struct _holomesh_face {
        holomesh_face_indices positions;
        holomesh_face_indices uvs;
        uint8_t texture;
    } holomesh_face;

    typedef struct _holomesh_transform {
        uint32_t m[16];
    } holomesh_transform;

    typedef struct _holomesh_hull {
        HOLOMESH_ARRAY(holomesh_vec3) vertices;
        HOLOMESH_ARRAY(holomesh_vec3) scratch_vertices;
        HOLOMESH_ARRAY(holomesh_vec2) uvs;
        HOLOMESH_ARRAY(holomesh_edge) edges;
        HOLOMESH_ARRAY(holomesh_face) faces;
    } holomesh_hull;

    typedef struct _holomesh_string {
        HOLOMESH_ARRAY(char) str;
    } holomesh_string;

    typedef struct _holomesh_texture {
        HOLOMESH_ARRAY(uint8_t) data;
        uint16_t widthMinusOne;
        uint16_t heightMinusOne;
    } holomesh_texture;

    typedef struct _holomesh_info_point {
        holomesh_vec3 point;
        uint16_t name_string;
    } holomesh_info_point;

    typedef struct _holomesh_craft_info {
        uint16_t craft_name_string;
        uint8_t affiliation;
    } holomesh_craft_info;

    typedef struct _holomesh {
        uint32_t magic;
        uint32_t version;
        uint32_t file_size;
        uint32_t full_data_size;
        holomesh_craft_info info;
        holomesh_transform transforms[holomesh_transform_count];
        HOLOMESH_ARRAY(holomesh_hull) hulls;
        HOLOMESH_ARRAY(holomesh_info_point) info_points;
        HOLOMESH_ARRAY(uint16_t) info_stats;
        HOLOMESH_ARRAY(holomesh_texture) textures;
        HOLOMESH_ARRAY(holomesh_string) string_table;
    } holomesh;
#pragma pack(pop)

    typedef enum _holomesh_result {
        hmresult_ok,
        hmresult_fail,
        hmresult_bad_magic,
        hmresult_wrong_version,
        hmresult_bad_parameter,
        hmresult_buffer_too_small
    } holomesh_result;

    // Serialize utilities
    uint32_t holomesh_get_size(const holomesh* mesh, int include_scratch);
    uint32_t holomesh_texture_data_stride(uint16_t pixelsPerRow);
    uint32_t holomesh_texture_data_size(uint16_t w, uint16_t h);
    holomesh_result holomesh_pack_texture(uint8_t* dst, uint32_t dstSize, const uint8_t* texture, uint16_t width, uint16_t height);
    holomesh_result holomesh_unpack_texture(uint8_t* dst, uint32_t dstSize, const uint8_t* texture, uint16_t width, uint16_t height);
    void holomesh_set_vec2(holomesh_vec2* vec, uint16_t u, uint16_t v);
    void holomesh_set_vec3(holomesh_vec3* vec, uint16_t x, uint16_t y, uint16_t z);
    void holomesh_set_edge(holomesh_edge* e, uint8_t a, uint8_t b);
    void holomesh_set_face(holomesh_face* f, uint8_t pa, uint8_t pb, uint8_t pc, uint8_t ta, uint8_t tb, uint8_t tc, uint8_t texture);
    void holomesh_set_string(holomesh_string* out, char* str);
    void holomesh_set_texture(holomesh_texture* texture, uint8_t* data, uint32_t dataSize, uint16_t width, uint16_t height);
    void holomesh_set_info_point(holomesh_info_point* info, uint16_t nameIndex, const holomesh_vec3* point);
    void holomesh_set_transform(holomesh_transform* t, const uint16_t m[16]);

    // Lifetime of these arrays must persist over lifetime of hull
    void holomesh_set_hull(
        holomesh_hull* hull, 
        holomesh_vec3* vertices, uint32_t numVertices, 
        holomesh_vec2* uvs, uint32_t numUVs,
        holomesh_edge* edges, uint32_t numEdges,
        holomesh_face* faces, uint32_t numFaces);

    void holomesh_set_hulls(holomesh* mesh, holomesh_hull* hulls, uint32_t numHulls);
    void holomesh_set_info_points(holomesh* mesh, holomesh_info_point* points, uint32_t numPoints);
    void holomesh_set_info_stats(holomesh* mesh, uint16_t* stats, uint32_t numStats);
    void holomesh_set_textures(holomesh* mesh, holomesh_texture* textures, uint32_t numTextures);
    void holomesh_set_strings(holomesh* mesh, holomesh_string* strings, uint32_t numStrings);

    holomesh_result holomesh_serialize(const holomesh* sourceMesh, holomesh* destMesh, uint32_t destMeshSize);

    // Deserialize utilities
    holomesh_result holomesh_deserialize(holomesh* mesh, uint32_t meshSize);


#ifdef __cplusplus
}
#endif
