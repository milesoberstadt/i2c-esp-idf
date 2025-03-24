#ifndef I2C_MASTER_H
#define I2C_MASTER_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "types.h"

// Random I2C address range (for initial discovery)
#define I2C_ADDR_RANDOM_MIN 20
#define I2C_ADDR_RANDOM_MAX 127

// Reserved I2C address range (for DOM to assign to SUBs)
#define I2C_ADDR_RESERVED_MIN 1
#define I2C_ADDR_RESERVED_MAX 19

// Initialize I2C master
esp_err_t i2c_master_init(void);

// Scan I2C bus for devices
esp_err_t i2c_master_scan(uint8_t start_addr, uint8_t end_addr, uint8_t *found_addrs, uint8_t *num_found);

// Send a message to a SUB
esp_err_t i2c_master_send(uint8_t addr, message_type_t msg_type, uint8_t sub_id, const uint8_t *data, uint8_t data_len);

// Read response from a SUB
esp_err_t i2c_master_read_msg(uint8_t addr, i2c_message_t *response);

// Send "hello" message to discover SUBs
esp_err_t i2c_master_send_hello(uint8_t addr);

// Send verification message to a SUB
esp_err_t i2c_master_send_verify(uint8_t addr, const char *id_str);

// Assign an I2C address, WiFi channel, and SUB ID to a SUB
esp_err_t i2c_master_send_assign(uint8_t old_addr, uint8_t new_addr, uint8_t wifi_channel, uint8_t sub_id);

// Send reset command to a SUB
esp_err_t i2c_master_send_reset(uint8_t addr);

// Set the timestamp on a SUB
esp_err_t i2c_master_set_time(uint8_t addr, uint8_t sub_id, uint32_t timestamp);

// Start scanning on a SUB
esp_err_t i2c_master_start_scan(uint8_t addr, uint8_t sub_id);

// Stop scanning on a SUB
esp_err_t i2c_master_stop_scan(uint8_t addr, uint8_t sub_id);

// Request AP count from a SUB
esp_err_t i2c_master_req_ap_count(uint8_t addr, uint8_t sub_id, uint16_t *count);

// Request AP data from a SUB
esp_err_t i2c_master_req_ap_data(uint8_t addr, uint8_t sub_id, ap_record_t *record, bool *has_more);

// Confirm AP data was received and processed
esp_err_t i2c_master_confirm_ap(uint8_t addr, uint8_t sub_id, const uint8_t *bssid);

#endif // I2C_MASTER_H