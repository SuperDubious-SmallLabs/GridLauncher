#pragma once
#include <3ds.h>

//#define WATERBORDERCOLOR (u8[]){66, 224, 255}
//#define WATERCOLOR (u8[]){66, 163, 255}

#define BEERBORDERCOLOR (u8[]){240, 240, 240}
#define BEERCOLOR (u8[]){188, 157, 75}

#define BGCOLOR (u8[]){0, 132, 255}

#define BUBBLES_ENABLED 1
#define BUBBLE_COUNT 10

#define KEYS_EXCITE_WATER 1

#define FISH_EASTER_EGG 1

extern bool hideWaves;
extern bool waterAnimated;
extern bool waterEnabled;
extern bool staticWaterDrawn;
extern bool fishEasterEggActive;
//extern bool keysExciteWater;

typedef struct
{
	s32 x, y;
	u8 fade;
}bubble_t;

void initBackground(void);
void updateBackground(void);
void drawBackground();
