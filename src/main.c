#include "common.h"
#include "holomesh.h"
#include "matrix.h"
#include "wireframe.h"
#include "render.h"
#include "scratch.h"

char g_sample_craft_name_lower[256] = {0};

static const char* c_sample_craft_name = "RZ-1 A-WING INTERCEPTOR";
static const char* c_sample_craft_stats[] = {
    "LENGTH: 9.6M",
    "WIDTH: 6.48M",
    "HEIGHT: 3.11M",
    "MAX. ACCEL: 5,100G",
    "MGLT: 120 MGLT",
    "ATMOS. SPEED: 1,300KM/H",
    "ENGINE: NOVALDEX J-77 EVENT HORIZON",
    "HYPRDRV: INCOM GBk-785 CLASS 1.0",
    "POWER: MPS Bpr-99 FUSION",
    "SHIELDS: SIRPLEX Z-9",
    "SENSORS: FABRITECH ANs-7e",
    "NAV: LpL-449",
    "AVIONICS: TORPLEX Rq9.Z",
    "COUNTERMSRS: MIRADYNE 4x-PHANTOM",
    "ARMS: BORSTEL RG-9 LAS. CANNONS",
    "MISSILES: DYMEK HM-6 CONCUSSION",
    "CARGO CAP.: 40KG"
};
static const size_t c_sample_craft_stat_count = sizeof(c_sample_craft_stats) / sizeof(const char*);
    
static GColor c_palette[] = {
    {0b11000000},
    {0b11000101},
    {0b11001010},
    {0b11001111}
};

static const int c_refreshTimer = 100;
static const uint8_t c_logoSize = 36;

// UI elements
Window *my_window;
BitmapLayer *frameBufferLayer;
Layer* uiElementsLayer;
TextLayer* textLayer;
TextLayer* textLayerSym;
TextLayer* infoTextLayer;
BitmapLayer* rebelLogoLayer;
BitmapLayer* impLogoLayer;
BitmapLayer* bobaLogoLayer;
TextLayer* timeLayer;

// Global resources
GBitmap* frameBufferBitmap;
GBitmap* rebelLogoBitmap;
GBitmap* impLogoBitmap;
GBitmap* bobaLogoBitmap;

GFont g_font_sw;
GFont g_font_sw_symbol;
GFont g_font_time;
GFont g_font_info;

AppTimer* g_timer;

// Rendering stuff
holomesh_t* g_holomesh;

void load_holomesh(void) {
    ResHandle handle = resource_get_handle(RESOURCE_ID_HOLO_CORELLIAN_TRANSPORT);
    
    // Allocate space for the resource
    // TODO: estimate this better
    size_t size = resource_size(handle);
    g_holomesh = (holomesh_t*) malloc(size);
    
    // Load it
    size_t copied = resource_load(handle, (uint8_t*) g_holomesh, size);
    ASSERT(copied == g_holomesh->file_size);
    ASSERT(size >= g_holomesh->file_size);
    
    // Deserialize it
    holomesh_result hr = holomesh_deserialize(g_holomesh, size);
    ASSERT(hr == hmresult_ok);
    
    // Allocate scratch
    scratch_init(g_holomesh->scratch_size);
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "HOLOMESH: %u bytes, %u scratch size", 
            (unsigned) size,
            (unsigned) g_holomesh->scratch_size);
}

void set_pixel_on_row(const GBitmapDataRowInfo* row_info, int x, int color) {
    // TODO: this check is done outside this function so we could remove this too
    if (x >= row_info->min_x && x <= row_info->max_x) {
        int byte_offset = x >> 2; // divide by 4 to get actual pixel byte
        int bit_shift = (~x & 3) << 1;
        // TODO: remove & 3 here -- technically this is out of range and insane data
        row_info->data[byte_offset] |= (color & 3) << bit_shift;
    }
}

static int paint_tick = 0;
void paint(void) {
    int min_y = 30;
    for (int y = min_y; y < 144; ++y) {
        GBitmapDataRowInfo row_info = gbitmap_get_data_row_info(frameBufferBitmap, y);
        int byte_offset = row_info.min_x >> 2;
        for (int x = row_info.min_x; x < row_info.max_x; x += 4) {
            uint8_t color[4] = {
                (rand()) & 1,
                (rand()) & 1,
                (rand()) & 1,
                (rand()) & 1
            };
            
            if (x > 30 && x < 114) {
                color[0] += 1;
                color[1] += 1;
                color[2] += 1;
                color[3] += 1;
            }
            if (x > 50 && x < 94) {
                color[0] += 1;
                color[1] += 1;
                color[2] += 1;
                color[3] += 1;
            }
            
            uint8_t final = (color[0] << 6) | (color[1] << 4) | (color[2] << 2) | color[3];
            row_info.data[byte_offset++] = final;
        }
    }
    paint_tick++;
    
    layer_mark_dirty(bitmap_layer_get_layer(frameBufferLayer));
}

void wireframe_draw_line(void* user_ptr, int x0, int y0, int x1, int y1) {
    GContext* ctx = (GContext*) user_ptr;
    graphics_draw_line(ctx, GPoint(x0, y0), GPoint(x1, y1));
}

void update_display(Layer* layer, GContext* ctx) {
    graphics_context_set_antialiased(ctx, true);
    graphics_context_set_stroke_color(ctx, GColorElectricBlue);
    graphics_context_set_stroke_width(ctx, 1);
    
    scratch_clear();

    // Get the projection matrix_t
    matrix_t transform;
    static fix16_t angle = 0;

    render_create_3d_transform(
        &transform, 
        &g_holomesh->transforms[holomesh_transform_perspective_square_aspect], 
        angle);

    angle += fix16_one >> 4;

    // Transform the mesh
    viewport_t viewport;
    viewport_init(&viewport, 144, 144);

    vec3_t* transformed_vertices = (vec3_t*) scratch_alloc(scratch_size_bytes());

    render_draw_mesh_wireframe(
        ctx,
        &viewport,
        g_holomesh,
        &transform,
        transformed_vertices);
}

bool fade_text(TextLayer* layer, int fade_timer, bool fade_out) {
    uint8_t text_color;
    
    // Fade in the opposite direction?
    if (fade_out) {
        text_color = ~fade_timer & 3;
    } else {
        text_color = fade_timer & 3;
    }
    
    if (text_color == 0) {
        layer_set_hidden(text_layer_get_layer(layer), true);
    } else {
        layer_set_hidden(text_layer_get_layer(layer), false);
        GColor8 col = { .argb = 0b11000000 | (text_color << 4) | (text_color << 2)};
        text_layer_set_text_color(layer, col);
    }

    // Turn off the timer at the end of the animation
    return (fade_timer & 3) != 3;
}

bool fade_between_text(TextLayer* layer_a, TextLayer* layer_b, int fade_timer) {
    TextLayer* colorMe;
    uint8_t text_color;
    
    // Fade in the opposite direction depending on 
    // where we are in the animation
    if ((fade_timer & 4) == 0) {
        text_color = ~fade_timer & 3;
    } else {
        text_color = fade_timer & 3;
    }    
    
    // Which layer are we referring to?
    if (((fade_timer - 4) & 8) == 0) {
        colorMe = layer_b;   
    } else {
        colorMe = layer_a;
    }
    
    if (text_color == 0) {
        layer_set_hidden(text_layer_get_layer(colorMe), true);
    } else {
        layer_set_hidden(text_layer_get_layer(colorMe), false);
        GColor8 col = { .argb = 0b11000000 | (text_color << 4) | (text_color << 2)};
        text_layer_set_text_color(colorMe, col);
    }

    // Turn off the timer at the end of the animation
    return (fade_timer & 7) != 7;
}

static bool g_do_title_fade_timer = false;
static int g_title_fade_timer = 0;

void animation_timer_trigger(void* data) {
    // TODO: restore me: paint();
    
    // TODO: remove me
    layer_mark_dirty(uiElementsLayer);
    
    if (g_do_title_fade_timer) {
        g_do_title_fade_timer = fade_between_text(textLayer, textLayerSym, g_title_fade_timer++);
    }
    g_timer = app_timer_register(c_refreshTimer, animation_timer_trigger, NULL);    
}

size_t g_current_stat = 0;
void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    if (units_changed == SECOND_UNIT) {
        // TODO: restore me: layer_mark_dirty(uiElementsLayer);
        text_layer_set_text(infoTextLayer, c_sample_craft_stats[
            (g_current_stat++) % c_sample_craft_stat_count
        ]);
        
        layer_set_hidden(bitmap_layer_get_layer(rebelLogoLayer), ((g_current_stat) % 3) != 0);
        layer_set_hidden(bitmap_layer_get_layer(impLogoLayer),   ((g_current_stat) % 3) != 1);
        layer_set_hidden(bitmap_layer_get_layer(bobaLogoLayer),  ((g_current_stat) % 3) != 2);
        
        g_do_title_fade_timer = (g_current_stat % 4) == 0;
    }
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

void handle_init(void) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "INIT MEMORY: %u bytes used, %u bytes free", (unsigned) heap_bytes_used(), (unsigned) heap_bytes_free());
    
    load_holomesh();
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "UI MEMORY: %u bytes used, %u bytes free", (unsigned) heap_bytes_used(), (unsigned) heap_bytes_free());

    my_window = window_create();
    window_set_background_color(my_window, GColorBlack);
    
    g_font_sw = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_12));
    g_font_sw_symbol = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SYMBOL_12));
    g_font_time = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    g_font_info = fonts_get_system_font(FONT_KEY_GOTHIC_09);
    
    frameBufferLayer = bitmap_layer_create(GRect(0, 0, 144, 144));
    layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(frameBufferLayer));
    
    frameBufferBitmap = gbitmap_create_blank_with_palette(
        GSize(144,144),
        GBitmapFormat2BitPalette,
        c_palette,
        false);
    bitmap_layer_set_bitmap(frameBufferLayer, frameBufferBitmap);
    
    //TODO: restore me: paint();    
    
    uiElementsLayer = layer_create(GRect(0, 0, 144, 144));
    layer_add_child(window_get_root_layer(my_window), uiElementsLayer);
    layer_set_update_proc(uiElementsLayer, update_display);
    
    GRect layerSize = GRect(0, 0, 144, 168);
    const char* text = c_sample_craft_name;
    create_symbol_text(g_sample_craft_name_lower, sizeof(g_sample_craft_name_lower), text);
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "'%s' -> '%s'", text, g_sample_craft_name_lower);
    
    GSize textSize = graphics_text_layout_get_content_size(
        text,
        g_font_sw,
        layerSize,
        0,
        GTextAlignmentLeft);

    // Two small text layers
    GRect textRect = { GPoint(1, -3), textSize };
    textLayer = text_layer_create(textRect);
    text_layer_set_background_color(textLayer, GColorClear);
    text_layer_set_text_color(textLayer, GColorYellow);
    text_layer_set_font(textLayer, g_font_sw);
    text_layer_set_text(textLayer, text);
    layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(textLayer));
    
    textLayerSym = text_layer_create(textRect);
    text_layer_set_background_color(textLayerSym, GColorClear);
    text_layer_set_text_color(textLayerSym, GColorYellow);
    text_layer_set_font(textLayerSym, g_font_sw_symbol);
    text_layer_set_text(textLayerSym, g_sample_craft_name_lower);
    layer_set_hidden(text_layer_get_layer(textLayerSym), true);
    layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(textLayerSym));
    
    // Info text layer
    int infoTop = textRect.origin.y + textRect.size.h;
    GRect infoTextRect = { GPoint(1, infoTop), GSize(144, 168 - c_logoSize - infoTop) };
    infoTextLayer = text_layer_create(infoTextRect);
    text_layer_set_background_color(infoTextLayer, GColorClear);
    text_layer_set_text_color(infoTextLayer, GColorYellow);
    text_layer_set_font(infoTextLayer, g_font_info);
    text_layer_set_text(infoTextLayer, c_sample_craft_stats[0]);
    layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(infoTextLayer));
    
    // Load logo bitmap
    rebelLogoBitmap = gbitmap_create_with_resource(RESOURCE_ID_REBEL_LOGO);
    impLogoBitmap = gbitmap_create_with_resource(RESOURCE_ID_IMPERIAL_LOGO);
    bobaLogoBitmap = gbitmap_create_with_resource(RESOURCE_ID_BOUNTY_HUNGER_LOGO);

    // Rebel logo
    GRect logoRect = GRect(144 - c_logoSize, 168 - c_logoSize, c_logoSize, c_logoSize);
    rebelLogoLayer = bitmap_layer_create(logoRect);
    bitmap_layer_set_bitmap(rebelLogoLayer, rebelLogoBitmap);
    bitmap_layer_set_compositing_mode(rebelLogoLayer, GCompOpSet);
    layer_set_hidden(bitmap_layer_get_layer(rebelLogoLayer), true);
    layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(rebelLogoLayer));

    // Imperial logo
    logoRect = GRect(144 - c_logoSize, 168 - c_logoSize, c_logoSize, c_logoSize);
    impLogoLayer = bitmap_layer_create(logoRect);
    bitmap_layer_set_bitmap(impLogoLayer, impLogoBitmap);
    bitmap_layer_set_compositing_mode(impLogoLayer, GCompOpSet);
    layer_set_hidden(bitmap_layer_get_layer(impLogoLayer), true);
    layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(impLogoLayer));
    
    // Bounty hunter logo
    logoRect = GRect(144 - c_logoSize, 168 - c_logoSize, c_logoSize, c_logoSize);
    bobaLogoLayer = bitmap_layer_create(logoRect);
    bitmap_layer_set_bitmap(bobaLogoLayer, bobaLogoBitmap);
    bitmap_layer_set_compositing_mode(bobaLogoLayer, GCompOpSet);
    layer_set_hidden(bitmap_layer_get_layer(bobaLogoLayer), true);
    layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(bobaLogoLayer));
    
    // Time
    GSize timeSize = graphics_text_layout_get_content_size(
        "0:APM",
        g_font_time,
        layerSize,
        0,
        GTextAlignmentLeft);
    GRect timeRect = { GPoint(0, 168 - timeSize.h), GSize(144, timeSize.h) };
    timeLayer = text_layer_create(timeRect);
    text_layer_set_background_color(timeLayer, GColorClear);
    text_layer_set_text_color(timeLayer, GColorYellow);
    text_layer_set_font(timeLayer, g_font_time);
    text_layer_set_text(timeLayer, "00:00 AM");
    layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(timeLayer));
    
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "FINAL MEMORY: %u bytes used, %u bytes free", (unsigned) heap_bytes_used(), (unsigned) heap_bytes_free());
    
    window_stack_push(my_window, true);
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
    
    g_timer = app_timer_register(c_refreshTimer, animation_timer_trigger, NULL);
}

void handle_deinit(void) {
  bitmap_layer_destroy(frameBufferLayer);
    // TODO
  window_destroy(my_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
