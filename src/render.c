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

void render_init(void) {
    rasterizer_init_span_pool();
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

void render_transform_point(vec3_t* out_p, const vec3_t* p, const matrix_t* transform, fix16_t viewport_half_width, fix16_t viewport_half_height) {
    matrix_vector_transform(out_p, p, transform);

    // Move to NDC
    out_p->x = fix16_floor(fix16_add(fix16_mul(out_p->x, viewport_half_width), viewport_half_width));
    out_p->y = fix16_floor(fix16_add(fix16_mul(out_p->y, viewport_half_height), viewport_half_height));
}

void render_transform_hulls(const holomesh_hull_t* hulls, size_t num_hulls, const viewport_t* viewport, const matrix_t* transform, render_transform_hulls_info_t* info) {
    fix16_t max_x = -fix16_maximum;
    fix16_t max_y = -fix16_maximum;
    fix16_t min_x = fix16_maximum;
    fix16_t min_y = fix16_maximum;

    fix16_t viewport_half_width = viewport->fwidth >> 1;
    fix16_t viewport_half_height = viewport->fheight >> 1;

    for (size_t hull_index = 0; hull_index < num_hulls; ++hull_index) {
        const holomesh_hull_t* hull = &hulls[hull_index];

        vec3_t* transformed_vertices = scratch_alloc(sizeof(vec3_t) * hull->vertices.size);
        info->transformed_points[hull_index] = transformed_vertices;

        for (size_t i = 0; i < hull->vertices.size; ++i) {
            vec3_t p;
            matrix_vector_transform(&p, &hull->vertices.ptr[i], transform);

            // Move to NDC
            p.x = fix16_floor(fix16_add(fix16_mul(p.x, viewport_half_width), viewport_half_width));
            p.y = fix16_floor(fix16_add(fix16_mul(p.y, viewport_half_height), viewport_half_height));

            min_x = fix16_min(min_x, p.x);
            min_y = fix16_min(min_y, p.y);
            max_x = fix16_max(max_x, p.x);
            max_y = fix16_max(max_y, p.y);

            transformed_vertices[i] = p;
        }     
    }

    info->min_x = min_x;
    info->max_x = max_x;
    info->min_y = min_y;
    info->max_y = max_y;
}

void render_draw_hull_wireframe(void* user_ptr, const holomesh_hull_t* hull, const vec3_t* transformed_vertices) {
    wireframe_context ctx;
    ctx.edge_indices = (const uint8_t*) hull->edges.ptr;
    ctx.num_edges = hull->edges.size;
    ctx.points = (vec3_t*) transformed_vertices;
    ctx.user_ptr = user_ptr;
    wireframe_draw(&ctx);
}

void render_draw_mesh_wireframe(void* user_ptr, const holomesh_t* mesh, const vec3_t* const* transformed_points) {
    for (size_t i = 0; i < mesh->hulls.size; ++i) {
        const holomesh_hull_t* hull = &mesh->hulls.ptr[i];
        render_draw_hull_wireframe(user_ptr, hull, transformed_points[i]);
    }
}

#define MAX_KICKOFFS 250

static fix16_t g_depths[MAX_VIEWPORT_X];

typedef struct render_context_s {
    render_frame_buffer_t* frame_buffer;
    int8_t color_mod;
} render_context_t;

void raster_set_pixel_on_row(uint8_t* row_data, int x, uint8_t color) {
#ifdef FULL_COLOR
    row_data[x] = color;
#else
    int byte_offset = x >> 2; // divide by 4 to get actual pixel byte
    int bit_shift = (~x & 3) << 1;
    uint8_t mask = 3 << bit_shift;
    uint8_t src = row_data[byte_offset] & ~mask;
    row_data[byte_offset] = src | (color << bit_shift);
#endif
}

void rasterizer_set_pixel(void* user_ptr, int x, int y, uint8_t color) {
    render_context_t* ctx = (render_context_t*) user_ptr;
    render_frame_buffer_t* fb = ctx->frame_buffer;
    raster_set_pixel_on_row(&fb->data[y * fb->row_stride], x, color);
}

#ifndef FULL_COLOR
void rasterizer_set_pixel_4(void* user_ptr, int x, int y, uint8_t color, uint8_t mask) {
    render_context_t* ctx = (render_context_t*) user_ptr;
    render_frame_buffer_t* fb = ctx->frame_buffer;
    uint8_t* row = &fb->data[y * fb->row_stride];
    row[x] = (color & mask);
}
#endif

uint8_t rasterizer_shade(void* user_ptr, uint8_t color) {
    render_context_t* ctx = (render_context_t*) user_ptr;
    
    // TODO: saturated add?
    int8_t c = color + ctx->color_mod;
    if (c < 0 || c > 3) return color;
    else return c;
}

void render_scanlines(render_frame_buffer_t* frame_buffer, const viewport_t* viewport, const holomesh_t* mesh, const rasterizer_face_kickoff_t* face_kickoffs, size_t num_kickoffs, const vec3_t* const* hull_transformed_vertices) {
    // Set up the rasterizer
    ASSERT(viewport->width <= MAX_VIEWPORT_X);

    uint16_t min_y = face_kickoffs[0].y;
    
    render_context_t render_ctx;
    render_ctx.frame_buffer = frame_buffer;

    rasterizer_context_t raster_ctx;
    raster_ctx.depths = g_depths;
    raster_ctx.user_ptr = &render_ctx;

    rasterizer_stepping_span_t* active_span_list = NULL;

    const rasterizer_face_kickoff_t* face_kickoff = face_kickoffs;
    const rasterizer_face_kickoff_t* last_kickoff = face_kickoff + num_kickoffs;
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
      
        // What color difference shall we apply to this line?
        render_ctx.color_mod = get_color_mod((uint8_t) y);

        // Clear the depth values for this scanline
        memset(g_depths, 0, sizeof(g_depths));

        active_span_list = rasterizer_draw_active_spans(
            &raster_ctx,
            active_span_list,
            (uint8_t) y);
    }

    // Check for span leak
    ASSERT(active_span_list == NULL);
}

size_t render_create_mesh_kickoffs(const viewport_t* viewport, const holomesh_t* mesh, rasterizer_face_kickoff_t* face_kickoffs, size_t max_kickoffs, const vec3_t* const* hull_transformed_vertices) {
    rasterizer_face_kickoff_t* face_kickoff = &face_kickoffs[0];
    size_t face_kickoff_count = 0;

    // Transform all the points
    for (size_t hull_index = 0; hull_index < mesh->hulls.size; ++hull_index) {
        const holomesh_hull_t* hull = &mesh->hulls.ptr[hull_index];

        // Submit the faces to the face list
        size_t faces_added = rasterizer_create_face_kickoffs(
            face_kickoff,
            max_kickoffs - face_kickoff_count,
            viewport,
            hull_index,
            hull_transformed_vertices[hull_index],
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
static rasterizer_face_kickoff_t g_face_kickoffs[MAX_KICKOFFS];

void render_draw_mesh_solid(render_frame_buffer_t* frame_buffer, const viewport_t* viewport, const holomesh_t* mesh, const vec3_t* const* transformed_points) {
    
    size_t kickoff_count = render_create_mesh_kickoffs(
        viewport,
        mesh,
        g_face_kickoffs,
        MAX_KICKOFFS,
        transformed_points);

    render_scanlines(
        frame_buffer,
        viewport,
        mesh,
        g_face_kickoffs,
        kickoff_count,
        transformed_points);
}

#ifdef RASTERIZER_CHECKS
size_t render_get_span_high_watermark(void) {
    return g_active_span_high_watermark;
}
#endif

