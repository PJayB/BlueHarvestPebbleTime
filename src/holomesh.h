#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

    /*---------------------------------

    MODEL LOADING

    ---------------------------------*/
#define HOLOMESH_MAGIC 0x484f4c4fu // 'HOLO'
#define HOLOMESH_VERSION 4

#define HOLOMESH_ARRAY_PTR(type) union { type* ptr; int32_t offset; }
#define HOLOMESH_ARRAY(type) struct { HOLOMESH_ARRAY_PTR(type); uint32_t size; }

#define HOLOMESH_BAD_REF 0xFFFF

    typedef enum holomesh_transform_index_e {
        holomesh_transform_perspective_pebble_aspect,
        holomesh_transform_perspective_square_aspect,
        holomesh_transform_left,
        holomesh_transform_front,
        holomesh_transform_top,
        holomesh_transform_count
    } holomesh_transform_index_t;

    typedef enum holomesh_craft_affiliation_e {
        holomesh_craft_affiliation_neutral,
        holomesh_craft_affiliation_rebel,
        holomesh_craft_affiliation_imperial,
        holomesh_craft_affiliation_bounty_hunter
    } holomesh_craft_affiliation_t;

    typedef struct vec2_s holomesh_vec2_t;
    typedef struct vec3_s holomesh_vec3_t;
    typedef struct edge_s holomesh_edge_t;
    typedef struct face_s holomesh_face_t;
    typedef struct matrix_s holomesh_transform_t;
    
#pragma pack(push, 1)
    typedef struct holomesh_hull_s {
        HOLOMESH_ARRAY(holomesh_vec3_t) vertices;
        HOLOMESH_ARRAY(holomesh_vec2_t) uvs;
        HOLOMESH_ARRAY(holomesh_edge_t) edges;
        HOLOMESH_ARRAY(holomesh_face_t) faces;
    } holomesh_hull_t;

    typedef struct holomesh_string_s {
        HOLOMESH_ARRAY(char) str;
    } holomesh_string_t;

    typedef struct holomesh_texture_s {
        HOLOMESH_ARRAY(uint8_t) data;
        uint32_t scale_u;
        uint32_t scale_v;
        uint16_t stride;
        uint16_t width;
        uint16_t height;
    } holomesh_texture_t;

    typedef struct holomesh_info_point_s {
        holomesh_vec3_t point;
        uint16_t name_string;
    } holomesh_info_point_t;

    typedef struct holomesh_craft_info_s {
        uint16_t craft_name_string;
        uint8_t affiliation;
    } holomesh_craft_info_t;

    typedef struct holomesh_s {
        uint32_t magic;
        uint32_t version;
        uint32_t file_size;
        uint32_t scratch_size;
        holomesh_craft_info_t info;
        holomesh_transform_t transforms[holomesh_transform_count];
        HOLOMESH_ARRAY(holomesh_hull_t) hulls;
        HOLOMESH_ARRAY(holomesh_info_point_t) info_points;
        HOLOMESH_ARRAY(uint16_t) info_stats;
        HOLOMESH_ARRAY(holomesh_texture_t) textures;
        HOLOMESH_ARRAY(holomesh_string_t) string_table;
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
    void holomesh_set_vec2(holomesh_vec2_t* vec, uint32_t u, uint32_t v);
    void holomesh_set_vec3(holomesh_vec3_t* vec, uint32_t x, uint32_t y, uint32_t z);
    void holomesh_set_edge(holomesh_edge_t* e, uint8_t a, uint8_t b);
    void holomesh_set_face(holomesh_face_t* f, uint8_t pa, uint8_t pb, uint8_t pc, uint8_t ta, uint8_t tb, uint8_t tc, uint8_t texture);
    void holomesh_set_string(holomesh_string_t* out, char* str);
    void holomesh_set_texture(holomesh_texture_t* texture, uint8_t* data, uint32_t dataSize, uint16_t width, uint16_t height);
    void holomesh_set_info_point(holomesh_info_point_t* info, uint16_t nameIndex, const holomesh_vec3_t* point);
    void holomesh_set_transform(holomesh_transform_t* t, const uint32_t m[16]);

    // Lifetime of these arrays must persist over lifetime of hull
    void holomesh_set_hull(
        holomesh_hull_t* hull,
        holomesh_vec3_t* vertices, uint32_t numVertices, 
        holomesh_vec2_t* uvs, uint32_t numUVs,
        holomesh_edge_t* edges, uint32_t numEdges,
        holomesh_face_t* faces, uint32_t numFaces);

    void holomesh_set_hulls(holomesh* mesh, holomesh_hull_t* hulls, uint32_t numHulls);
    void holomesh_set_info_points(holomesh* mesh, holomesh_info_point_t* points, uint32_t numPoints);
    void holomesh_set_info_stats(holomesh* mesh, uint16_t* stats, uint32_t numStats);
    void holomesh_set_textures(holomesh* mesh, holomesh_texture_t* textures, uint32_t numTextures);
    void holomesh_set_strings(holomesh* mesh, holomesh_string_t* strings, uint32_t numStrings);

    holomesh_result holomesh_serialize(const holomesh* sourceMesh, holomesh* destMesh, uint32_t destMeshSize);

    // Deserialize utilities
    holomesh_result holomesh_deserialize(holomesh* mesh, uint32_t meshSize);


#ifdef __cplusplus
}
#endif
