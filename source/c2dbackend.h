#pragma once
#include <3ds.h>
#include <citro2d.h>

void c2dBackendInit(void);
void c2dBackendExit(void);
void c2dFrameFlip(void);
void c2dDrawSprite(gfxScreen_t screen, const u8* data, int width, int height,
                   int x, int y, bool hasAlpha, u8 fade);
void c2dDrawSpriteKeyed(gfxScreen_t screen, const void* key, const u8* data,
                        int width, int height, int x, int y,
                        bool hasAlpha, u8 fade);
bool c2dSpriteCached(const void* key, int width, int height, bool hasAlpha);
void c2dFillRect(gfxScreen_t screen, u8 r, u8 g, u8 b, u8 a,
                 int x, int y, int w, int h);
void c2dFillRectGradientH(gfxScreen_t screen,
                          u8 r0, u8 g0, u8 b0, u8 r1, u8 g1, u8 b1, u8 a,
                          int x, int y, int w, int h);
void c2dDrawTriangle(gfxScreen_t screen,
                     float x0, float y0, u32 c0,
                     float x1, float y1, u32 c1,
                     float x2, float y2, u32 c2);
void c2dDrawGlyph(gfxScreen_t screen, const u8* atlas, int atlasStride,
                  int glyphIndex, int uLeft, int vTop, int gW, int gH,
                  int x, int y, u8 r, u8 g, u8 b, u8 a);
void c2dInvalidate(const void* ptr);
