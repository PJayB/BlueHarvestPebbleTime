#include "common.h"
#include "rasterizer.h"

#ifdef SANDBOX
#   include <stdlib.h>
#endif

typedef struct _rasterizer_stepping_span_freelist_node {
    union {
        rasterizer_stepping_span span;
        struct _rasterizer_stepping_span_freelist_node* next;
    };
} rasterizer_stepping_span_freelist_node;

#ifdef SANDBOX
#   define MAX_SPAN_COUNT 100000
#else
#   define MAX_SPAN_COUNT 100
#endif

static rasterizer_stepping_span_freelist_node g_span_pool[MAX_SPAN_COUNT];
static rasterizer_stepping_span_freelist_node* g_first_free = NULL;
static size_t g_active_span_count = 0;

static const size_t c_span_pool_size = MAX_SPAN_COUNT;

void rasterizer_init_span_pool(void) {
    memset(g_span_pool, 0, sizeof(g_span_pool));
    for (size_t i = 0; i < c_span_pool_size - 1; ++i) {
        g_span_pool[i].next = &g_span_pool[i+1];
    }
    g_first_free = &g_span_pool[0];
}

rasterizer_stepping_span* rasterizer_allocate_stepping_span(void) {
    ASSERT(g_first_free != NULL);
    if (g_first_free == NULL) {
        ASSERT(g_active_span_count == c_span_pool_size);
        return NULL; // no more spans available! :(
    }
    rasterizer_stepping_span* span = &g_first_free->span;
    g_first_free = g_first_free->next;
    g_active_span_count++;
    return span;
}

void rasterizer_free_stepping_span(rasterizer_stepping_span* span) {
    ASSERT(g_active_span_count > 0);
    ASSERT(span != NULL);
    rasterizer_stepping_span_freelist_node* node = (rasterizer_stepping_span_freelist_node*) span;
    node->next = g_first_free;
    g_first_free = node;
    g_active_span_count--;
}

size_t rasterizer_get_active_stepping_span_count(void) {
    return g_active_span_count;
}
