#include "common.h"
#include "wireframe.h"

void wireframe_init_context(wireframe_context* context, const holomesh* holomesh, int width, int height) {
    context->width = width;
    context->height = height;
    context->mesh = holomesh;
}

void wireframe_draw(wireframe_context* context) {
    (void) context;
}
