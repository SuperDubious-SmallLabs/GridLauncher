#include <math.h>
#include <stdlib.h>

#include "background.h"
#include "water.h"
#include "gfx.h"
#include "config.h"

#include "stillwater_bin.h"
#include "stillwaterborder_bin.h"

#include "colours.h"

#include "MAGFX.h"
#include "menu.h"

//#include "logo_bin.h"
#if BUBBLES_ENABLED
#include "bubble_bin.h"
#endif

#if FISH_EASTER_EGG
#include "fish1_bin.h"
#include "fish1r_bin.h"
#include "fish2_bin.h"
#include "fish2r_bin.h"
#include "fish3_bin.h"
#include "fish3r_bin.h"
#include "shark_bin.h"
#include "sharkr_bin.h"
#endif

#define BG_WATER_CONTROLPOINTS (100)
#define BG_WATER_NEIGHBORHOODS (3)
#define BG_WATER_DAMPFACTOR (0.7f)
#define BG_WATER_SPRINGFACTOR (0.85f)
#define BG_WATER_WIDTH (500)
#define BG_WATER_OFFSET (-25)

#define waterTopLevel 50
#define waterLevelDiff 6
#define waterLowerLevel waterTopLevel - waterLevelDiff

#define BG_BUBBLE_HEIGHT 32
#define BG_BUBBLE_RISE (waterTopLevel - BG_BUBBLE_HEIGHT)
#define BG_BUBBLE_PHASE (BG_BUBBLE_RISE * 10)

bool hideWaves = false;
bool waterAnimated = true;
bool waterEnabled = true;
//bool keysExciteWater = true;

#if BUBBLES_ENABLED
static bubble_t bubbles[BUBBLE_COUNT];
#endif
static waterEffect_s waterEffect;
static int backgroundCnt;

#if FISH_EASTER_EGG
#define EASTER_WATER_LOWERLEVEL 130
#define FISH_TYPES 4
#define FISH_POOL 8
#define FISH_MAX_ACTIVE 6

typedef struct {
	const u8* left;
	const u8* right;
	int w;
	int h;
} fishSprite_t;

static const fishSprite_t fishSprites[FISH_TYPES] = {
	{ fish1_bin, fish1r_bin, 31, 120 },
	{ fish2_bin, fish2r_bin, 35,  85 },
	{ fish3_bin, fish3r_bin, 48,  95 },
	{ shark_bin, sharkr_bin, 62, 150 },
};

typedef struct {
	bool  active;
	int   type;
	int   dir;
	float y;
	float speed;
	float depthFrac;
	int   fade;
} fish_t;

static fish_t fishes[FISH_POOL];
static int fishSpawnTimer = 0;
bool fishEasterEggActive = false;

static void updateFishes(void);
static void drawFishes(void);
#endif

#if KEYS_EXCITE_WATER || FISH_EASTER_EGG
static float randomFloat(void)
{
	return (float)rand() / (float)RAND_MAX;
}
#endif

void initBackground(void)
{
#if BUBBLES_ENABLED
	int i;
	for(i = 0; i < BUBBLE_COUNT; i++)
	{
		bubbles[i].x = rand() % 400;
		bubbles[i].y = rand() % BG_BUBBLE_PHASE;
		bubbles[i].fade = 0;
	}
#endif

#if FISH_EASTER_EGG
	{
		int k;
		for (k = 0; k < FISH_POOL; k++) fishes[k].active = false;
	}
	fishEasterEggActive = false;
	fishSpawnTimer = 0;
#endif

	initWaterEffect(&waterEffect, BG_WATER_CONTROLPOINTS, BG_WATER_NEIGHBORHOODS, BG_WATER_DAMPFACTOR, BG_WATER_SPRINGFACTOR, BG_WATER_WIDTH, BG_WATER_OFFSET);
	backgroundCnt = 0;
}

#if BUBBLES_ENABLED
void updateBubble(bubble_t* bubble)
{
	bubble->y += 2;

	if(bubble->y >= BG_BUBBLE_PHASE)
	{
		bubble->x = rand() % 400;
		bubble->y = 0;
		bubble->fade = 0;
		return;
	}

	int up = bubble->y;
	int down = BG_BUBBLE_PHASE - bubble->y;
	int f = (up < down) ? up : down;
	f = f * 510 / BG_BUBBLE_PHASE;
	if(f > 255) f = 255;
	bubble->fade = (u8)f;
}

void drawBubbles(void)
{
	int i;
	for(i = 0; i < BUBBLE_COUNT; i++)
	{
		if(bubbles[i].fade == 0) continue;
		gfxDrawSpriteAlphaBlendFade(GFX_TOP, GFX_LEFT, (u8*)bubble_bin, 32, 32,
			bubbles[i].y / 10,
			bubbles[i].x, bubbles[i].fade);
	}
}
#endif

void updateBackground(void)
{
#if FISH_EASTER_EGG
	if ((hidKeysHeld() & KEY_L) && (hidKeysHeld() & KEY_R) && (hidKeysDown() & KEY_SELECT)) {
		fishEasterEggActive = !fishEasterEggActive;
	}
#endif

    if (!waterAnimated) {
        return;
    }

#if BUBBLES_ENABLED
	int i;
	for(i = 0; i < BUBBLE_COUNT; i++)
	{
		updateBubble(&bubbles[i]);
	}
#endif

	exciteWater(&waterEffect, sin(backgroundCnt*0.1f)*2.0f, 0, true);

#if KEYS_EXCITE_WATER
	if ((hidKeysDown() & KEY_UP) || (hidKeysDown() & KEY_DOWN))
	{
		exciteWater(&waterEffect, 0.2f + randomFloat() * 2.0f, rand() % BG_WATER_CONTROLPOINTS, false);
	}
	else if ((hidKeysDown() & KEY_LEFT) || (hidKeysDown() & KEY_RIGHT))
	{
		float v = 3.0f + randomFloat() * 1.0f;
		if (rand() % 2) v = -v;
		int l = rand() % BG_WATER_CONTROLPOINTS;
		int j;
		for (j = 0; j < 5; j++) exciteWater(&waterEffect, v, l - 2 + j, false);
	}
#endif

	updateWaterEffect(&waterEffect);

	backgroundCnt++;

#if FISH_EASTER_EGG
	updateFishes();
#endif
}

int topLevel = waterTopLevel;
int lowerLevel = waterLowerLevel;

int staticWaterX = 0;

u8 tintedWater[70*400*4];
u8 tintedWaterBorder[70*400*4];
bool staticWaterDrawn = false;

#if FISH_EASTER_EGG
static void spawnFish(void)
{
	int idx = -1, activeCount = 0, i;
	for (i = 0; i < FISH_POOL; i++) {
		if (fishes[i].active) activeCount++;
		else if (idx < 0) idx = i;
	}
	if (idx < 0 || activeCount >= FISH_MAX_ACTIVE) return;

	fish_t* f = &fishes[idx];
	const fishSprite_t* s;
	f->active = true;
	f->type = rand() % FISH_TYPES;
	f->dir = (rand() % 2) ? 1 : -1;
	f->speed = 0.6f + randomFloat() * 1.4f;
	f->depthFrac = randomFloat();
	f->fade = 0;
	s = &fishSprites[f->type];
	f->y = (f->dir > 0) ? -(float)s->h : 400.0f;
}

static void updateFishes(void)
{
	int i;

	if (fishEasterEggActive) {
		if (fishSpawnTimer > 0) {
			fishSpawnTimer--;
		}
		else {
			spawnFish();
			fishSpawnTimer = 40 + rand() % 80;
		}
	}

	for (i = 0; i < FISH_POOL; i++) {
		fish_t* f = &fishes[i];
		const fishSprite_t* s;
		if (!f->active) continue;
		s = &fishSprites[f->type];

		f->y += f->dir * f->speed;

		if (fishEasterEggActive) {
			f->fade += 8;
			if (f->fade > 255) f->fade = 255;
		}
		else {
			f->fade -= 8;
			if (f->fade <= 0) { f->active = false; continue; }
		}

		if (f->dir > 0 && f->y > 400.0f) f->active = false;
		else if (f->dir < 0 && f->y < -(float)s->h) f->active = false;
	}
}

static void drawFishes(void)
{
	int i;
	for (i = 0; i < FISH_POOL; i++) {
		fish_t* f = &fishes[i];
		const fishSprite_t* s;
		const u8* data;
		int span, x;
		if (!f->active || f->fade <= 0) continue;
		s = &fishSprites[f->type];
		data = (f->dir > 0) ? s->right : s->left;

		span = topLevel - s->w - 4;
		if (span < 0) span = 0;
		x = 2 + (int)(f->depthFrac * span);

		gfxDrawSpriteAlphaBlendFade(GFX_TOP, GFX_LEFT, (u8*)data, s->w, s->h,
			x, (int)f->y, (u8)f->fade);
	}
}
#endif

void drawBackground()
{
    if (waterEnabled) {
        rgbColour * waterTop = waterTopColour();
        rgbColour * waterBottom = waterBottomColour();

        if (!waterAnimated) {
            if (hideWaves) {
                if (staticWaterX > -70) {
                    staticWaterX -= 2;
                }
            }
            else {
                if (staticWaterX < 0) {
                    staticWaterX += 2;
                }
            }


            if (!staticWaterDrawn) {
                MAGFXImageWithRGBAndAlphaMask(waterBottom->r, waterBottom->g, waterBottom->b, (u8*)stillwater_bin, tintedWater, 70, 400);
                MAGFXImageWithRGBAndAlphaMask(waterTop->r, waterTop->g, waterTop->b, (u8*)stillwaterborder_bin, tintedWaterBorder, 70, 400);
                    staticWaterDrawn = true;
            }


            gfxDrawSpriteAlphaBlendFade(GFX_TOP, GFX_LEFT, tintedWater, 70, 400, staticWaterX, 0, translucencyWater);
            gfxDrawSpriteAlphaBlendFade(GFX_TOP, GFX_LEFT, tintedWaterBorder, 70, 400, staticWaterX, 0, translucencyWater);
            return;
        }

        int targetLower;
        if (hideWaves) {
            targetLower = 0;
        }
#if FISH_EASTER_EGG
        else if (fishEasterEggActive) {
            targetLower = EASTER_WATER_LOWERLEVEL;
        }
#endif
        else {
            targetLower = waterLowerLevel;
        }

        if (lowerLevel < targetLower) {
            topLevel += 1;
            lowerLevel += 1;
        }
        else if (lowerLevel > targetLower) {
            topLevel -= 1;
            lowerLevel -= 1;
        }


        u8 * waterBorderColor = (u8[]){waterTop->r, waterTop->g, waterTop->b};
        u8 * waterColor = (u8[]){waterBottom->r, waterBottom->g, waterBottom->b};

        gfxDrawWave(GFX_TOP, GFX_LEFT, waterBorderColor, waterColor, topLevel, 20, waterLevelDiff, (gfxWaveCallback)&evaluateWater, &waterEffect);
        gfxDrawWave(GFX_TOP, GFX_LEFT, waterColor, waterBorderColor, lowerLevel, 20, 0, (gfxWaveCallback)&evaluateWater, &waterEffect);

#if BUBBLES_ENABLED
        drawBubbles();
#endif

#if FISH_EASTER_EGG
        drawFishes();
#endif
    }
}
