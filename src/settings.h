#include <pebble.h>
#pragma once

typedef enum {
  KEY_COLOR_FACE = 0,
  KEY_STYLE,
  KEY_SENTRY
} MessageKey;

GColor settings_get_color(MessageKey idx);
int32_t settings_get_int32(MessageKey key);
void settings_init(void (*cb)(void));