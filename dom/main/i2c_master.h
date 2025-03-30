#ifndef I2C_MASTER_H
#define I2C_MASTER_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "types.h"

// I2C Master configuration
#define I2C_MASTER_SCL_IO           22  // GPIO pin for SCL
#define I2C_MASTER_SDA_IO           21  // GPIO pin for SDA
#define I2C_MASTER_PORT             I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000  // 100kHz
#define I2C_MASTER_TIMEOUT_MS       1000    // 1 second timeout

// Fixed I2C address for the SUB node
#define I2C_FIXED_SUB_ADDR          0x42

// I2C address ranges
#define I2C_ADDR_RANDOM_MIN         0x20    // Start of random address range
#define I2C_ADDR_RANDOM_MAX         0x27    // End of random address range
#define I2C_ADDR_RESERVED_MIN       0x40    // Start of reserved address range
#define I2C_ADDR_RESERVED_MAX       0x47    // End of reserved address range

// I2C message header and data sizes
#define I2C_HEADER_LEN              3     // msg_type (1) + sub_id (1) + data_len (1)
#define I2C_MESSAGE_DATA_LEN        (I2C_DATA_LEN - I2C_HEADER_LEN)

#define I2C_MASTER_TAG              "i2c_master"

// Initialize I2C master
esp_err_t i2c_master_init(void);

// Send a message to a SUB node
esp_err_t i2c_master_send(uint8_t addr, message_type_t msg_type, uint8_t sub_id, const uint8_t *data, uint8_t data_len);

// Read a message from a SUB node
esp_err_t i2c_master_read(uint8_t addr, i2c_message_t *response);

// Alias for i2c_master_read for compatibility with existing code
esp_err_t i2c_master_read_msg(uint8_t addr, i2c_message_t *response);

// Send "hello" message to check SUB availability
esp_err_t i2c_master_send_hello(uint8_t addr);

// Send verification message to a SUB
esp_err_t i2c_master_send_verify(uint8_t addr, const char *id_str);

// Assign WiFi channel and SUB ID to a SUB
esp_err_t i2c_master_send_assign(uint8_t addr, uint8_t new_addr, uint8_t wifi_channel, uint8_t sub_id);

// Send reset command to a SUB
esp_err_t i2c_master_send_reset(uint8_t addr);

// Set the timestamp on a SUB
esp_err_t i2c_master_set_time(uint8_t addr, uint8_t sub_id, uint32_t timestamp);

// Start WiFi scanning on a SUB
esp_err_t i2c_master_start_scan(uint8_t addr, uint8_t sub_id);

// Stop WiFi scanning on a SUB
esp_err_t i2c_master_stop_scan(uint8_t addr, uint8_t sub_id);

// Request AP count from a SUB
esp_err_t i2c_master_req_ap_count(uint8_t addr, uint8_t sub_id, uint16_t *count);

// Request AP data from a SUB
esp_err_t i2c_master_req_ap_data(uint8_t addr, uint8_t sub_id, ap_record_t *record, bool *has_more);

// Confirm AP data was received and processed
esp_err_t i2c_master_confirm_ap(uint8_t addr, uint8_t sub_id, const uint8_t *bssid);

#endif // I2C_MASTER_H