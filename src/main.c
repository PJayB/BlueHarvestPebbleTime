#include "common.h"
#include "holomesh.h"
#include "matrix.h"
#include "render.h"

// TODO: remove me
#include "scratch.h"

#define PROFILE
//#define OVERDRAW_TEST
#define NO_HOLO_EFFECT

char g_craft_name_lower[256] = {0};
    
static GColor c_palette[] = {
    {0b00000000},
    {GColorCobaltBlueARGB8}, // CobaltBlue or DukeBlue
    {GColorVividCeruleanARGB8},
    {GColorCelesteARGB8}
};

static const struct craft_info_s {
    int resource_id;
} c_craft_info[] = {
    { RESOURCE_ID_HOLO_AWING },
    { RESOURCE_ID_HOLO_BWING },
    { RESOURCE_ID_HOLO_CORTN },
    { RESOURCE_ID_HOLO_CORV },
    { RESOURCE_ID_HOLO_ISD },
    { RESOURCE_ID_HOLO_NEB },
    { RESOURCE_ID_HOLO_SHUTTLE },
    { RESOURCE_ID_HOLO_SLAVEONE },
    { RESOURCE_ID_HOLO_SSD },
    { RESOURCE_ID_HOLO_TIEADV },
    { RESOURCE_ID_HOLO_TIEBMB },
    { RESOURCE_ID_HOLO_TIEFTR },
    { RESOURCE_ID_HOLO_TIEINT },
    { RESOURCE_ID_HOLO_XWING },
    { RESOURCE_ID_HOLO_YWING }
};

static const int c_affiliation_logos[] = {
    0, //holomesh_craft_affiliation_not_set,
    0, //holomesh_craft_affiliation_neutral,
    RESOURCE_ID_PREZR_REBELBG_PACK, //holomesh_craft_affiliation_rebel,
    RESOURCE_ID_PREZR_IMPBG_PACK, //holomesh_craft_affiliation_imperial,
    RESOURCE_ID_PREZR_MANDBG_PACK  //holomesh_craft_affiliation_bounty_hunter
};

static const int c_craft_info_count = sizeof(c_craft_info) / sizeof(struct craft_info_s);

#define MAX_MEMORY_SIZE 30000
#define MAX_HULLS 47

static const int c_refreshTimer = 100;
static const int c_viewportWidth = 144;
static const int c_viewportHeight = 168;

#define BACKGROUND_BLOB_SIZE 2622
static uint8_t g_background_blob[BACKGROUND_BLOB_SIZE];

// UI elements
Window *my_window;
BitmapLayer *frameBufferLayer;
BitmapLayer* logoLayer;
TextLayer* textLayer;
TextLayer* textLayerSym;
TextLayer* infoTextLayer;
TextLayer* timeLayer;

// Global resources
GBitmap* frameBufferBitmap;
GBitmap* logoBitmap;

GFont g_font_sw;
GFont g_font_sw_symbol;
GFont g_font_time;
GFont g_font_info;

AppTimer* g_timer;

// Rendering stuff
holomesh_t* g_holomesh = NULL;
matrix_t g_last_transform;
size_t g_current_stat = 0;
size_t g_hologram_frame = 0;
vec3_t* g_transformed_points[MAX_HULLS];
char g_time_str[12];
int g_current_craft = 0;

void update_title_and_info(void);

typedef struct prezr_pack_header_s {
    uint32_t reserved;
    uint32_t num_resources;
} prezr_pack_header_t;

typedef struct prezr_bitmap_s {
    uint16_t width;
    uint16_t height;
    size_t   data_offset;
} prezr_bitmap_t;

const uint8_t* prezr_load_image_data(void* blob) {
    prezr_pack_header_t* pack_header = pack_header = (prezr_pack_header_t*) blob;
    prezr_bitmap_t* resources = (prezr_bitmap_t*) (blob + sizeof(prezr_pack_header_t));
    
    ASSERT(pack_header->num_resources == 1);
    
    // Fix up the pointer to the resource data
    return blob + resources[0].data_offset;
}

void load_background_image(int affiliation) {
    int logo_resource_id = c_affiliation_logos[affiliation];
    ASSERT(logo_resource_id != 0);
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Loading logo resource %i for affiliation %i", logo_resource_id, affiliation);
    
    ResHandle logo_resource_handle = resource_get_handle(logo_resource_id);
    ASSERT(resource_size(logo_resource_handle) == sizeof(g_background_blob));
    if (resource_load(logo_resource_handle, g_background_blob, sizeof(g_background_blob)) != sizeof(g_background_blob)) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to load background logo resource %u", (unsigned) logo_resource_id);
    } else if (logoBitmap == NULL) {
        const uint8_t* data = prezr_load_image_data(g_background_blob);
        logoBitmap = gbitmap_create_with_data(data);
        if (logoBitmap == NULL) {
            APP_LOG(APP_LOG_LEVEL_ERROR, "Loaded background logo resource %u but gbitmap create failed", (unsigned) logo_resource_id);
        }
    }
}

void load_holomesh(int craft_index) {
    g_current_craft = craft_index;
#ifdef OVERDRAW_TEST
    int resource_id = RESOURCE_ID_HOLO_TIEFTR;
#else
    int resource_id = c_craft_info[craft_index].resource_id;
#endif
    ResHandle handle = resource_get_handle(resource_id);
    
    // Allocate space for the resource
    // TODO: estimate this better
    if (g_holomesh == NULL) {
        g_holomesh = (holomesh_t*) malloc(MAX_MEMORY_SIZE);
        ASSERT(g_holomesh != NULL);
    }

    // Load it
    size_t size = resource_size(handle);
    size_t copied = resource_load(handle, (uint8_t*) g_holomesh, size);
    ASSERT(copied == g_holomesh->file_size);
    ASSERT(size >= g_holomesh->file_size);
    
    // Deserialize it
    holomesh_result hr = holomesh_deserialize(g_holomesh, size);
    ASSERT(hr == hmresult_ok);
    
    // Allocate renderer resources
    size_t scratch_size = g_holomesh->scratch_size;
    scratch_set(((uint8_t*)g_holomesh) + size, scratch_size);
    
    // Init the renderer
    render_init();
    
    // Load the logo
    load_background_image(g_holomesh->info.affiliation);
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "HOLOMESH: %u bytes [0x%x-0x%x], %u mesh, %u scratch, %u hulls", 
            (unsigned) MAX_MEMORY_SIZE,
            (unsigned) g_holomesh,
            (unsigned) g_holomesh + (unsigned) MAX_MEMORY_SIZE,
            (unsigned) size,
            (unsigned) scratch_size,
            (unsigned) g_holomesh->hulls.size);

    ASSERT(g_holomesh->hulls.size <= MAX_HULLS);
}

void clear_framebuffer(void) {
    for (int i = 0; i < c_viewportHeight; ++i) {
        GBitmapDataRowInfo row_info = gbitmap_get_data_row_info(frameBufferBitmap, i);
        memset(row_info.data, 0, c_viewportWidth >> 2);
    }
}

void paint(void) {
    render_prep_frame();

    clear_framebuffer();

    // Get the projection matrix_t
    static fix16_t angle = 0;

    render_create_3d_transform(
        &g_last_transform, 
        &g_holomesh->transforms[holomesh_transform_perspective_pebble_aspect], 
        angle);

    angle += fix16_one >> 6;

    // Set up viewport
    viewport_t viewport;
    viewport_init(&viewport, c_viewportWidth, c_viewportHeight);

    // Transform the mesh
    render_transform_hulls_info_t t;
    t.transformed_points = g_transformed_points;

    render_transform_hulls(
        g_holomesh->hulls.ptr, 
        g_holomesh->hulls.size, 
        &viewport, 
        &g_last_transform, 
        &t);
    
    render_frame_buffer_t fb;
    fb.data = gbitmap_get_data(frameBufferBitmap);
    fb.row_stride = gbitmap_get_bytes_per_row(frameBufferBitmap);

    // Render the mesh
    render_draw_mesh_solid(
        &fb,
        &viewport,
        g_holomesh,
        (const vec3_t* const*) g_transformed_points);

    g_hologram_frame++;
}

int8_t get_color_mod(uint8_t y) {
#ifdef NO_HOLO_EFFECT
    return 0;
#else
    return (y & 1) - (((y - g_hologram_frame) & 7) == 7);
#endif
}

#ifdef PROFILE
static uint32_t max_time = 0;
static uint32_t min_time = 10000;
static uint32_t total_time = 0;
static uint32_t samples = 0;
#endif

void animation_timer_trigger(void* data) {
#ifdef PROFILE
    uint32_t paint_start_time = get_milliseconds();
#endif
    paint();
#ifdef PROFILE
    uint32_t paint_time = get_milliseconds() - paint_start_time;
    min_time = min_time > paint_time ? paint_time : min_time;
    max_time = max_time < paint_time ? paint_time : max_time;
    total_time += paint_time;
    samples++;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "PAINT %ums (min %ums, max %ums, avg %ums)", (unsigned) paint_time, (unsigned) min_time, (unsigned) max_time, (unsigned)(total_time / samples));
#endif
    
    layer_mark_dirty((Layer*) frameBufferLayer); 
    g_timer = app_timer_register(c_refreshTimer, animation_timer_trigger, NULL);    
}

void set_new_stat_text(void) {
    const holomesh_tag_t* tag = &g_holomesh->tags.ptr[
        (++g_current_stat) % g_holomesh->tags.size
    ];

    const char* stat = g_holomesh->string_table.ptr[tag->name_string].str.ptr;

    GRect currentFrame = layer_get_bounds(text_layer_get_layer(infoTextLayer));

    currentFrame.size = GSize(c_viewportWidth, c_viewportHeight);
    currentFrame.size = graphics_text_layout_get_content_size(
        stat,
        g_font_info,
        currentFrame,
        0,
        GTextAlignmentLeft);
    
    layer_set_bounds(text_layer_get_layer(infoTextLayer), currentFrame);
    text_layer_set_text(infoTextLayer, stat);
}

void update_time_display(struct tm *tick_time) {
    if (clock_is_24h_style()) {
        strftime(g_time_str, sizeof(g_time_str), "%R", tick_time);
    } else {
        strftime(g_time_str, sizeof(g_time_str), "%r", tick_time);
    }
    text_layer_set_text(timeLayer, g_time_str);
}

uint32_t g_stat_timer = 1;
void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
#ifndef PROFILE
    if (units_changed & SECOND_UNIT) {
        g_stat_timer++;

        if (g_stat_timer == 8) {        
            set_new_stat_text();
            g_stat_timer = 0;
        }
    }
    if (units_changed & MINUTE_UNIT) {
        int new_craft_index = rand() % (c_craft_info_count - 1);
        if (new_craft_index >= g_current_craft) {
            new_craft_index++;
        }
        
        load_holomesh(new_craft_index);
        update_title_and_info();
        update_time_display(tick_time);
    }
#endif
}

void create_symbol_text(char* out, size_t out_size, const char* in) {
    for (size_t i = 0; *in && i < out_size - 1; ++in) {
        if (*in >= 'A' && *in <= 'Z')
            *out++ = (*in - 'A') + 'a';
        else if (*in >= 'a' && *in <= 'a')
            *out++ = *in;
        else if (*in == ' ' || *in == '\n')
            *out++ = *in;
    }
    *out = 0;
}

void update_title_and_info(void) {
    GRect layerSize = GRect(0, 0, c_viewportWidth, c_viewportHeight);
    
    const char* text = g_holomesh->string_table.ptr[g_holomesh->info.craft_name_string].str.ptr;
    create_symbol_text(g_craft_name_lower, sizeof(g_craft_name_lower), text);
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "'%s' -> '%s'", text, g_craft_name_lower);
    
    GSize textSize = graphics_text_layout_get_content_size(
        text,
        g_font_sw,
        layerSize,
        0,
        GTextAlignmentLeft);
    
    GRect textRect = { GPoint(0, 0), textSize };
    layer_set_bounds((Layer*) textLayer, textRect);
    layer_set_bounds((Layer*) textLayerSym, textRect);
    
    text_layer_set_text(textLayer, text);
    text_layer_set_text(textLayerSym, g_craft_name_lower);

    GRect currentFrame = layer_get_bounds((Layer*) infoTextLayer);
    currentFrame.origin.y = textRect.origin.y + textRect.size.h / 2;
    layer_set_bounds((Layer*) infoTextLayer, currentFrame);

    set_new_stat_text();
}

//Hippo Command, I put my pants on backwards!
void handle_init(void) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "INIT MEMORY: %u bytes used, %u bytes free", (unsigned) heap_bytes_used(), (unsigned) heap_bytes_free());
    
    // TODO: restore this once done profiling
#ifndef PROFILE
    load_holomesh(rand() % c_craft_info_count);
#else
    load_holomesh(2);
#endif
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "UI MEMORY: %u bytes used, %u bytes free", (unsigned) heap_bytes_used(), (unsigned) heap_bytes_free());

    my_window = window_create();
    window_set_background_color(my_window, GColorBlack);

    GRect logoRect = GRect(0, 12, c_viewportWidth, c_viewportWidth);
    logoLayer = bitmap_layer_create(logoRect);
    bitmap_layer_set_bitmap(logoLayer, logoBitmap);
    layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(logoLayer));

    // Fonts    
    g_font_sw = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_12));
    g_font_sw_symbol = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SYMBOL_12));
    g_font_time = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
    g_font_info = fonts_get_system_font(FONT_KEY_GOTHIC_09);
    
    // Paint layer
    frameBufferLayer = bitmap_layer_create(GRect(0, 0, c_viewportWidth, c_viewportHeight));
    layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(frameBufferLayer));
    
    frameBufferBitmap = gbitmap_create_blank_with_palette(
        GSize(c_viewportWidth, c_viewportHeight),
        GBitmapFormat2BitPalette,
        c_palette,
        false);
    bitmap_layer_set_bitmap(frameBufferLayer, frameBufferBitmap);
    bitmap_layer_set_compositing_mode(frameBufferLayer, GCompOpSet);
    
    paint();

    GRect layerSize = GRect(0, 0, c_viewportWidth, c_viewportHeight);

    // Two small text layers
    textLayer = text_layer_create(layerSize);
    text_layer_set_background_color(textLayer, GColorClear);
    text_layer_set_text_color(textLayer, GColorYellow);
    text_layer_set_font(textLayer, g_font_sw);
    layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(textLayer));

    //Jet Force Push-up, you silly-billy.   
    textLayerSym = text_layer_create(layerSize);
    text_layer_set_background_color(textLayerSym, GColorClear);
    text_layer_set_text_color(textLayerSym, GColorYellow);
    text_layer_set_font(textLayerSym, g_font_sw_symbol);
    layer_set_hidden(text_layer_get_layer(textLayerSym), true);
    layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(textLayerSym));

    //Hippo Command, I also put my watch on backwards!

    // Info text layer
    infoTextLayer = text_layer_create(layerSize);
    text_layer_set_background_color(infoTextLayer, GColorClear);
    text_layer_set_text_color(infoTextLayer, GColorYellow);
    text_layer_set_font(infoTextLayer, g_font_info);
    layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(infoTextLayer));
    
    // Time
    GSize timeSize = graphics_text_layout_get_content_size(
        "0:APM",
        g_font_time,
        layerSize,
        0,
        GTextAlignmentLeft);
    GRect timeRect = { GPoint(3, c_viewportHeight - timeSize.h), GSize(c_viewportWidth, timeSize.h) };
    timeLayer = text_layer_create(timeRect);
    text_layer_set_background_color(timeLayer, GColorClear);
    text_layer_set_text_color(timeLayer, GColorYellow);
    text_layer_set_font(timeLayer, g_font_time);
    //text_layer_set_text(timeLayer, "23:45 AM");
    layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(timeLayer));
    
    time_t t = time(NULL);
    update_time_display(localtime(&t));
    update_title_and_info();
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "FINAL MEMORY: %u bytes used, %u bytes free", (unsigned) heap_bytes_used(), (unsigned) heap_bytes_free());
    
    window_stack_push(my_window, true);
    tick_timer_service_subscribe(SECOND_UNIT | MINUTE_UNIT, tick_handler);
    
    g_timer = app_timer_register(c_refreshTimer, animation_timer_trigger, NULL);
}

void handle_deinit(void) {
    app_timer_cancel(g_timer);

    APP_LOG(APP_LOG_LEVEL_DEBUG, "SHUTDOWN:");
    APP_LOG(APP_LOG_LEVEL_DEBUG, "  pre: %u bytes used", (unsigned) heap_bytes_used());

    free(g_holomesh);

    APP_LOG(APP_LOG_LEVEL_DEBUG, "  holomesh cleaned up: %u bytes remaining", (unsigned) heap_bytes_used());

    gbitmap_destroy(frameBufferBitmap);
    gbitmap_destroy(logoBitmap);

    APP_LOG(APP_LOG_LEVEL_DEBUG, "  bitmaps cleaned up: %u bytes remaining", (unsigned) heap_bytes_used());

    fonts_unload_custom_font(g_font_sw_symbol);
    fonts_unload_custom_font(g_font_sw);

    APP_LOG(APP_LOG_LEVEL_DEBUG, "  fonts cleaned up: %u bytes remaining", (unsigned) heap_bytes_used());

    text_layer_destroy(timeLayer);
    text_layer_destroy(textLayer);
    text_layer_destroy(textLayerSym);
    text_layer_destroy(infoTextLayer);
    bitmap_layer_destroy(frameBufferLayer);
    bitmap_layer_destroy(logoLayer);

    APP_LOG(APP_LOG_LEVEL_DEBUG, "  layers cleaned up: %u bytes remaining", (unsigned) heap_bytes_used());

    window_destroy(my_window);

    APP_LOG(APP_LOG_LEVEL_DEBUG, "FINAL SHUTDOWN: %u bytes used", (unsigned) heap_bytes_used());
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
