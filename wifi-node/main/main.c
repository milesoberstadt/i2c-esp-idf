#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi.h"
#include "sd.h"
#include "constants.h"
#include "i2c_slave.h"

void app_main(void)
{
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(wifi_TAG, "ESP_WIFI_MODE_STA");
  wifi_init_sta();
  init_sd();
  i2c_slave_init();

  // create file on sd card containing wifi information
  char data[EXAMPLE_MAX_CHAR_SIZE];
  const char *path = MOUNT_POINT "/wifi.txt";
  snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "SSID: %s\nPassword: %s\n", ESP_WIFI_SSID, ESP_WIFI_PASS);
  ret = write_file(path, data);
  
  // write i2c data received to sd card
  while (1)
  {
    uint8_t data_received = i2c_slave_receive();
    snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "Data received: %d\n", data_received);
    ret = write_file(MOUNT_POINT "/i2c.txt", data);
    if (ret != ESP_OK)
    {
      return;
    }
  }
}