#include <pebble.h>
#include "settings.h"

static void (*callback)(void);

GColor settings_get_color(MessageKey key) {
  if (! persist_exists(key)) {
    return GColorBlack;
  }
  
  return GColorFromHEX(persist_read_int(key));
}

int32_t settings_get_int32(MessageKey key) {
  if (! persist_exists(key)) {
    return 0;
  }
  
  return persist_read_int(key);
}

static void in_received_handler(DictionaryIterator *rec, void *context) {
  int i;
  
  for (i = 0; i < KEY_SENTRY; i += 1) {
    Tuple *cur = dict_find(rec, i);
    
    if (! cur) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Holy crap! Key %i isn't around!", i);
      continue;
    }

    persist_write_int(i, cur->value->int32);
  }
  
  callback();
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
  // XXX: I don't understand what we could possibly do here
}

void settings_init(void (*cb)(void)) {
  callback = cb;
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_open(256, 64);
}