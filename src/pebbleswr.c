#include "common.h"
#include "pebbleswr.h"

void pswr_init_context(pswr_context* context, int width, int height) {
    context->width = width;
    context->height = height;
}
