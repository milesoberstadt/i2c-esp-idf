#include "ui.h"

size_t selected_device = 0;

size_t get_selected_device() {
    return selected_device;
}

void switch_selected_device() {
    selected_device = (selected_device + 1) % PROFILE_NUM;
}