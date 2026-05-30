#ifndef MATEXT_H
#define MATEXT_H

#include <3ds.h>
#include <stdint.h>

typedef struct {int asciiID, x, y, w, h;} MACharDesc;

typedef struct {
    MACharDesc * charDescs;
    u8 * data;
    int nChars, pngW, pngH, lineHeight;
} MAFont;

typedef struct {int cursorx, cursory;} MACursorPosition;

void MADrawText(gfxScreen_t screen, gfx3dSide_t side, int cursorx, int cursory, char * text, MAFont * font, int red, int green, int blue);

int MADrawTextWrap(gfxScreen_t screen, gfx3dSide_t side, int cursorx, int cursory, char * text, MAFont * font, int red, int green, int blue, int maxWidth, int maxLines);

int MATextWidthInPixels(char * text, MAFont * font);

#endif
