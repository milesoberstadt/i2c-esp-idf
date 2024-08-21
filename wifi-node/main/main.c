#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi.h"
#include "sd.h"
#include "constants.h"
#include "i2c_slave.h"
#include "i2c_messages.h"
#include "display.h"
#include "esp_log.h"
#include "devices.h"

#define MAIN "main"

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

  // wifi_init_sta();
  init_sd();
  init_display();
  i2c_slave_init();
  init_devices();

  display_text(" Ready !", 9, 1);
  
  i2c_start_receive();

  ESP_LOGI(MAIN, "Initialization complete. Getting devices data...");

  for (int i = 0; i < DEVICES_COUNT; i++)
  {
    i2c_send_message(msg_req_dev, i);
  }

}