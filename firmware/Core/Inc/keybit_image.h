/* Keybit firmware image (firmware/keybit), flashed onto Keybits over SWIO. */
#ifndef KEYBIT_IMAGE_H
#define KEYBIT_IMAGE_H

#include <stdint.h>

extern const uint8_t keybit_image[];
extern const uint32_t keybit_image_len;
extern const uint8_t keybit_image_version;  /* FW_VERSION in keybit/main.c */

#endif
