#include "c2dbackend.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static C3D_RenderTarget* targetTop = NULL;
static C3D_RenderTarget* targetBottom = NULL;

static bool frameActive = false;
static int  boundScreen = -1;
static C2D_TintMode curTint = C2D_TintMult;
static u32  frameCounter = 0;

#define EVICT_AGE 600
#define CLEAR_COLOR 0x00000000u

static void setTint(C2D_TintMode m)
{
    if (m != curTint) {
        C2D_SetTintMode(m);
        curTint = m;
    }
}

static void ensureFrame(void)
{
    if (!frameActive) {
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(targetTop, CLEAR_COLOR);
        C2D_TargetClear(targetBottom, CLEAR_COLOR);
        frameActive = true;
        boundScreen = -1;
    }
}

static void bindScreen(gfxScreen_t screen)
{
    ensureFrame();
    if ((int)screen != boundScreen) {
        C2D_Flush();
        C3D_RenderTarget* t = (screen == GFX_TOP) ? targetTop : targetBottom;
        C3D_FrameDrawOn(t);
        C2D_SceneSize(240, (screen == GFX_TOP) ? 400 : 320, false);
        boundScreen = (int)screen;
    }
}

static void cacheEvictStale(void);

void c2dFrameFlip(void)
{
    ensureFrame();
    C3D_FrameEnd(0);
    frameActive = false;
    boundScreen = -1;
    frameCounter++;
    cacheEvictStale();
}

typedef struct {
    u64                hkey;
    const void*        srcptr;
    u64                sig;
    bool               used;
    u32                stamp;
    C3D_Tex            tex;
    Tex3DS_SubTexture  subtex;
    C2D_Image          img;
} CacheEntry;

#define CACHE_SIZE 4096
static CacheEntry cache[CACHE_SIZE];
#define HASH_AREA_THRESHOLD 1536

static u64 hashBytes(const void* data, size_t len)
{
    const u8* p = (const u8*)data;
    u64 h = 1469598103934665603ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static u64 quickSig(const void* data, size_t len)
{
    const u8* p = (const u8*)data;
    u64 s = (u64)len * 0x9E3779B97F4A7C15ULL;
    if (len == 0) return s;
    size_t step = (len < 16) ? 1 : (len / 16);
    for (size_t i = 0; i < len; i += step) {
        s ^= (u64)p[i];
        s *= 1099511628211ULL;
    }
    s ^= (u64)p[len - 1];
    s *= 1099511628211ULL;
    return s;
}

static CacheEntry* cacheSlot(u64 hkey)
{
    u32 h = (u32)(hkey ^ (hkey >> 32)) & (CACHE_SIZE - 1);
    for (int i = 0; i < CACHE_SIZE; i++) {
        CacheEntry* e = &cache[(h + i) & (CACHE_SIZE - 1)];
        if (!e->used)
            return e;
        if (e->hkey == hkey)
            return e;
    }
    return NULL;
}

static void cacheEvictStale(void)
{
    for (int i = 0; i < CACHE_SIZE; i++) {
        CacheEntry* e = &cache[i];
        if (e->used && (frameCounter - e->stamp) > EVICT_AGE) {
            C3D_TexDelete(&e->tex);
            memset(e, 0, sizeof(*e));
        }
    }
}

void c2dInvalidate(const void* ptr)
{
    if (!ptr) return;
    for (int i = 0; i < CACHE_SIZE; i++) {
        CacheEntry* e = &cache[i];
        if (e->used && e->srcptr == ptr) {
            C3D_TexDelete(&e->tex);
            memset(e, 0, sizeof(*e));
        }
    }
}

static int clampPot(int v)
{
    int p = 8;
    while (p < v && p < 1024) p <<= 1;
    if (p > 1024) p = 1024;
    return p;
}

static Tex3DS_SubTexture makeSubtex(int w, int h, int tw, int th)
{
    Tex3DS_SubTexture s;
    s.width  = (u16)w;
    s.height = (u16)h;
    s.left   = 0.0f;
    s.top    = 1.0f;
    s.right  = (float)w / (float)tw;
    s.bottom = 1.0f - (float)h / (float)th;
    return s;
}

static inline u32 mortonInterleave(u32 x, u32 y)
{
    u32 i = (x & 7) | ((y & 7) << 8);
    i = (i ^ (i << 2)) & 0x1313;
    i = (i ^ (i << 1)) & 0x1515;
    i = (i | (i >> 7)) & 0x3F;
    return i;
}

static inline u32 tiledOffset(u32 x, u32 y, u32 tw)
{
    return mortonInterleave(x, y) + (x & ~7u) * 8 + (y & ~7u) * tw;
}

static bool buildTexRGBA(C3D_Tex* tex, const u8* src, int w, int h,
                         int srcBpp, int tw, int th)
{
    if (!C3D_TexInit(tex, (u16)tw, (u16)th, GPU_RGBA8))
        return false;
    C3D_TexSetFilter(tex, GPU_LINEAR, GPU_LINEAR);

    u32 size = (u32)tw * (u32)th * 4;
    u8* tmp = (u8*)linearAlloc(size);
    if (!tmp) { C3D_TexDelete(tex); return false; }
    memset(tmp, 0, size);

    for (int v = 0; v < h; v++) {
        for (int u = 0; u < w; u++) {
            const u8* s = &src[((u) + (v) * w) * srcBpp];
            u8 b = s[0], g = s[1], r = s[2];
            u8 a = (srcBpp == 4) ? s[3] : 255;
            u32 off = tiledOffset((u32)u, (u32)v, (u32)tw) * 4;
            tmp[off + 0] = a;
            tmp[off + 1] = b;
            tmp[off + 2] = g;
            tmp[off + 3] = r;
        }
    }

    C3D_TexUpload(tex, tmp);
    linearFree(tmp);
    return true;
}

static bool buildTexA8(C3D_Tex* tex, const u8* atlas, int atlasStride,
                       int uLeft, int vTop, int gW, int gH, int tw, int th)
{
    if (!C3D_TexInit(tex, (u16)tw, (u16)th, GPU_A8))
        return false;
    C3D_TexSetFilter(tex, GPU_LINEAR, GPU_LINEAR);

    u32 size = (u32)tw * (u32)th;
    u8* tmp = (u8*)linearAlloc(size);
    if (!tmp) { C3D_TexDelete(tex); return false; }
    memset(tmp, 0, size);

    for (int v = 0; v < gH; v++) {
        for (int u = 0; u < gW; u++) {
            u8 a = atlas[(uLeft + u) + (vTop + v) * atlasStride];
            u32 off = tiledOffset((u32)u, (u32)v, (u32)tw);
            tmp[off] = a;
        }
    }

    C3D_TexUpload(tex, tmp);
    linearFree(tmp);
    return true;
}

static void drawCachedImage(CacheEntry* e, int x, int y, u8 fade)
{
    setTint(C2D_TintMult);
    if (fade >= 255) {
        C2D_DrawImageAt(e->img, (float)x, (float)y, 0.0f, NULL, 1.0f, 1.0f);
    } else {
        C2D_ImageTint tint;
        C2D_AlphaImageTint(&tint, (float)fade / 255.0f);
        C2D_DrawImageAt(e->img, (float)x, (float)y, 0.0f, &tint, 1.0f, 1.0f);
    }
}

void c2dDrawSprite(gfxScreen_t screen, const u8* data, int width, int height,
                   int x, int y, bool hasAlpha, u8 fade)
{
    if (!data || width <= 0 || height <= 0) return;
    bindScreen(screen);

    int bpp = hasAlpha ? 4 : 3;
    size_t bytes = (size_t)width * (size_t)height * (size_t)bpp;
    u64 dimSalt = ((u64)width << 1) ^ ((u64)height << 20) ^ ((u64)bpp << 40);

    bool keyByPointer = ((long)width * (long)height) > HASH_AREA_THRESHOLD;

    if (keyByPointer) {
        u64 hkey = ((u64)(uintptr_t)data * 1099511628211ULL) ^ dimSalt
                 ^ 0x0002000000000000ULL;
        u64 sig = quickSig(data, bytes);

        CacheEntry* e = cacheSlot(hkey);
        if (!e) return;

        if (e->used && e->sig != sig) {
            C3D_TexDelete(&e->tex);
            e->used = false;
        }
        if (!e->used) {
            int tw = clampPot(width);
            int th = clampPot(height);
            if (!buildTexRGBA(&e->tex, data, width, height, bpp, tw, th))
                return;
            e->subtex = makeSubtex(width, height, tw, th);
            e->img.tex = &e->tex;
            e->img.subtex = &e->subtex;
            e->hkey = hkey;
            e->srcptr = data;
            e->sig = sig;
            e->used = true;
        }
        e->stamp = frameCounter;
        drawCachedImage(e, x, y, fade);
        return;
    }

    u64 hkey = hashBytes(data, bytes) ^ dimSalt;
    CacheEntry* e = cacheSlot(hkey);
    if (!e) return;
    if (!e->used) {
        int tw = clampPot(width);
        int th = clampPot(height);
        if (!buildTexRGBA(&e->tex, data, width, height, bpp, tw, th))
            return;
        e->subtex = makeSubtex(width, height, tw, th);
        e->img.tex = &e->tex;
        e->img.subtex = &e->subtex;
        e->hkey = hkey;
        e->srcptr = NULL;
        e->used = true;
    }
    e->stamp = frameCounter;
    drawCachedImage(e, x, y, fade);
}

static u64 keyedHkey(const void* key, int width, int height, bool hasAlpha)
{
    int bpp = hasAlpha ? 4 : 3;
    u64 dimSalt = ((u64)width << 1) ^ ((u64)height << 20) ^ ((u64)bpp << 40);
    return ((u64)(uintptr_t)key * 1099511628211ULL) ^ dimSalt
         ^ 0x0003000000000000ULL;
}

bool c2dSpriteCached(const void* key, int width, int height, bool hasAlpha)
{
    if (!key) return false;
    u64 hkey = keyedHkey(key, width, height, hasAlpha);
    u32 h = (u32)(hkey ^ (hkey >> 32)) & (CACHE_SIZE - 1);
    for (int i = 0; i < CACHE_SIZE; i++) {
        CacheEntry* e = &cache[(h + i) & (CACHE_SIZE - 1)];
        if (!e->used) return false;
        if (e->hkey == hkey) return true;
    }
    return false;
}

void c2dDrawSpriteKeyed(gfxScreen_t screen, const void* key, const u8* data,
                        int width, int height, int x, int y,
                        bool hasAlpha, u8 fade)
{
    if (!key || width <= 0 || height <= 0) return;
    bindScreen(screen);

    int bpp = hasAlpha ? 4 : 3;
    u64 hkey = keyedHkey(key, width, height, hasAlpha);

    CacheEntry* e = cacheSlot(hkey);
    if (!e) return;
    if (!e->used) {
        if (!data) return;
        int tw = clampPot(width);
        int th = clampPot(height);
        if (!buildTexRGBA(&e->tex, data, width, height, bpp, tw, th))
            return;
        e->subtex = makeSubtex(width, height, tw, th);
        e->img.tex = &e->tex;
        e->img.subtex = &e->subtex;
        e->hkey = hkey;
        e->srcptr = key;
        e->used = true;
    }
    e->stamp = frameCounter;
    drawCachedImage(e, x, y, fade);
}

void c2dFillRect(gfxScreen_t screen, u8 r, u8 g, u8 b, u8 a,
                 int x, int y, int w, int h)
{
    if (a == 0 || w <= 0 || h <= 0) return;
    bindScreen(screen);
    setTint(C2D_TintMult);
    C2D_DrawRectSolid((float)x, (float)y, 0.0f, (float)w, (float)h,
                      C2D_Color32(r, g, b, a));
}

void c2dFillRectGradientH(gfxScreen_t screen,
                          u8 r0, u8 g0, u8 b0, u8 r1, u8 g1, u8 b1, u8 a,
                          int x, int y, int w, int h)
{
    if (a == 0 || w <= 0 || h <= 0) return;
    bindScreen(screen);
    setTint(C2D_TintMult);
    u32 left  = C2D_Color32(r0, g0, b0, a);
    u32 right = C2D_Color32(r1, g1, b1, a);
    /* clr0=top-left, clr1=top-right, clr2=bottom-left, clr3=bottom-right */
    C2D_DrawRectangle((float)x, (float)y, 0.0f, (float)w, (float)h,
                      left, right, left, right);
}

void c2dDrawTriangle(gfxScreen_t screen,
                     float x0, float y0, u32 c0,
                     float x1, float y1, u32 c1,
                     float x2, float y2, u32 c2)
{
    bindScreen(screen);
    setTint(C2D_TintMult);
    C2D_DrawTriangle(x0, y0, c0, x1, y1, c1, x2, y2, c2, 0.0f);
}

void c2dDrawGlyph(gfxScreen_t screen, const u8* atlas, int atlasStride,
                  int glyphIndex, int uLeft, int vTop, int gW, int gH,
                  int x, int y, u8 r, u8 g, u8 b, u8 a)
{
    if (!atlas || gW <= 0 || gH <= 0) return;
    bindScreen(screen);

    u64 hkey = ((u64)(uintptr_t)atlas * 1099511628211ULL)
             ^ ((u64)(u32)glyphIndex << 8) ^ 0x0001000000000000ULL;

    CacheEntry* e = cacheSlot(hkey);
    if (!e) return;
    if (!e->used) {
        int tw = clampPot(gW);
        int th = clampPot(gH);
        if (!buildTexA8(&e->tex, atlas, atlasStride, uLeft, vTop, gW, gH, tw, th))
            return;
        e->subtex = makeSubtex(gW, gH, tw, th);
        e->img.tex = &e->tex;
        e->img.subtex = &e->subtex;
        e->hkey = hkey;
        e->used = true;
    }
    e->stamp = frameCounter;

    setTint(C2D_TintSolid);
    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, C2D_Color32(r, g, b, a), 1.0f);
    C2D_DrawImageAt(e->img, (float)x, (float)y, 0.0f, &tint, 1.0f, 1.0f);
}

void c2dBackendInit(void)
{
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(8192);
    C2D_Prepare();

    targetTop    = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    targetBottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    memset(cache, 0, sizeof(cache));
    frameActive = false;
    boundScreen = -1;
    curTint = C2D_TintMult;
}

void c2dBackendExit(void)
{
    if (frameActive) {
        C3D_FrameEnd(0);
        frameActive = false;
    }
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].used) {
            C3D_TexDelete(&cache[i].tex);
            cache[i].used = false;
        }
    }
    C2D_Fini();
    C3D_Fini();
}
