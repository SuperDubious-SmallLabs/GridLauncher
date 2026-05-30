/*
 * Copyright 2002-2010 Guillaume Cottenceau.
 *
 * This software may be freely redistributed under the terms
 * of the X11 license.
 *
 */

#include "pngloader.h"
#include "filesystem.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define PNG_DEBUG 3
#include <png.h>

int x, y;

int pngWidth, pngHeight;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep * row_pointers;

int bytesPerPixel;

typedef struct {
    const u8 * data;
    size_t size;
    size_t pos;
} pngMemSource;

static void pngReadFromMem(png_structp png_ptr_arg, png_bytep out, png_size_t bytes) {
    pngMemSource * src = (pngMemSource *)png_get_io_ptr(png_ptr_arg);
    if (src->pos + bytes > src->size) {
        png_error(png_ptr_arg, "PNG read past end of buffer");
        return;
    }
    memcpy(out, src->data + src->pos, bytes);
    src->pos += bytes;
}

bool read_png_file(char* file_name) {
    FS_Path fsPath = fsMakePath(PATH_ASCII, file_name);
    Handle fileHandle;
    Result ret = FSUSER_OpenFile(&fileHandle, sdmcArchive, fsPath, FS_OPEN_READ, 0);
    if (R_FAILED(ret)) return false;

    u64 fileSize64 = 0;
    FSFILE_GetSize(fileHandle, &fileSize64);
    if (fileSize64 < 8 || fileSize64 > 16 * 1024 * 1024) {
        FSFILE_Close(fileHandle);
        return false;
    }
    size_t fileSize = (size_t)fileSize64;

    u8 * buffer = malloc(fileSize);
    if (!buffer) {
        FSFILE_Close(fileHandle);
        return false;
    }

    u32 bytesRead = 0;
    ret = FSFILE_Read(fileHandle, &bytesRead, 0, buffer, (u32)fileSize);
    FSFILE_Close(fileHandle);
    if (R_FAILED(ret) || bytesRead != fileSize) {
        free(buffer);
        return false;
    }

    if (png_sig_cmp(buffer, 0, 8)) {
        free(buffer);
        return false;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) { free(buffer); return false; }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        free(buffer);
        return false;
    }

    pngMemSource memSrc = { buffer, fileSize, 8 };

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        free(buffer);
        return false;
    }

    png_set_read_fn(png_ptr, &memSrc, pngReadFromMem);
    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);

    pngWidth  = png_get_image_width(png_ptr, info_ptr);
    pngHeight = png_get_image_height(png_ptr, info_ptr);
    color_type = png_get_color_type(png_ptr, info_ptr);
    bit_depth  = png_get_bit_depth(png_ptr, info_ptr);

    if (bit_depth == 16)
        png_set_strip_16(png_ptr);
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);

    number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    color_type = png_get_color_type(png_ptr, info_ptr);
    bit_depth  = png_get_bit_depth(png_ptr, info_ptr);

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        free(buffer);
        return false;
    }

    row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * pngHeight);
    for (y = 0; y < pngHeight; y++)
        row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png_ptr, info_ptr));

    png_read_image(png_ptr, row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    png_ptr = NULL;
    info_ptr = NULL;

    free(buffer);
    return true;
}

u8 * process_png_file(void) {
    bytesPerPixel = (color_type == PNG_COLOR_TYPE_RGBA) ? 4 : 3;

    u8 * out = malloc(pngWidth * pngHeight * bytesPerPixel);

    if (out) {
        for (y = 0; y < pngHeight; y++) {
            int moveRight = pngHeight - y;
            png_byte* row = row_pointers[y];
            for (x = 0; x < pngWidth; x++) {
                int offset = (moveRight + x * pngHeight) * bytesPerPixel - bytesPerPixel;
                png_byte* ptr = &(row[x * bytesPerPixel]);
                out[offset]   = ptr[2];
                out[offset+1] = ptr[1];
                out[offset+2] = ptr[0];
                if (bytesPerPixel == 4)
                    out[offset+3] = ptr[3];
            }
        }
    }

    int i;
    for (i = 0; i < pngHeight; i++) free(row_pointers[i]);
    free(row_pointers);
    row_pointers = NULL;

    return out;
}
