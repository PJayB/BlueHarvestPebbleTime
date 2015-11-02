#include "common.h"
#include "rasterizer.h"
#include "holomesh.h"
#include "render.h"
#include "matrix.h"
#include "wireframe.h"
#include "scratch.h"

#ifdef RASTERIZER_CHECKS
static size_t g_active_span_high_watermark = 0;
#endif

void render_init(size_t scratch_size) {
    rasterizer_init_span_pool();
    scratch_init(scratch_size);
}

void render_prep_frame(void) {
    scratch_clear();
}

void render_create_3d_transform(matrix_t* out, const matrix_t* proj, fix16_t angle) {
    // Rotate the transform
    matrix_t rotation;
    matrix_create_rotation_z(&rotation, angle);

    // Create the final transform
    matrix_multiply(out, &rotation, proj);
}

void render_transform_hull(const holomesh_hull_t* hull, const matrix_t* transform, fix16_t viewport_half_width, fix16_t viewport_half_height, vec3_t* transformed_vertices) {
    ASSERT(hull != NULL);
    ASSERT(hull->vertices.ptr != NULL);
    ASSERT(hull->vertices.size > 0);
    ASSERT(transform != NULL);
    ASSERT(transformed_vertices != NULL);

    for (size_t i = 0; i < hull->vertices.size; ++i) {
        vec3_t p;
        matrix_vector_transform(&p, &hull->vertices.ptr[i], transform);

        // Move to NDC
        p.x = fix16_floor(fix16_add(fix16_mul(p.x, viewport_half_width), viewport_half_width));
        p.y = fix16_floor(fix16_add(fix16_mul(p.y, viewport_half_height), viewport_half_height));

        transformed_vertices[i] = p;
    }
}

void render_draw_hull_wireframe(void* user_ptr, const holomesh_hull_t* hull, const vec3_t* transformed_vertices) {
    wireframe_context ctx;
    ctx.edge_indices = (const uint8_t*) hull->edges.ptr;
    ctx.num_edges = hull->edges.size;
    ctx.points = (vec3_t*) transformed_vertices;
    ctx.user_ptr = user_ptr;
    wireframe_draw(&ctx);
}

void render_draw_mesh_wireframe(void* user_ptr, const viewport_t* viewport, const holomesh_t* mesh, const matrix_t* transform) {
    fix16_t half_width = viewport->fwidth >> 1;
    fix16_t half_height = viewport->fheight >> 1;

    vec3_t* scratch = (vec3_t*) scratch_alloc(mesh->scratch_size);

    for (size_t i = 0; i < mesh->hulls.size; ++i) {
        const holomesh_hull_t* hull = &mesh->hulls.ptr[i];
        render_transform_hull(hull, transform, half_width, half_height, scratch);
        render_draw_hull_wireframe(user_ptr, hull, scratch);
    }
}

#define MAX_HULLS 16
#define MAX_KICKOFFS 250

static fix16_t g_depths[MAX_VIEWPORT_X];

void render_scanlines(void* user_ptr, const viewport_t* viewport, const holomesh_t* mesh, const rasterizer_face_kickoff* face_kickoffs, size_t num_kickoffs, const vec3_t** hull_transformed_vertices) {
    // Set up the rasterizer
    ASSERT(viewport->width <= MAX_VIEWPORT_X);

    uint16_t min_y = face_kickoffs[0].y;

    rasterizer_context raster_ctx;
    raster_ctx.depths = g_depths;
    raster_ctx.user_ptr = user_ptr;

    rasterizer_stepping_span* active_span_list = NULL;

    const rasterizer_face_kickoff* face_kickoff = face_kickoffs;
    const rasterizer_face_kickoff* last_kickoff = face_kickoff + num_kickoffs;
    for (uint16_t y = min_y; y < viewport->height; ++y)
    {
        // Kick off any faces this scanline?
        while (face_kickoff->y == y && face_kickoff < last_kickoff)
        {
            // Kick off the face
            const holomesh_hull_t* hull = &mesh->hulls.ptr[face_kickoff->hull_index];
            const holomesh_face_t* face = &hull->faces.ptr[face_kickoff->face_index];

            active_span_list = rasterizer_create_face_spans(
                active_span_list,
                viewport,
                face,
                hull_transformed_vertices[face_kickoff->hull_index],
                hull->uvs.ptr,
                &mesh->textures.ptr[face->texture],
                y,
                (uint8_t) face_kickoff->clip);
            face_kickoff++;
        }

        if (!active_span_list)
            return; // end of mesh (we know all meshes are contiguous)

                    // Clear the depth values for this scanline
        memset(g_depths, 0, sizeof(g_depths));

        active_span_list = rasterizer_draw_active_spans(
            &raster_ctx,
            active_span_list,
            y);

        // TODO: 
        //gActiveSpanHighWaterMark = std::max(gActiveSpanHighWaterMark, rasterizer_get_active_stepping_span_count());
    }

    // Check for span leak
    ASSERT(active_span_list == NULL);
}

size_t render_create_mesh_kickoffs(const viewport_t* viewport, const holomesh_t* mesh, const matrix_t* transform, rasterizer_face_kickoff* face_kickoffs, size_t max_kickoffs, const vec3_t** hull_transformed_vertices) {
    rasterizer_face_kickoff* face_kickoff = &face_kickoffs[0];
    size_t face_kickoff_count = 0;

    fix16_t half_width = viewport->fwidth >> 1;
    fix16_t half_height = viewport->fheight >> 1;

    // Transform all the points
    for (size_t hull_index = 0; hull_index < mesh->hulls.size; ++hull_index) {
        const holomesh_hull_t* hull = &mesh->hulls.ptr[hull_index];

        // Transform the hull
        vec3_t* transformed_vertices = (vec3_t*) scratch_alloc(sizeof(vec3_t) * hull->vertices.size);
        render_transform_hull(hull, transform, half_width, half_height, transformed_vertices);
        hull_transformed_vertices[hull_index] = transformed_vertices;

        // Submit the faces to the face list
        size_t faces_added = rasterizer_create_face_kickoffs(
            face_kickoff,
            max_kickoffs - face_kickoff_count,
            viewport,
            hull_index,
            transformed_vertices,
            hull->faces.ptr,
            hull->faces.size);

        face_kickoff += faces_added;
        face_kickoff_count += faces_added;
    }

    // Sort the kickoffs
    rasterizer_sort_face_kickoffs(face_kickoffs, face_kickoff_count);

    return face_kickoff_count;
}

// TODO: move me
static rasterizer_face_kickoff g_face_kickoffs[MAX_KICKOFFS];

void render_draw_mesh_solid(void* user_ptr, const viewport_t* viewport, const holomesh_t* mesh, const matrix_t* transform) {
    const vec3_t* hull_transformed_vertices[MAX_HULLS];

    ASSERT(mesh->hulls.size < MAX_HULLS);

    size_t kickoff_count = render_create_mesh_kickoffs(
        viewport,
        mesh,
        transform,
        g_face_kickoffs,
        MAX_KICKOFFS,
        hull_transformed_vertices);

    render_scanlines(
        user_ptr,
        viewport,
        mesh,
        g_face_kickoffs,
        kickoff_count,
        hull_transformed_vertices);
}

#ifdef RASTERIZER_CHECKS
size_t render_get_span_high_watermark(void) {
    return g_active_span_high_watermark;
}
#endif
