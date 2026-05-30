#pragma once
#include <3ds.h>

extern int pngWidth, pngHeight, bytesPerPixel;

bool read_png_file(char* file_name);
u8 * process_png_file(void);