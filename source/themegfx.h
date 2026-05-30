#pragma once
#include <3ds.h>
#include "gfx.h"

extern int themeImageAppBackground;
extern int themeImageAppBackgroundSelected;

extern int themeImageCartBackground;
extern int themeImageCartBackgroundSelected;

extern int themeImageTopWallpaper;
extern int themeImageBottomWallpaper;

extern int themeImageTopLeftButton;
extern int themeImageTopRightButton;
extern int themeImageBottomLeftButton;
extern int themeImageBottomRightButton;

extern int themeImageTopLeftButtonSelected;
extern int themeImageTopRightButtonSelected;
extern int themeImageBottomLeftButtonSelected;
extern int themeImageBottomRightButtonSelected;

extern int themeImageHelpSymbol;
extern int themeImageBackSymbol;
extern int themeImageHomeSymbol;
extern int themeImageSettingsSymbol;
extern int themeImageFoldersSymbol;

extern int themeImageTopWallpaperInfo;

extern int themeImageBottomWallpaperNonGrid;

extern int themeImageSplashTop;
extern int themeImageSplashBottom;

extern int themeImageTopWallpaperReboot;
extern int themeImageBottomWallpaperReboot;

extern int themeImageAppOverlay;
extern int themeImageAppOverlaySelected;

extern int themeImageCartOverlay;
extern int themeImageCartOverlaySelected;





extern bool themeHasProgressWheel;
extern int numProgressWheelImages;

void initThemeImages();
void loadSplashImages();

void drawThemeImage(int imageID, gfxScreen_t screen, int x, int y);
void drawProgressWheelImage(int frame, gfxScreen_t screen, int x, int y);

bool themeImageExists(int imageID);
void freeThemeImages();
