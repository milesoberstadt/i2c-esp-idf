#include "utils.h"

size_t device_type_str(device_type_t type, char* buffer, size_t buffer_size)
{
    const char* str;

    switch (type)
    {
    case DEVICE_M_NODE:
        str = "M-Node";
        break;
    case DEVICE_A_NODE:
        str = "A-Node";
        break;
    case DEVICE_SLEEPER:
        str = "Sleeper";
        break;
    default:
        str = "Unknown";
        break;
    }

    size_t len = snprintf(buffer, buffer_size, "%s", str);

    if (len >= buffer_size)
    {
        // Handle buffer overflow
        return -1;
    }

    return len;
}

size_t device_state_str(device_state_t state, char* buffer, size_t buffer_size)
{
    const char* str;

    switch (state)
    {
    case dev_state_error:
        str = "Error";
        break;
    case dev_state_connected:
        str = "Connected";
        break;
    case dev_state_pairing:
        str = "Pairing";
        break;
    case dev_state_disconnecting:
        str = "Disconnecting ...";
        break;
    case dev_state_connecting:
        str = "Connecting ...";
        break;
    default:
        str = "Disconnected";
        break;
    }

    size_t len = snprintf(buffer, buffer_size, "%s", str);

    if (len >= buffer_size)
    {
        // Handle buffer overflow
        return -1;
    }

    return len;
}

size_t device_value_str(uint8_t* value, uint8_t value_size, char* buffer, size_t buffer_size)
{
    size_t len = 0;

    for (uint8_t i = 0; i < value_size; i++)
    {
        size_t temp_len = snprintf(buffer + len, buffer_size - len, "%d ", value[i]);

        if (temp_len >= buffer_size - len)
        {
            // Handle buffer overflow
            return -1;
        }

        len += temp_len;
    }

    return len;
}