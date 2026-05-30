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
//#include "bubble_bin.h"

#define BG_WATER_CONTROLPOINTS (100)
#define BG_WATER_NEIGHBORHOODS (3)
#define BG_WATER_DAMPFACTOR (0.7f)
#define BG_WATER_SPRINGFACTOR (0.85f)
#define BG_WATER_WIDTH (500)
#define BG_WATER_OFFSET (-25)

bool hideWaves = false;
bool waterAnimated = true;
bool waterEnabled = true;
//bool keysExciteWater = true;

//static bubble_t bubbles[BUBBLE_COUNT];
static waterEffect_s waterEffect;
static int backgroundCnt;

void initBackground(void)
{
//	int i = 0;
//	for(i = 0;i < BUBBLE_COUNT;i += 1)
//	{
//		bubbles[i].x = rand() % 400;
//		bubbles[i].y = rand() % 240;
//		bubbles[i].fade = 15;
//	}

	initWaterEffect(&waterEffect, BG_WATER_CONTROLPOINTS, BG_WATER_NEIGHBORHOODS, BG_WATER_DAMPFACTOR, BG_WATER_SPRINGFACTOR, BG_WATER_WIDTH, BG_WATER_OFFSET);
	backgroundCnt = 0;
}

//void updateBubble(bubble_t* bubble)
//{
//	// Float up the screen.
//	bubble->y += 2;
//
//	// Check if faded away, then reset if gone.
//	if(bubble->fade < 10)
//	{
//		bubble->x = rand() % 400;
//		bubble->y = rand() % 10;
//		bubble->fade = 15;
//	}
//	// Check if too far up screen and start fizzling away.
//	else if(bubble->y >= 240 && bubble->y % 240 > 100)
//	{
//		bubble->fade -= 10;
//	}
//	// Otherwise make sure the bubble is visible.
//	else if(bubble->fade < 255)
//	{
//		bubble->fade += 10;
//	}
//}

//void drawBubbles(void)
//{
//	int i;
//	//BUBBLES!!
//	for(i = 0;i < BUBBLE_COUNT;i += 1)
//	{
//		// Then draw (no point in separating more because then we go through them all twice).
//		gfxDrawSpriteAlphaBlendFade((bubbles[i].y >= 240) ? (GFX_TOP) : (GFX_BOTTOM), GFX_LEFT, (u8*)bubble_bin, 32, 32,
//			((bubbles[i].y >= 240) ? -64 : 0) + bubbles[i].y % 240,
//			((bubbles[i].y >= 240) ? 0 : -40) + bubbles[i].x, bubbles[i].fade);
//	}
//}
//
//float randomFloat()
//{
//	return (float)rand()/(float)(RAND_MAX);
//}

void updateBackground(void)
{
    if (!waterAnimated) {
        return;
    }

//	int i;
//	for(i = 0;i < BUBBLE_COUNT;i += 1)
//	{
//		// Update first
//		updateBubble(&bubbles[i]);
//	}

	exciteWater(&waterEffect, sin(backgroundCnt*0.1f)*2.0f, 0, true);

	//TODO : improve
//    if (keysExciteWater) {
//        if((hidKeysDown()&KEY_UP) || hidKeysDown()&KEY_DOWN)
//        {
//            exciteWater(&waterEffect, 0.2f+randomFloat()*2.0f, rand()%BG_WATER_CONTROLPOINTS, false);
//        }else if((hidKeysDown()&KEY_LEFT) || hidKeysDown()&KEY_RIGHT)
//        {
//            float v=3.0f+randomFloat()*1.0f;
//            if(rand()%2)v=-v;
//            int l=rand()%BG_WATER_CONTROLPOINTS;
//            int i; for(i=0;i<5;i++)exciteWater(&waterEffect, v, l-2+i, false);
//        }
//
//    }

	updateWaterEffect(&waterEffect);

	backgroundCnt++;
}

#define waterTopLevel 50
#define waterLevelDiff 5
#define waterLowerLevel waterTopLevel - waterLevelDiff

int topLevel = waterTopLevel;
int lowerLevel = waterLowerLevel;

int staticWaterX = 0;

u8 tintedWater[70*400*4];
u8 tintedWaterBorder[70*400*4];
bool staticWaterDrawn = false;

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

        if (hideWaves) {
            if (lowerLevel > 0) {
                topLevel -= 1;
                lowerLevel -= 1;
            }
        }
        else {
            if (lowerLevel < waterLowerLevel) {
                topLevel += 1;
                lowerLevel += 1;
            }
        }


        u8 * waterBorderColor = (u8[]){waterTop->r, waterTop->g, waterTop->b};
        u8 * waterColor = (u8[]){waterBottom->r, waterBottom->g, waterBottom->b};

        gfxDrawWave(GFX_TOP, GFX_LEFT, waterBorderColor, waterColor, topLevel, 20, 5, (gfxWaveCallback)&evaluateWater, &waterEffect);
        gfxDrawWave(GFX_TOP, GFX_LEFT, waterColor, waterBorderColor, lowerLevel, 20, 0, (gfxWaveCallback)&evaluateWater, &waterEffect);
    }
}
