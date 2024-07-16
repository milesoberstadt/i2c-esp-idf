#include "preferences.h"

nvs_handle_t handle;

bool init_preferences() {

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    err = nvs_open(PREFERENCES_PARTITION, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(PREFERENCES_TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return false;
    } 

    ESP_LOGI(PREFERENCES_TAG, "Preferences initialized");

    return true;
}

size_t put_int(const char *key, int32_t value) {
    esp_err_t err = nvs_set_i32(handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(PREFERENCES_TAG, "Error (%s) writing int to NVS!\n", esp_err_to_name(err));
        return 0;
    }

    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGE(PREFERENCES_TAG, "Error (%s) committing int to NVS!\n", esp_err_to_name(err));
        return 0;
    }

    return sizeof(value);
}

int32_t get_int(const char *key, int32_t default_value) {
    int32_t value = default_value;
    if (!key) {
        return value;
    }
    esp_err_t err = nvs_get_i32(handle, key, &value);
    if (err) {
        ESP_LOGE(PREFERENCES_TAG, "Error (%s) reading int from NVS!\n", esp_err_to_name(err));
    }
    return value;
}

size_t put_bytes(const char *key, const void *value, size_t len) {
  if (!key || !value || !len) {
    return 0;
  }
  esp_err_t err = nvs_set_blob(handle, key, value, len);
  if (err) {
    ESP_LOGE(PREFERENCES_TAG, "Error (%s) writing bytes to NVS!\n", esp_err_to_name(err));
    return 0;
  }
  err = nvs_commit(handle);
  if (err) {
    ESP_LOGE(PREFERENCES_TAG, "Error (%s) committing bytes to NVS!\n", esp_err_to_name(err));
    return 0;
  }
  return len;
}

size_t get_bytes(const char *key, void *buf, size_t maxLen) {
  size_t len = get_bytes_length(key);
  if (!len || !buf || !maxLen) {
    return len;
  }
  if (len > maxLen) {
    ESP_LOGE(PREFERENCES_TAG, "Buffer too small to read bytes from NVS!\n");
    return 0;
  }
  esp_err_t err = nvs_get_blob(handle, key, buf, &len);
  if (err) {
    ESP_LOGE(PREFERENCES_TAG, "Error (%s) reading bytes from NVS!\n", esp_err_to_name(err));
    return 0;
  }
  return len;
}

size_t get_bytes_length(const char *key) {
  size_t len = 0;
  if (!key) {
    return 0;
  }
  esp_err_t err = nvs_get_blob(handle, key, NULL, &len);
  if (err) {
    ESP_LOGE(PREFERENCES_TAG, "Error (%s) getting bytes length from NVS!\n", esp_err_to_name(err));
    return 0;
  }
  return len;
}

bool remove_preference(const char *key) {
  if (!key) {
    return false;
  }
  esp_err_t err = nvs_erase_key(handle, key);
  if (err) {
    ESP_LOGE(PREFERENCES_TAG, "Error (%s) removing value from NVS!\n", esp_err_to_name(err));
    return false;
  }
  err = nvs_commit(handle);
  if (err) {
    ESP_LOGE(PREFERENCES_TAG, "Error (%s) committing value to NVS!\n", esp_err_to_name(err));
    return false;
  }
  return true;
}

PreferenceType getType(const char *key) {
  if (!key || strlen(key) > 15) {
    return PT_INVALID;
  }
  int8_t mt1;
  uint8_t mt2;
  int16_t mt3;
  uint16_t mt4;
  int32_t mt5;
  uint32_t mt6;
  int64_t mt7;
  uint64_t mt8;
  size_t len = 0;
  if (nvs_get_i8(handle, key, &mt1) == ESP_OK) {
    return PT_I8;
  }
  if (nvs_get_u8(handle, key, &mt2) == ESP_OK) {
    return PT_U8;
  }
  if (nvs_get_i16(handle, key, &mt3) == ESP_OK) {
    return PT_I16;
  }
  if (nvs_get_u16(handle, key, &mt4) == ESP_OK) {
    return PT_U16;
  }
  if (nvs_get_i32(handle, key, &mt5) == ESP_OK) {
    return PT_I32;
  }
  if (nvs_get_u32(handle, key, &mt6) == ESP_OK) {
    return PT_U32;
  }
  if (nvs_get_i64(handle, key, &mt7) == ESP_OK) {
    return PT_I64;
  }
  if (nvs_get_u64(handle, key, &mt8) == ESP_OK) {
    return PT_U64;
  }
  if (nvs_get_str(handle, key, NULL, &len) == ESP_OK) {
    return PT_STR;
  }
  if (nvs_get_blob(handle, key, NULL, &len) == ESP_OK) {
    return PT_BLOB;
  }
  return PT_INVALID;
}

bool isKey(const char *key) {
  return getType(key) != PT_INVALID;
}
