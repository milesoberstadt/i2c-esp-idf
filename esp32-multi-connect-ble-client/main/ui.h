#if !defined(__UI_H__)
#define __UI_H__

#include <stdio.h>

#include "constants.h"
#include "led.h"

#define UI_TAG "UI"

size_t get_selected_device();

void switch_selected_device();

#endif // __UI_H__
