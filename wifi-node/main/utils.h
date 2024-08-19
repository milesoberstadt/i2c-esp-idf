#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

size_t device_type_str(device_type_t type, char* buffer, size_t buffer_size);

size_t device_state_str(device_state_t state, char* buffer, size_t buffer_size);

size_t device_value_str(uint8_t* value, char* buffer, size_t buffer_size);
