#include <pebble.h>
#include "settings.h"

static int HAND_OUT = 200;
static int HAND_IN = 40;
static int MINUTE_WIDTH = 10;
static int HOUR_WIDTH = 12;
static int BORDER_WIDTH_BASE = 14;
static int FONT_SIZE = 16;

#define BORDER_WIDTH (BORDER_WIDTH_BASE + PBL_IF_ROUND_ELSE(4, 0))
#define HBW (BORDER_WIDTH / 2)

static Window *window;
static Layer *s_bg_layer, *s_hands_layer;
static TextLayer *s_day_label, *s_bt_label;
static GColor face_color;
static GColor accent_color;
static GColor hand_color;

static char s_day_buffer[15];

GRect display_bounds;
GPoint center;

bool bt_connected;

static GPoint point_of_polar(int32_t theta, int r) {
  GPoint ret = {
    .x = (int16_t)(sin_lookup(theta) * r / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(theta) * r / TRIG_MAX_RATIO) + center.y,
  };
  
  return ret;
}

static void bg_update_proc(Layer *layer, GContext *ctx) {
  // Draw Background, which is just a solid color
  graphics_context_set_fill_color(ctx, face_color);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  
  if (bt_connected) {
    text_layer_set_text(s_bt_label, "");
  } else {
    text_layer_set_text(s_bt_label, "");
  }
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  char *b = s_day_buffer;
  
  // hour hand
  graphics_context_set_stroke_width(ctx, HOUR_WIDTH);
  graphics_context_set_stroke_color(ctx, hand_color);
  graphics_draw_line(ctx,
                     point_of_polar(TRIG_MAX_ANGLE * (t->tm_hour % 12) / 12, HAND_IN),
                     point_of_polar(TRIG_MAX_ANGLE * (t->tm_hour % 12) / 12, HAND_OUT));
  
  // minute hand
  graphics_context_set_stroke_width(ctx, MINUTE_WIDTH);
  graphics_context_set_stroke_color(ctx, accent_color);
  graphics_draw_line(ctx,
                     center,
                     point_of_polar(TRIG_MAX_ANGLE * t->tm_min / 60, HAND_OUT));

  // border
#ifdef PBL_ROUND
  GRect frame = grect_inset(display_bounds, GEdgeInsets(0));
  graphics_context_set_fill_color(ctx, accent_color);
  graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, BORDER_WIDTH, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
#else
  graphics_context_set_stroke_width(ctx, BORDER_WIDTH);
  GPoint a;
  a.x = display_bounds.origin.x + HBW;
  a.y = display_bounds.origin.y + HBW;
  for (int i = 0; i < 4; i += 1) {
    GPoint b = a;
    switch (i) {
    case 0:
      b.x = display_bounds.size.w - HBW;
      break;
    case 1:
      b.y = display_bounds.size.h - HBW;
      break;
    case 2:
      b.x = display_bounds.origin.x + HBW;
      break;
    case 3:
      b.y = display_bounds.origin.y + HBW;
      break;
    }
    graphics_draw_line(ctx, a, b);
    a = b;
  }
#endif

  strftime(s_day_buffer, sizeof(s_day_buffer), "%d %b", t);
  if (b[0] == '0') {
    b += 1;
  }
  text_layer_set_text(s_day_label, b);
}


static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_hands_layer);
}

static void set_configurables() {
  face_color = settings_get_color(KEY_COLOR_FACE);
  layer_mark_dirty(s_bg_layer);
  
#ifdef PBL_COLOR
  do {
    uint8_t argb = rand() % 0b00111111;
    accent_color = (GColor8){ .argb = argb | 0b11000000 };
  } while (gcolor_equal(accent_color, face_color));
#else
  accent_color = gcolor_legible_over(face_color);
#endif
  hand_color = gcolor_legible_over(face_color);
  layer_mark_dirty(s_hands_layer);

  text_layer_set_text_color(s_day_label, gcolor_legible_over(accent_color));
  text_layer_set_text_color(s_bt_label, gcolor_legible_over(face_color));
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Style: %ld", settings_get_int32(KEY_STYLE));
  switch (settings_get_int32(KEY_STYLE)) {
  case 1: // Thin style
    HAND_IN = 45;
    BORDER_WIDTH_BASE = 12;
    MINUTE_WIDTH = 10;
    HOUR_WIDTH = 12;
    text_layer_set_font(s_day_label, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    text_layer_set_text_alignment(s_day_label, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentRight));
    break;
  default:
    HAND_IN = 40;
    BORDER_WIDTH_BASE = 16;
    MINUTE_WIDTH = 12;
    HOUR_WIDTH = 14;
    text_layer_set_font(s_day_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(s_day_label, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft));
    break;
  }
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Background
  s_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_bg_layer);
  
  // Hands
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(s_bg_layer, s_hands_layer);
 
  // Day
  s_day_label = text_layer_create(GRect(bounds.origin.x + 3, 
                                        PBL_IF_ROUND_ELSE(0, -4), 
                                        bounds.size.w - 6,
                                        24));
  text_layer_set_text(s_day_label, s_day_buffer);
  text_layer_set_background_color(s_day_label, GColorClear);
  layer_add_child(s_hands_layer, text_layer_get_layer(s_day_label));

  // Missing phone
#ifdef PBL_ROUND
  s_bt_label = text_layer_create(GRect(26, 50, 52, 64));
#else
  s_bt_label = text_layer_create(GRect(10, 42, 52, 64));
#endif
  text_layer_set_text_alignment(s_bt_label, GTextAlignmentCenter);
  text_layer_set_text(s_bt_label, "");
  text_layer_set_background_color(s_bt_label, GColorClear);
  text_layer_set_font(s_bt_label, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_SYMBOLS_64)));
  layer_add_child(s_bg_layer, text_layer_get_layer(s_bt_label));
  
  set_configurables();
}

static void window_unload(Window *window) {
  layer_destroy(s_bg_layer);
  layer_destroy(s_hands_layer);

  text_layer_destroy(s_day_label);
}

static void bt_handler(bool connected) {
  bt_connected = connected;
  if (! connected) {
    vibes_double_pulse();
  }
  layer_mark_dirty(s_bg_layer);
}

static void tick_subscribe() {
  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
}

static void init() {  
  // Pick out a color
  face_color = GColorBlack;
  
  
  s_day_buffer[0] = '\0';

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);

  Layer *window_layer = window_get_root_layer(window);
  display_bounds = layer_get_bounds(window_layer);
  center = grect_center_point(&display_bounds);

  tick_subscribe();
  
  bluetooth_connection_service_subscribe(bt_handler);
  bt_connected = bluetooth_connection_service_peek();
  
  settings_init(set_configurables);
}

static void deinit() {
  // XXX: text destroy?

  tick_timer_service_unsubscribe();
  window_destroy(window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
