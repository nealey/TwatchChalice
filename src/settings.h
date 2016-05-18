#include <pebble.h>
#pragma once

typedef enum {
  KEY_COLOR_FACE = 0,
  KEY_SENTRY
} MessageKey;

GColor settings_get_color(MessageKey idx);
void settings_init(void (*cb)(void));