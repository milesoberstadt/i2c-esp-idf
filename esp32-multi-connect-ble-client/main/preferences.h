#if !defined(__PREFERENCES_H__)
#define __PREFERENCES_H__

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_check.h"

#include "constants.h"

#define PREFERENCES_TAG "PREFERENCES"

typedef enum {
  PT_I8,
  PT_U8,
  PT_I16,
  PT_U16,
  PT_I32,
  PT_U32,
  PT_I64,
  PT_U64,
  PT_STR,
  PT_BLOB,
  PT_INVALID
} PreferenceType;

bool init_preferences();

size_t put_int(const char *key, int32_t value);

int32_t get_int(const char *key, int32_t default_value);

size_t put_bytes(const char *key, const void *value, size_t len);

size_t get_bytes(const char *key, void *buf, size_t maxLen);

size_t get_bytes_length(const char *key);

bool remove_preference(const char *key);

PreferenceType getType(const char *key);

bool isKey(const char *key);

#endif // __PREFERENCES_H__
