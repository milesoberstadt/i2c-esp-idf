#if !defined(__UTILS__)
#define __UTILS__

#include <stdio.h>

#include "WString.h"
#include "types.h"

String device_type_str(device_type_t type);
String device_state_str(device_state_t state);
String device_value_str(uint8_t *value, uint8_t value_size);

#endif // __UTILS__
