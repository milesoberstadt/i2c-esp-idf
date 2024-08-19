#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi.h"
#include "sd.h"
#include "constants.h"
#include "i2c_slave.h"
#include "i2c_messages.h"
#include "display.h"
#include "esp_log.h"

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
  // wifi_init_sta();
  init_sd();
  init_display();
  i2c_slave_init();

  display_text(" Ready !", 9);
  
  // write i2c data received to sd card
  uint8_t data_received[I2C_DATA_LEN];
  for (;;) {

    i2c_receive(data_received);
    esp_log_buffer_hex("MAIN", data_received, I2C_DATA_LEN);
    // process_message(&data_received, sizeof(uint8_t));

  }
}