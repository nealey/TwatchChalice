#include <pebble.h>

#define HAND_OUT 200
#define HAND_IN 50
#define MINUTE_WIDTH 10
#define HOUR_WIDTH 12
#define BORDER_WIDTH 12
#define HBW (BORDER_WIDTH / 2)

static Window *window;
static Layer *s_bg_layer, *s_hands_layer;
static TextLayer *s_day_label, *s_bt_label;
static GColor accent_color;

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
  // Draw Background, which is just blackness
  graphics_context_set_fill_color(ctx, GColorBlack);
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
  graphics_context_set_stroke_color(ctx, GColorWhite);
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

  strftime(s_day_buffer, sizeof(s_day_buffer), "%d %b", t);
  if (b[0] == '0') {
    b += 1;
  }
  text_layer_set_text(s_day_label, b);
}


static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_hands_layer);
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
  layer_add_child(window_layer, s_hands_layer);
 
  // Day
#ifdef PBL_ROUND
  s_day_label = text_layer_create(GRect(75, 75, 30, 24));
#else
  //s_day_label = text_layer_create(GRect(92, 152, 50, 16));
  s_day_label = text_layer_create(GRect(88, -3, 50, 16));
#endif
  text_layer_set_font(s_day_label, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_day_label, GTextAlignmentRight);
  text_layer_set_text(s_day_label, s_day_buffer);
  text_layer_set_background_color(s_day_label, GColorClear);
  text_layer_set_text_color(s_day_label, GColorBlack);
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
  text_layer_set_text_color(s_bt_label, GColorDarkGray);
  text_layer_set_font(s_bt_label, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_SYMBOLS_64)));
  layer_add_child(s_bg_layer, text_layer_get_layer(s_bt_label));
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
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);

  s_day_buffer[0] = '\0';
  
  // Pick out a color
  accent_color = (GColor8){ .argb = ((rand() % 0b00111111) + 0b11000000)};
  //accent_color = GColorVividCerulean;

  Layer *window_layer = window_get_root_layer(window);
  display_bounds = layer_get_bounds(window_layer);
  center = grect_center_point(&display_bounds);

  tick_subscribe();
  
  bluetooth_connection_service_subscribe(bt_handler);
  bt_connected = bluetooth_connection_service_peek();
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
