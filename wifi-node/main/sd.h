#ifndef __SD_H__
#define __SD_H__

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "constants.h"

esp_err_t write_file(const char *path, char *data);
void init_sd();


#endif // __SD_H__