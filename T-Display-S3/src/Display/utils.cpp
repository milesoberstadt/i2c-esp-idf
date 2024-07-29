#include "utils.h"

String device_type_str(device_type_t type)
{

    switch (type)
    {
    case DEVICE_M_NODE:
        return "M-Node";
    case DEVICE_A_NODE:
        return "A-Node";
    case DEVICE_SLEEPER:
        return "Sleeper";
    default:
        return "Unknown";
    }

}

String device_state_str(device_state_t state)
{

    switch (state)
    {
    case dev_state_error:
        return "Error";
    case dev_state_connected:
        return "Connected";
    case dev_state_pairing:
        return "Pairing";
    case dev_state_disconnecting:
        return "Disconnecting ...";
    case dev_state_connecting:
        return "Connecting ...";
    default:
        return "Disconnected";
    }

}

String device_value_str(uint8_t *value, uint8_t value_size)
{

    String str = "";

    for (uint8_t i = 0; i < value_size; i++)
    {
        str += value[i];
        str += " ";
    }

    return str;

}