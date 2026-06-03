#include "MAGFX.h"
#include "string.h"
#include "gfx.h"
#include "MAText.h"
#include "colours.h"
#include "diskalphamask_bin.h"
#include "MAFontRobotoRegular.h"
#include "config.h"

int panelAlphaTop;
int panelAlphaBottom;

int panelRTop;
int panelGTop;
int panelBTop;

int panelRBottom;
int panelGBottom;
int panelBBottom;

bool panelsDrawn = false;

void MAGFXImageWithRGBAndAlphaMask(u8 r, u8 g, u8 b, u8* alphaSourceMask, u8* dest, int width, int height) {
    int alphaSourceByte = 0;
    int destByte = 0;
    int len = width*height;

    while (alphaSourceByte < len) {
        memcpy(&dest[destByte], &b, 1); destByte++;
        memcpy(&dest[destByte], &g, 1); destByte++;
        memcpy(&dest[destByte], &r, 1); destByte++;
        memcpy(&dest[destByte], &alphaSourceMask[alphaSourceByte], 1); destByte++;

        alphaSourceByte ++;
    }
}

void MAGFXApplyAlphaMask(u8* imageSource, u8* alphaSourceMask, u8* dest, int width, int height, bool sourceHasAlpha) {
    int GRBSourceByte = 0;
    int alphaSourceByte = 0;
    int destByte = 0;
    int bytesPerPixel = sourceHasAlpha ? 4 : 3;
    int len = width*height*bytesPerPixel;

    while (GRBSourceByte < len) {
        memcpy(&dest[destByte], &imageSource[GRBSourceByte], 3);
        destByte += 3;
        GRBSourceByte += bytesPerPixel;

        memcpy(&dest[destByte], &alphaSourceMask[alphaSourceByte], 1);
        destByte += 1;
        alphaSourceByte += 1;
    }
}

void MAGFXTranslucentRect(int width, int height, int r, int g, int b, int a, u8* dest) {
    int len = width * height * 4;
    int byte = 0;

    while (byte < len) {
        memcpy(&dest[byte], &b, 1); byte++;
        memcpy(&dest[byte], &g, 1); byte++;
        memcpy(&dest[byte], &r, 1); byte++;
        memcpy(&dest[byte], &a, 1); byte++;
    }
}

int panelEdgeOffset = 20;
int panelLeftOffsetTop = 0;
int panelHeightTop = 360;
int panelHeightBottom = 280;
int panelWidthTop = 182;
int panelWidthBottom = 200;
u8 panelPixel[4]; // 360 * 182 * 4
//u8 panelBottom[203840]; // 280 * 182 * 4

void MAGFXDrawPanel(gfxScreen_t screen, bool forceZeroLeftOffset) {
    if (!panelsDrawn) {
//        panelLeftOffsetTop = getConfigIntForKey("panelLeftOffsetTop", 0, configTypeTheme);

//        int panelColour = 128;
        int panelAlpha = (screen == GFX_TOP) ? panelAlphaTop : panelAlphaBottom;

        int panelR = (screen == GFX_TOP) ? panelRTop : panelRBottom;
        int panelG = (screen == GFX_TOP) ? panelGTop : panelGBottom;
        int panelB = (screen == GFX_TOP) ? panelBTop : panelBBottom;

        panelPixel[0] = panelB;
        panelPixel[1] = panelG;
        panelPixel[2] = panelR;
        panelPixel[3] = panelAlpha;
    }

    if (screen == GFX_TOP && panelAlphaTop == 0) {
        return;
    }

    if (screen == GFX_BOTTOM && panelAlphaBottom == 0) {
        return;
    }

    int drawWidth;
    int drawHeight;
    int leftOffset = 0;

    if (screen == GFX_TOP) {
        drawWidth = panelWidthTop;
        drawHeight = panelHeightTop;

        if (!forceZeroLeftOffset) {
            leftOffset = panelLeftOffsetTop;
        }
    }
    else {
        drawWidth = panelWidthBottom;
        drawHeight = panelHeightBottom;
    }

    drawHeight -= leftOffset;

    u8 panelR = (screen == GFX_TOP) ? panelRTop : panelRBottom;
    u8 panelG = (screen == GFX_TOP) ? panelGTop : panelGBottom;
    u8 panelB = (screen == GFX_TOP) ? panelBTop : panelBBottom;
    u8 panelA = (screen == GFX_TOP) ? panelAlphaTop : panelAlphaBottom;

    gfxFillRectAlphaBlend(screen, GFX_LEFT, panelR, panelG, panelB, panelA,
                          panelEdgeOffset, panelEdgeOffset + leftOffset,
                          drawWidth, drawHeight);
}

u8 disk[64*64*4];
bool diskDrawn = false;

void drawDiskFade(char * text, u8 fade) {
    int rectAlpha = 200 * fade / 255;
    gfxFillRectAlphaBlend(GFX_BOTTOM, GFX_LEFT, 128, 128, 128, rectAlpha, 0, 0, 240, 320);

    int diskX = (240-64)/2;

    if (!diskDrawn) {
        diskDrawn = true;
        rgbColour * tint = tintColour();
        MAGFXImageWithRGBAndAlphaMask(tint->r, tint->g, tint->b, (u8*)diskalphamask_bin, disk, 64, 64);
    }

    gfxDrawSpriteAlphaBlendFade(GFX_BOTTOM, GFX_LEFT, disk, 64, 64, diskX, (320-64)/2, fade);

    if (text && text[0]) {
        int len = MATextWidthInPixels(text, &MAFontRobotoRegular16);
        rgbColour * dark = darkTextColour();
        MASetTextAlpha(fade);
        MADrawText(GFX_BOTTOM, GFX_LEFT, diskX - 32, (320/2)-(len/2), text, &MAFontRobotoRegular16, dark->r, dark->g, dark->b);
        MASetTextAlpha(255);
    }
}

void drawDisk(char * text) {
    drawDiskFade(text, 255);
}


//u8 * MAGFXScaleSprite(u8* spriteData, u8* newSpriteData, u16 width, u16 height, int scaleFactor, bool alpha) {
//    int sourcePos = 0;
//    int destPos = 0;
//    int len = alpha ? 4 : 3;
//    int lineCount = 0;
//    int lineSize = height * len;
//    int newLineSize = lineSize * scaleFactor;
//    int start = 0;
//    int count;
//
//    //While there are still lines left to process
//    while (lineCount < width) {
//        //Repeat scaleFactor times
//        for (count=0; count<scaleFactor; count++) {
//            //Start at the source position in the current line, plus all the previous lines
//            start = sourcePos + (lineSize * lineCount);
//            //Copy one pixel to the end of the new data
//            memcpy(&newSpriteData[destPos], &spriteData[start], len);
//            //Move to the next pixel in the destination (but not the source - the same one might need to be drawn again)
//            destPos += len;
//        }
//
//        //That pixel has been drawn enough times for the line. Move to the next pixel
//        sourcePos += len;
//
//        //If the source position is now off the end of a line...
//        if (sourcePos >= lineSize) {
//            //Move back to the beginning of the line
//            sourcePos = 0;
//            //Move up a line
//            lineCount++;
//
//            //Repeat scaleFactor-1 times
//            for (count=0; count<(scaleFactor-1); count++) {
//                //Copy the whole of the line just completed in the destination to the end of the destination again
//                memcpy(&newSpriteData[destPos], &newSpriteData[destPos-newLineSize], newLineSize);
//                //Move to the next line in the destination
//                destPos += newLineSize;
//            }
//        }
//    }
//
//    return newSpriteData;
//}
