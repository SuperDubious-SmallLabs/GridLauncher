#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <3ds.h>

#include "gfx.h"
#include "c2dbackend.h"

void gfxDrawSprite(gfxScreen_t screen, gfx3dSide_t side, u8* spriteData,
                   u16 width, u16 height, s16 x, s16 y)
{
    (void)side;
    c2dDrawSprite(screen, spriteData, width, height, x, y, false, 255);
}

void gfxDrawSpriteAlphaBlend(gfxScreen_t screen, gfx3dSide_t side, u8* spriteData,
                             u16 width, u16 height, s16 x, s16 y)
{
    (void)side;
    c2dDrawSprite(screen, spriteData, width, height, x, y, true, 255);
}

void gfxDrawSpriteAlphaBlendFade(gfxScreen_t screen, gfx3dSide_t side, u8* spriteData,
                                 u16 width, u16 height, s16 x, s16 y, u8 fadeValue)
{
    (void)side;
    c2dDrawSprite(screen, spriteData, width, height, x, y, true, fadeValue);
}

void gfxFillRectAlphaBlend(gfxScreen_t screen, gfx3dSide_t side,
                           u8 r, u8 g, u8 b, u8 a, s16 x, s16 y, u16 width, u16 height)
{
    (void)side;
    c2dFillRect(screen, r, g, b, a, x, y, width, height);
}

void gfxFillColor(gfxScreen_t screen, gfx3dSide_t side, u8 rgbColor[3])
{
    (void)side;
    int h = (screen == GFX_TOP) ? 400 : 320;
    c2dFillRect(screen, rgbColor[0], rgbColor[1], rgbColor[2], 255, 0, 0, 240, h);
}

void gfxDrawRectangle(gfxScreen_t screen, gfx3dSide_t side, u8 rgbColor[3],
                      s16 x, s16 y, u16 width, u16 height)
{
    (void)side;
    c2dFillRect(screen, rgbColor[0], rgbColor[1], rgbColor[2], 255, x, y, width, height);
}

static inline u8 lerp8(u8 a, u8 b, float n)
{
    float v = (float)a * (1.0f - n) + (float)b * n;
    if (v < 0.0f) v = 0.0f;
    if (v > 255.0f) v = 255.0f;
    return (u8)v;
}

static inline int waveLevelAt(gfxWaveCallback cb, void* p, int j,
                              int level, int amplitude, int width, int fbWidth)
{
    int wl = level + (int)(cb(p, j) * amplitude);
    if (width) { if (wl < width) wl = width; }
    else       { if (wl < 0)     wl = 0;     }
    if (wl > fbWidth) wl = fbWidth;
    return wl;
}

void gfxDrawWave(gfxScreen_t screen, gfx3dSide_t side,
                 u8 rgbColorStart[3], u8 rgbColorEnd[3],
                 u16 level, u16 amplitude, u16 width, gfxWaveCallback cb, void* p)
{
    (void)side;
    int fbWidth  = 240;
    int fbHeight = (screen == GFX_TOP) ? 400 : 320;

    (void)rgbColorEnd;

    u32 startClr = C2D_Color32(rgbColorStart[0], rgbColorStart[1],
                               rgbColorStart[2], 255);

    float lift = width ? 0.85f : 0.28f;
    u32 crestClr = C2D_Color32(lerp8(rgbColorStart[0], 255, lift),
                               lerp8(rgbColorStart[1], 255, lift),
                               lerp8(rgbColorStart[2], 255, lift), 255);

    const int step = 6;

    int prevJ  = 0;
    int prevWl = waveLevelAt(cb, p, 0, level, amplitude, width, fbWidth);

    int j = step;
    bool done = false;
    while (!done) {
        if (j >= fbHeight - 1) { j = fbHeight - 1; done = true; }

        int wl = waveLevelAt(cb, p, j, level, amplitude, width, fbWidth);

        float y0 = (float)prevJ;
        float y1 = (float)j;

        if (width) {
            float xL0 = (float)(prevWl - width), xR0 = (float)prevWl;
            float xL1 = (float)(wl - width),     xR1 = (float)wl;
            c2dDrawTriangle(screen, xL0, y0, startClr, xR0, y0, crestClr,
                                    xR1, y1, crestClr);
            c2dDrawTriangle(screen, xL0, y0, startClr, xR1, y1, crestClr,
                                    xL1, y1, startClr);
        } else if (prevWl > 0 || wl > 0) {
            c2dDrawTriangle(screen, 0.0f, y0, startClr, (float)prevWl, y0, crestClr,
                                    (float)wl, y1, crestClr);
            c2dDrawTriangle(screen, 0.0f, y0, startClr, (float)wl, y1, crestClr,
                                    0.0f, y1, startClr);
        }

        prevJ  = j;
        prevWl = wl;
        if (!done) {
            j += step;
        }
    }
}

void gfxFlip(void)
{
    c2dFrameFlip();
}
