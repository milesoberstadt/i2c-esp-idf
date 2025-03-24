#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_wifi_types.h"

// Maximum AP list size
#define MAX_AP_LIST_SIZE 500

// Maximum I2C message size
#define I2C_DATA_LEN 64

// Message types
typedef enum {
    MSG_ERROR = 0x00,        // Error message
    MSG_HELLO = 0x01,        // Initial contact from DOM
    MSG_VERIFY = 0x02,       // Verification message with SUB id
    MSG_ASSIGN = 0x03,       // Assignment of new I2C and WiFi channel
    MSG_RESET = 0x04,        // Reset command for SUBs
    MSG_SET_TIME = 0x05,     // Set time from DOM to SUB
    MSG_START_SCAN = 0x06,   // Start WiFi scanning
    MSG_STOP_SCAN = 0x07,    // Stop WiFi scanning
    MSG_REQ_AP_COUNT = 0x08, // Request count of APs seen
    MSG_AP_COUNT = 0x09,     // Response with AP count
    MSG_REQ_AP_DATA = 0x0A,  // Request AP data record
    MSG_AP_DATA = 0x0B,      // Response with AP data
    MSG_CONFIRM_AP = 0x0C,   // Confirm AP data received
    MSG_EAPOL_DATA = 0x0D,   // EAPOL packet data
} message_type_t;

// AP record structure
typedef struct {
    uint8_t bssid[6];      // MAC address of the AP
    uint8_t ssid[33];      // SSID of the AP (32 chars + null terminator)
    int8_t rssi;           // Signal strength
    uint8_t channel;       // WiFi channel
    uint32_t timestamp;    // UNIX timestamp when seen
    bool synced;           // Whether this record has been synced to DOM
} ap_record_t;

// Message header structure
typedef struct {
    uint8_t msg_type;      // Message type (from message_type_t)
    uint8_t sub_id;        // SUB node identifier
    uint8_t data_len;      // Length of data payload
} __attribute__((packed)) message_header_t;

// Message structure for I2C communication
typedef struct {
    message_header_t header;
    uint8_t data[I2C_DATA_LEN - sizeof(message_header_t)];
} __attribute__((packed)) i2c_message_t;

// SUB node status
typedef enum {
    SUB_STATUS_UNINITIALIZED = 0,
    SUB_STATUS_INITIALIZED,
    SUB_STATUS_SCANNING,
    SUB_STATUS_PAUSED
} sub_status_t;

// SUB node configuration
typedef struct {
    char id[3];            // 2-character ID + null terminator
    uint8_t i2c_addr;      // I2C address
    uint8_t wifi_channel;  // WiFi channel to scan
    sub_status_t status;   // Current status
    uint32_t timestamp;    // Last synchronized UNIX timestamp
} sub_config_t;

void random_id_generate(char *id);

#endif // TYPES_H