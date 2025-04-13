#ifndef __SD_H__
#define __SD_H__

#include <string.h>
#include <stdbool.h>

#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include "constants.h"

// return the handle of the opened file. -1 if error
uint8_t sd_open_file(const char *path);

// close the file with the given handle and return true if successful
bool sd_close_file(uint8_t handle);

// write a new line on a given file
bool sd_write_line(uint8_t handle, char* buffer, size_t len);

void init_sd();

#endif // __SD_H__