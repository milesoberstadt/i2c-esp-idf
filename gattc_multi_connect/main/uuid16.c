#include "uuid16.h"

unsigned int size = 16;

// Convert a UUID string to a byte array
void uuid_str_to_bytes(const char* uuid_str, uint8_t* uuid_bytes) {
    const char* pos = uuid_str;
    for (size_t i = 0; i < size; i++) {
        sscanf(pos, "%2hhx", &uuid_bytes[i]);
        pos += 2;
        if (*pos == '-') {
            pos++;
        }
    }
}

// Extract service UUID from advertisement data
bool get_adv_service_uuid(uint8_t *adv_data, uint8_t adv_data_len, uint8_t *service_uuid) {
    uint8_t i = 0;
    while (i < adv_data_len) {
        uint8_t length = adv_data[i];
        uint8_t type = adv_data[i + 1];

        if (type == 0x06 || type == 0x07) { // 0x06: 128-bit UUID, 0x07: Complete List of 128-bit UUIDs
            // Extract UUID in reverse order
            for (int j = 0; j < size; j++) {
                service_uuid[j] = adv_data[i + 2 + 15 - j];
            }
            return true;
        }
        i += length + 1; // Move to the next element
    }
    return false;
}

bool compare_uuid(const uint8_t *uuid1, const uint8_t *uuid2) {
    return memcmp(uuid1, uuid2, size) == 0;
}