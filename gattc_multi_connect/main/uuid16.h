#if !defined(__UUID16_H__)
#define __UUID16_H__

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

void uuid_str_to_bytes(const char* uuid_str, uint8_t* uuid_bytes);

bool get_adv_service_uuid(uint8_t *adv_data, uint8_t adv_data_len, uint8_t *service_uuid);

bool compare_uuid(const uint8_t *uuid1, const uint8_t *uuid2);

#endif // __UUID16_H__
