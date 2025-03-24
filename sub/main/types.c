#include "types.h"
#include <stdlib.h>
#include <time.h>
#include "esp_random.h"

void random_id_generate(char *id) {
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    uint32_t random_val = esp_random();
    
    // Generate two random characters from the charset
    id[0] = charset[random_val % sizeof(charset)];
    id[1] = charset[(random_val >> 8) % sizeof(charset)];
    id[2] = '\0'; // Null terminator
}