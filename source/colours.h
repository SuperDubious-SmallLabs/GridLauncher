#pragma once
#include <3ds.h>

#include "menu.h"

extern menu_s colourSelectMenu;
extern bool colourSelectMenuNeedsInit;

typedef struct {
    u8 r, g, b;
    u8 defR, defG, defB;
    char key[32];
    char description[32];
} rgbColour;

extern rgbColour * settingsColour;

rgbColour * tintColour();
rgbColour * inactiveColour();
rgbColour * backgroundColour();
rgbColour * waterBottomColour();
rgbColour * waterTopColour();
rgbColour * lightTextColour();
rgbColour * darkTextColour();
rgbColour * titleTextColour();

void saveColour(rgbColour * colour, bool setToDefault);
void drawColourAdjuster();
void handleColourSelectMenuSelection();
void initColourSelectMenu();
void drawTranslucencyAdjust(gfxScreen_t screen);
void drawPanelTranslucencyAdjust(gfxScreen_t screen);
void initColours();
void resetColours(void);
