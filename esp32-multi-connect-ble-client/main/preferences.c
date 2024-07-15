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
        ESP_LOGE(PREFERENCES_TAG, "Error (%s) writing value to NVS!\n", esp_err_to_name(err));
        return 0;
    }

    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGE(PREFERENCES_TAG, "Error (%s) committing value to NVS!\n", esp_err_to_name(err));
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
        ESP_LOGE(PREFERENCES_TAG, "Error (%s) reading value from NVS!\n", esp_err_to_name(err));
    }
    return value;
}

size_t put_bytes(const char *key, const void *value, size_t len) {
  if (!key || !value || !len) {
    return 0;
  }
  esp_err_t err = nvs_set_blob(handle, key, value, len);
  if (err) {
    ESP_LOGE(PREFERENCES_TAG, "Error (%s) writing value to NVS!\n", esp_err_to_name(err));
    return 0;
  }
  err = nvs_commit(handle);
  if (err) {
    ESP_LOGE(PREFERENCES_TAG, "Error (%s) committing value to NVS!\n", esp_err_to_name(err));
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
    ESP_LOGE(PREFERENCES_TAG, "Buffer too small to read value from NVS!\n");
    return 0;
  }
  esp_err_t err = nvs_get_blob(handle, key, buf, &len);
  if (err) {
    ESP_LOGE(PREFERENCES_TAG, "Error (%s) reading value from NVS!\n", esp_err_to_name(err));
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
    ESP_LOGE(PREFERENCES_TAG, "Error (%s) reading value from NVS!\n", esp_err_to_name(err));
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