#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include <sys/stat.h>
#include <malloc.h>

#include "gfx.h"
#include "menu.h"
#include "background.h"
#include "statusbar.h"
#include "filesystem.h"
#include "netloader.h"
#include "cartridge.h"
#include "boot.h"
#include "titles.h"
#include "folders.h"
#include "logodefault_bin.h"
#include "logocompact_bin.h"
#include "logoclassic_bin.h"
#include "MAFontRobotoRegular.h"
#include "alert.h"
#include "logText.h"
#include "colours.h"

//#include "screenshot.h"
#include "config.h"

#include "MAGFX.h"

#include "help.h"
#include "touchblock.h"
#include "folders.h"
#include "themegfx.h"
#include "version.h"
#include "sound.h"

bool sdmcCurrent;
u64 nextSdCheck = 0;

bool die = false;
bool dieImmediately = false;
bool showRebootMenu = false;
bool startRebootProcess = false;

char launcherPath[256] = LAUNCHER_DEFAULT_PATH;

//Handle threadHandle, threadRequest;
//#define STACKSIZE (4 * 1024)

static bool initializeCoreServices(void);
static bool initializeFilesystem(void);
static bool initializeAudio(void);
static void initializeApplicationSubsystems(void);
static void scanInitialHomebrewDirectory(void);
static void handleGamecardStatusChange(void);
static void processMainLoop(void);
//static void cleanupAndExit(menuEntry_s* me);

static enum
{
	HBMENU_DEFAULT,
	HBMENU_TITLESELECT,
	HBMENU_TITLETARGET_ERROR,
	HBMENU_NETLOADER_ACTIVE,
	HBMENU_NETLOADER_UNAVAILABLE_NINJHAX2,
	HBMENU_NETLOADER_ERROR,
} hbmenu_state = HBMENU_DEFAULT;

int debugValues[100];

//void drawDebug()
//{
//	char str[256];
//	sprintf(str, "hello3 %08X %d %d %d %d %d %d %d\n\n%08X %08X %08X %08X\n\n%08X %08X %08X %08X\n\n%08X %08X %08X %08X\n\n", debugValues[50], debugValues[51], debugValues[52], debugValues[53], debugValues[54], debugValues[55], debugValues[56], debugValues[57], debugValues[58], debugValues[59], debugValues[60], debugValues[61], debugValues[62], debugValues[63], debugValues[64], debugValues[65], debugValues[66], debugValues[67], debugValues[68], debugValues[69]);
//
//    rgbColour * dark = darkTextColour();
//
//    MADrawText(GFX_TOP, GFX_LEFT, 48, 100, str, &MAFontRobotoRegular8, dark->r, dark->g, dark->b);
//}

extern void closeReboot() {
    showRebootMenu = false;
}

extern void doReboot() {
    audio_stop();
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
    APT_HardwareResetAsync();
    while (1) svcSleepThread(1000000000ULL);
}

void shutdown3DS()
{
    audio_stop();
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();

    Handle ptmSysmHandle = 0;
    Result result = srvGetServiceHandle(&ptmSysmHandle, "ns:s");
    if (result != 0)
        return;

    u32 *commandBuffer = getThreadCommandBuffer();
    commandBuffer[0] = IPC_MakeHeader(0xE, 1, 0);
    commandBuffer[1] = 0;

    svcSendSyncRequest(ptmSysmHandle);
    svcCloseHandle(ptmSysmHandle);
    while (1) svcSleepThread(1000000000ULL);
}

void launchSVDTFromTitleMenu() {
    menuEntry_s* me = getMenuEntry(&titleMenu, titleMenu.selectedEntry);

    if (me) {
        if (me->title_id) {
            if (me->title_id > 0) {
                createTitleInfoFromTitleID(me->title_id, me->mediatype, &target_title);
                targetProcessId = -2;

//                titleInfo_s* ret = NULL;
//                ret = getTitleWithID(&titleBrowser, me->title_id);
//                targetProcessId = -2;
//                target_title = *ret;

                die = true;
            }
        }
    }
}

void exitServices() {
    if (titlemenuIsUpdating) {
        cancelTitleLoading();
        svcSleepThread(2000000000ULL);
    }

    if (titleThreadNeedsRelease) {
        releaseTitleThread();
    }

    // cleanup whatever we have to cleanup
	audio_stop();
	csndExit();
    ptmuExit();
    acExit();
    romfsExit();
    nsExit();
    amExit();
    freeThemeImages();
    netloader_exit();
    titlesExit();
    hidExit();
    gfxExit();
    closeSDArchive();
    fsExit();
    aptExit();
    srvExit();
}

void launchTitleFromMenu(menu_s* m) {
    menuEntry_s* me = getMenuEntry(m, m->selectedEntry);

    if (me) {
        if (me->title_id && me->title_id > 0) {
            createTitleInfoFromTitleID(me->title_id, me->mediatype, &target_title);
            audio_stop();
            regionFreeRun2(me->title_id, me->mediatype);
        }
        else {
            die = true;
        }
    }
}

void putTitleMenu(char * barTitle) {
    drawGrid(&titleMenu);
    drawBottomStatusBar(barTitle);
}

#include "progresswheel.h"

void handleMenuSelection();

void renderFrame()
{
	// background stuff

    rgbColour * bgc = backgroundColour();

    gfxFillColor(GFX_BOTTOM, GFX_LEFT, (u8[]){bgc->r, bgc->g, bgc->b});
    gfxFillColor(GFX_TOP, GFX_LEFT, (u8[]){bgc->r, bgc->g, bgc->b});

    //Wallpaper
    if (themeImageExists(themeImageTopWallpaperInfo) && ((menuStatus == menuStatusHelp && showingHelpDetails) || menuStatus == menuStatusColourAdjust || menuStatus == menuStatusTranslucencyTop || menuStatus == menuStatusTranslucencyBottom || menuStatus == menuStatusPanelSettingsTop || menuStatus == menuStatusPanelSettingsBottom || hbmenu_state == HBMENU_NETLOADER_ACTIVE || hbmenu_state == HBMENU_NETLOADER_UNAVAILABLE_NINJHAX2 || hbmenu_state == HBMENU_TITLETARGET_ERROR || showRebootMenu || !sdmcCurrent)) {
        drawThemeImage(themeImageTopWallpaperInfo, GFX_TOP, 0, 0);
    }
    else if (themeImageExists(themeImageTopWallpaper)) {
        drawThemeImage(themeImageTopWallpaper, GFX_TOP, 0, 0);
    }

    if (themeImageExists(themeImageBottomWallpaperNonGrid) && ((menuStatus == menuStatusHelp && showingHelpDetails) || menuStatus == menuStatusColourAdjust || menuStatus == menuStatusTranslucencyTop || menuStatus == menuStatusTranslucencyBottom || menuStatus == menuStatusPanelSettingsTop || menuStatus == menuStatusPanelSettingsBottom || hbmenu_state == HBMENU_NETLOADER_ACTIVE || hbmenu_state == HBMENU_NETLOADER_UNAVAILABLE_NINJHAX2 || hbmenu_state == HBMENU_TITLETARGET_ERROR || showRebootMenu || !sdmcCurrent)) {
        drawThemeImage(themeImageBottomWallpaperNonGrid, GFX_BOTTOM, 0, 0);
    }

    else if (themeImageExists(themeImageBottomWallpaper)) {
        drawThemeImage(themeImageBottomWallpaper, GFX_BOTTOM, 0, 0);
    }

//    drawWallpaper();

	// // debug text
	// drawDebug();

//    if (!preloadTitles && titlemenuIsUpdating) {
//        drawDisk("Loading titles");
//    }
//
//    else {


        //menu stuff
        if (showRebootMenu) {
            //about to reboot
            char buttonTitles[3][32];

            bool drawRebootAlert = true;

            if (startRebootProcess) {
                if (themeImageExists(themeImageTopWallpaperReboot) && themeImageExists(themeImageBottomWallpaperReboot)) {
                    drawThemeImage(themeImageTopWallpaperReboot, GFX_TOP, 0, 0);
                    drawThemeImage(themeImageBottomWallpaperReboot, GFX_BOTTOM, 0, 0);
                    drawRebootAlert = false;
                }

                strcpy(buttonTitles[0], "Rebooting...");
            }
            else {
                strcpy(buttonTitles[0], "Reboot");
            }

            strcpy(buttonTitles[1], "Power off");
            strcpy(buttonTitles[2], "Back");

            int alertResult = -1;

            if (drawRebootAlert)
                alertResult = drawAlert("Power options", "Reboot:\nGo back to the system Home Menu\n\nPower off:\nShut down your 3DS", NULL, 3, buttonTitles);

            if (startRebootProcess) {
                doReboot();
            }
            else {
                if (alertResult == 0) {
                    startRebootProcess = true;
                }
                else if (alertResult == 1) {
                    shutdown3DS();
                }
                else if (alertResult == 2 || alertResult == alertButtonKeyB) {
                    closeReboot();
                }
            }

        }else if(!sdmcCurrent)
        {
            //no SD
            drawAlert("No SD detected", "It looks like your 3DS doesn't have an SD inserted into it. Please insert an SD card for optimal homebrew launcher performance!", NULL, 0, NULL);
        }else if(!sdmcCurrent)
        {
            //SD error
            drawAlert("SD Error", "Something unexpected happened when trying to mount your SD card. Try taking it out and putting it back in. If that doesn't work, please try again with another SD card.", NULL, 0, NULL);

        }else if(hbmenu_state == HBMENU_NETLOADER_ACTIVE){
            char bof[256];
            u32 ip = gethostid();
            sprintf(bof,
                "NetLoader Active - waiting for 3dslink connection\n\nIP: %lu.%lu.%lu.%lu, Port: %d\n\nB : Cancel\n",
                ip & 0xFF, (ip>>8)&0xFF, (ip>>16)&0xFF, (ip>>24)&0xFF, NETLOADER_PORT);

            drawAlert("NetLoader", bof, NULL, 0, NULL);
        }else if(hbmenu_state == HBMENU_NETLOADER_UNAVAILABLE_NINJHAX2){
            drawAlert("NetLoader", "The NetLoader is currently unavailable. :( This might be normal and fixable. Try and enable it ?\n\nA : Yes\nB : No\n", NULL, 0, NULL);
        }else if(hbmenu_state == HBMENU_TITLESELECT){

            if (updateGrid(&titleMenu)) {
                launchSVDTFromTitleMenu();
            }
            else {
                putTitleMenu("Select Title");

                if (titlemenuIsUpdating) {
                    drawProgressWheel(GFX_BOTTOM, GFX_LEFT, 0, 0);
                }
            }
        }else if(hbmenu_state == HBMENU_TITLETARGET_ERROR){
            drawAlert("Missing target title", "The application you are trying to run requested a specific target title.\nPlease make sure you have that title !\n\nB : Back\n", NULL, 0, NULL);
        }else if(hbmenu_state == HBMENU_NETLOADER_ERROR){
            netloader_draw_error();
        }else{
            //got SD

            if (menuStatus == menuStatusHomeMenuApps) {
                putTitleMenu("Select Title to Launch");

                if (titlemenuIsUpdating) {
                    drawProgressWheel(GFX_BOTTOM, GFX_LEFT, 0, 0);
                }
            }
            else if (menuStatus == menuStatusFolders) {
                drawGrid(&foldersMenu);
                drawBottomStatusBar("Select folder");
            }
            else if (menuStatus == menuStatusTitleFiltering) {
                putTitleMenu("Tap titles to show or hide them");

                if (titlemenuIsUpdating) {
                    drawProgressWheel(GFX_BOTTOM, GFX_LEFT, 0, 0);
                }
            }
            else if (menuStatus == menuStatusSettings) {
                drawGrid(&settingsMenu);
                drawBottomStatusBar("Settings");
            }
            else if (menuStatus == menuStatusGridSettings) {
                drawGrid(&gridSettingsMenu);
                drawBottomStatusBar("Grid settings");
            }
            else if (menuStatus == menuStatusWaterSettings) {
                drawGrid(&waterMenu);
                drawBottomStatusBar("Water settings");
            }
            else if (menuStatus == menuStatusThemeSettings) {
                drawGrid(&themeSettingsMenu);
                drawBottomStatusBar("Theme settings");
            }
            else if (menuStatus == menuStatusThemeSelect) {
                drawGrid(&themesMenu);
                drawBottomStatusBar("Select theme");
            }
            else if (menuStatus == menuStatusColourSettings) {
                drawGrid(&colourSelectMenu);
                drawBottomStatusBar("Colours");
            }
            else if (menuStatus == menuStatusHelp) {
                drawHelp();
            }
            else if (menuStatus == menuStatusColourAdjust) {
                drawColourAdjuster();
                drawBottomStatusBar("Colour adjustment");
            }
            else if (menuStatus == menuStatusTranslucencyTop) {
                drawTranslucencyAdjust(GFX_TOP);
                drawBottomStatusBar("Top screen translucency");
            }
            else if (menuStatus == menuStatusTranslucencyBottom) {
                drawTranslucencyAdjust(GFX_BOTTOM);
                drawBottomStatusBar("Bottom screen translucency");
            }
            else if (menuStatus == menuStatusPanelSettingsTop) {
                drawPanelTranslucencyAdjust(GFX_TOP);
                drawBottomStatusBar("Top panel settings");
            }
            else if (menuStatus == menuStatusPanelSettingsBottom) {
                drawPanelTranslucencyAdjust(GFX_BOTTOM);
                drawBottomStatusBar("Bottom panel settings");
            }
            else {
                drawMenu(&menu);
            }
        }
//    }



    drawBackground();

    u8 * logoImage = NULL;
    int logoWidth = 0;
    int logoHeight = 0;

    if (logoType == logoTypeDefault) {
        logoImage = (u8*)logodefault_bin;
        logoWidth = 54;
        logoHeight = 161;
    }
    else if (logoType == logoTypeCompact) {
        logoImage = (u8*)logocompact_bin;
        logoWidth = 35;
        logoHeight = 203;
    }
    else if (logoType == logoTypeClassic) {
        logoImage = (u8*)logoclassic_bin;
        logoWidth = 25;
        logoHeight = 223;
    }

    if (logoImage) {
        gfxDrawSpriteAlphaBlend(GFX_TOP, GFX_LEFT, logoImage, logoWidth, logoHeight, 0, 400-logoHeight);
    }

    drawStatusBar(wifiStatus, charging, batteryLevel);
}

// handled in main
// doing it in main is preferred because execution ends in launching another 3dsx
void __appInit()
{
}

// same
void __appExit()
{
}

void showTitleMenu(titleBrowser_s * aTitleBrowser, menu_s * aTitleMenu, int newMenuStatus, bool filter, bool forceHideRegionFree, bool setFilterTicks) {
    if (!titleMenuInitialLoadDone && !titlemenuIsUpdating) {
        updateTitleMenu(&titleBrowser, &titleMenu, "Loading titles", filter, forceHideRegionFree, setFilterTicks);
    }

    setMenuStatus(newMenuStatus);
}

void showSVDTTitleSelect() {
    showTitleMenu(&titleBrowser, &titleMenu, menuStatusTitleBrowser, true, false, false);
    hbmenu_state = HBMENU_TITLESELECT;

    if (animatedGrids) {
        startTransition(transitionDirectionUp, menu.pagePosition, &menu);
    }
}

void showHomeMenuTitleSelect() {
    checkReturnToGrid(&titleMenu);

    showTitleMenu(&titleBrowser, &titleMenu, menuStatusHomeMenuApps, true, false, false);

    if (animatedGrids) {
        startTransition(transitionDirectionUp, menu.pagePosition, &menu);
    }
}

void showFilterTitleSelect() {
    titleMenuInitialLoadDone = false;
    showTitleMenu(&titleBrowser, &titleMenu, menuStatusTitleFiltering, false, true, true);
    if (animatedGrids) {
        startTransition(transitionDirectionDown, settingsMenu.pagePosition, &settingsMenu);
    }
}

void closeTitleBrowser() {
    setMenuStatus(menuStatusIcons);
    checkReturnToGrid(&menu);
    hbmenu_state = HBMENU_DEFAULT;

    if (animatedGrids) {
        startTransition(transitionDirectionDown, titleMenu.pagePosition, &titleMenu);
    }
}

bool gamecardWasIn;
bool gamecardStatusChanged;

void handleMenuSelection() {
//    logText("Handle menu selection");

    menuEntry_s* me = getMenuEntry(&menu, menu.selectedEntry);
//    logText(me->executablePath);

    if(me && !strcmp(me->executablePath, REGIONFREE_PATH) && regionFreeAvailable && !netloader_boot)
    {
        regionFreeUpdate();
        if (regionFreeGamecardIn) {
            if (cartridgeIsNDS()) {
                audio_stop();
                TWLFirmRebootToTitle();
            }

            die = true;
        }
    }
    else
    {
        // if appropriate, look for specified titles in list
        if(me->descriptor.numTargetTitles)
        {
            // first refresh list (for sd/gamecard)
//                        updateTitleBrowser(&titleBrowser);

            // go through target title list in order so that first ones on list have priority
            int i;
            titleInfo_s* ret = NULL;
            for(i=0; i<me->descriptor.numTargetTitles; i++)
            {
                ret = findTitleBrowser(&titleBrowser, me->descriptor.targetTitles[i].mediatype, me->descriptor.targetTitles[i].tid);
                if(ret)break;
            }

            if(ret)
            {
                targetProcessId = -2;
                target_title = *ret;
//                logText("Die 1");
                die = true;
                return;
            }

            // if we get here, we aint found shit
            // if appropriate, let user select target title
            if(me->descriptor.selectTargetProcess) {
                showSVDTTitleSelect();
                //hbmenu_state = HBMENU_TITLESELECT;
            }
            else hbmenu_state = HBMENU_TITLETARGET_ERROR;
        }

        else
        {
            if(me->descriptor.selectTargetProcess) {
                showSVDTTitleSelect();
            }
            else {
//                logText("Die 2");
                die = true;
            }
        }


    }
}

void enterNetloader() {
    if(netloader_activate() == 0) hbmenu_state = HBMENU_NETLOADER_ACTIVE;
    else if(isNinjhax2()) hbmenu_state = HBMENU_NETLOADER_UNAVAILABLE_NINJHAX2;
}

static bool initializeCoreServices(void)
{    
    Result res = srvInit();
    if (R_FAILED(res)) {
        return false;
    }
    
    res = aptInit();
    if (R_FAILED(res)) {
        srvExit();
        return false;
    }
    
    gfxInitDefault();
    
    res = hidInit();
    if (R_FAILED(res)) {
        gfxExit();
        aptExit();
        srvExit();
        return false;
    }
    
    hidScanInput();
    
    u8* framebuf_top = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
    u8* framebuf_bot = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
    
    if (!framebuf_top || !framebuf_bot) {
        hidExit();
        gfxExit();
        aptExit();
        srvExit();
        return false;
    }
    
    memset(framebuf_top, 0, 400 * 240 * 3);
    memset(framebuf_bot, 0, 320 * 240 * 3);
    
    fsInit();
    romfsInit();
    
    ptmuInit();
    acInit();
    amInit();
    nsInit();

    gfxFlip();
    
    return true;
}

static bool initializeFilesystem(void)
{
    Result res = openSDArchive();
    if (R_FAILED(res)) {
        return false;
    }

    FSUSER_CreateDirectory(sdmcArchive, fsMakePath(PATH_ASCII, "/3ds"), 0);
    FSUSER_CreateDirectory(sdmcArchive, fsMakePath(PATH_ASCII, "/3ds/gridlauncher"), 0);
    FSUSER_CreateDirectory(sdmcArchive, fsMakePath(PATH_ASCII, "/3ds/gridlauncher/themes"), 0);
    FSUSER_CreateDirectory(sdmcArchive, fsMakePath(PATH_ASCII, "/3ds/gridlauncher/themes/Default"), 0);
    FSUSER_CreateDirectory(sdmcArchive, fsMakePath(PATH_ASCII, "/3ds/gridlauncher/folders"), 0);

    return true;
}

static bool initializeAudio(void)
{
    
    Result r = csndInit();
    
    if (R_SUCCEEDED(r)) {
        audioActive = true;
    } else {
        audioActive = false;
    }
    
    int bootAttempts = getConfigIntForKey("bootAttempts", 0, configTypeMain);
    
    if (!audioActive && bootAttempts < 5) {
        bootAttempts++;
        setConfigInt("bootAttempts", bootAttempts, configTypeMain);
        saveConfigWithType(configTypeMain);
        
        menuEntry_s* me = malloc(sizeof(menuEntry_s));
        if (!me) {
            return false;
        }
        
        initMenuEntry(me, launcherPath, "launcher", "", "", NULL);
        scanMenuEntry(me);
        exitServices();
        regionFreeExit();
        
        bootApp(me->executablePath, &me->descriptor.executableMetadata, me->arg);
        exit(0);
    }
    
    if (audioActive || bootAttempts >= 5) {
        if (bootAttempts > 0) {
            setConfigInt("bootAttempts", 0, configTypeMain);
            saveConfigWithType(configTypeMain);
        }
    }
    
    return audioActive;
}

static void initializeApplicationSubsystems(void)
{
    
    randomTheme = getConfigBoolForKey("randomTheme", false, configTypeMain);
    
    if (randomTheme) {
        randomiseTheme();
    } else {
        audio_stop();
        loadSplashImages();        
        if (themeImageExists(themeImageSplashTop)) {
            drawThemeImage(themeImageSplashTop, GFX_TOP, 0, 0);
        }
        if (themeImageExists(themeImageSplashBottom)) {
            drawThemeImage(themeImageSplashBottom, GFX_BOTTOM, 0, 0);
        }
        gfxFlip();
        playBootSound();
        initThemeImages();
        initThemeSounds();
        initColours();
    }
    registerAudioAptHook();
    initBackground();
    acInit();
    titlesInit();
    regionFreeInit();
    netloader_init();
    osSetSpeedupEnable(true);
    initMenu(&menu);
    initTitleBrowser(&titleBrowser, NULL);   
}

static void scanInitialHomebrewDirectory(void)
{
    
    bool sdmcCurrent;
    FSUSER_IsSdmcDetected(&sdmcCurrent);
    
    if (sdmcCurrent == 1) {
        scanHomebrewDirectory(&menu, currentFolder());
    } else {
        printf("SD card not detected, skipping scan");
    }
}

static void handleGamecardStatusChange(void)
{
    if (gamecardWasIn != regionFreeGamecardIn) {
        gamecardWasIn = regionFreeGamecardIn;
        
        u64 currentTitleID = 0;
        
        if (regionFreeGamecardIn) {
            int num = 1;
            u64* tmp = (u64*)malloc(sizeof(u64) * num);
            u8 mediatype = 2;
            Result ret = AM_GetTitleList(NULL, mediatype, num, tmp);
            
            if (ret) {
                printf("Error getting gamecard title");
            } else {
                currentTitleID = tmp[0];
            }
            free(tmp);
        }
        
        menuEntry_s *me = getMenuEntry(&menu, 0);
        if (me) {
            me->title_id = currentTitleID;
            me->mediatype = 2;
            me->bannerImagePath[0] = '\0';
            
            if (currentTitleID > 0) {
                addTitleBannerImagePathToMenuEntry(me, currentTitleID);
            }
        }
        
        if (titleMenuInitialLoadDone && titleMenu.numEntries > 0) {
            menuEntry_s *gcme = getMenuEntry(&titleMenu, 0);
            gcme->hidden = !regionFreeGamecardIn;
            gcme->title_id = currentTitleID;
            gcme->mediatype = 2;
            updateMenuIconPositions(&titleMenu);
            gotoFirstIcon(&titleMenu);
            
            gcme->bannerImagePath[0] = '\0';
            if (currentTitleID > 0) {
                addTitleBannerImagePathToMenuEntry(gcme, currentTitleID);
            }
        }
    }
}

static void processMainLoop(void)
{
    menuEntry_s* me = getMenuEntry(&menu, menu.selectedEntry);
    
    if (me) {
        debugValues[50] = me->descriptor.numTargetTitles;
        debugValues[51] = me->descriptor.selectTargetProcess;
        if (me->descriptor.numTargetTitles) {
            debugValues[58] = (me->descriptor.targetTitles[0].tid >> 32) & 0xFFFFFFFF;
            debugValues[59] = me->descriptor.targetTitles[0].tid & 0xFFFFFFFF;
        }
    }
    
    if (hbmenu_state == HBMENU_NETLOADER_ACTIVE) {
        if (hidKeysDown() & KEY_B) {
            netloader_deactivate();
            hbmenu_state = HBMENU_DEFAULT;
        } else {
            int rc = netloader_loop();
            if (rc > 0) {
                netloader_boot = true;
                die = true;
            } else if (rc < 0) {
                hbmenu_state = HBMENU_NETLOADER_ERROR;
            }
        }
    }
    else if (hbmenu_state == HBMENU_NETLOADER_UNAVAILABLE_NINJHAX2) {
        if (hidKeysDown() & KEY_B) {
            hbmenu_state = HBMENU_DEFAULT;
        } else if (hidKeysDown() & KEY_A) {
            if (isNinjhax2()) {
                netloadedPath = launcherPath;
                netloader_boot = true;
                die = true;
            }
        }
    }
    else if (hbmenu_state == HBMENU_TITLETARGET_ERROR) {
        if (hidKeysDown() & KEY_B) {
            hbmenu_state = HBMENU_DEFAULT;
        }
    }
    else if (hbmenu_state == HBMENU_TITLESELECT) {
        if (hidKeysDown() & KEY_A && titleBrowser.selected) {
            launchSVDTFromTitleMenu();
        } else if (hidKeysDown() & KEY_B) {
            closeTitleBrowser();
        }
    }
    else if (hbmenu_state == HBMENU_NETLOADER_ERROR) {
        if (hidKeysDown() & KEY_B) {
            hbmenu_state = HBMENU_DEFAULT;
        }
    }
    else if (!showRebootMenu) {
        if (hidKeysDown() & KEY_START) {
            alertSelectedButton = 0;
            showRebootMenu = true;
        }

        // if (hidKeysDown() & KEY_Y) {
        //     enterNetloader();
        // }
        
        if (menuStatus == menuStatusHomeMenuApps) {
            if (updateGrid(&titleMenu)) {
                launchTitleFromMenu(&titleMenu);
            }
        }
        else if (menuStatus == menuStatusTitleFiltering) {
            if (updateGrid(&titleMenu)) {
                menuEntry_s* me = getMenuEntry(&titleMenu, titleMenu.selectedEntry);
                toggleTitleFilter(me, &titleMenu);
            }
        }
        else if (menuStatus == menuStatusFolders) {
            if (updateGrid(&foldersMenu)) {
                menuEntry_s* me = getMenuEntry(&foldersMenu, foldersMenu.selectedEntry);
                if (me->showTick == NULL) {
                    setFolder(me->name);
                }
            }
        }
        else if (menuStatus == menuStatusSettings) {
            if (updateGrid(&settingsMenu)) {
                handleSettingsMenuSelection(&settingsMenu);
                if (menuStatus == menuStatusSoftwareUpdate) {
                    die = true;
                }
            }
        }
        else if (menuStatus == menuStatusGridSettings) {
            if (updateGrid(&gridSettingsMenu)) {
                handleSettingsMenuSelection(&gridSettingsMenu);
            }
        }
        else if (menuStatus == menuStatusWaterSettings) {
            if (updateGrid(&waterMenu)) {
                handleSettingsMenuSelection(&waterMenu);
            }
        }
        else if (menuStatus == menuStatusThemeSettings) {
            if (updateGrid(&themeSettingsMenu)) {
                handleSettingsMenuSelection(&themeSettingsMenu);
            }
        }
        else if (menuStatus == menuStatusThemeSelect) {
            if (updateGrid(&themesMenu)) {
                if (themesMenu.selectedEntry == 0) {
                    if (!randomTheme) {
                        randomTheme = true;
                        updateMenuTicks(&themesMenu, NULL, true);
                    }
                } else {
                    menuEntry_s* me = getMenuEntry(&themesMenu, themesMenu.selectedEntry);
                    if (me->showTick == NULL) {
                        randomTheme = false;
                        setTheme(me->executablePath);
                        char *currentThemeName = getConfigStringForKey("currentTheme", "Default", configTypeMain);
                        updateMenuTicks(&themesMenu, currentThemeName, true);
                    }
                }
            }
        }
        else if (menuStatus == menuStatusHelp) {
            updateHelp();
        }
        else if (menuStatus == menuStatusColourAdjust || menuStatus == menuStatusPanelSettingsTop || 
                 menuStatus == menuStatusPanelSettingsBottom || menuStatus == menuStatusTranslucencyTop || 
                 menuStatus == menuStatusTranslucencyBottom) {
            handleNonGridToolbarNavigation();
        }
        else if (menuStatus == menuStatusColourSettings) {
            if (updateGrid(&colourSelectMenu)) {
                handleColourSelectMenuSelection();
            }
        }
        else if (updateMenu(&menu)) {
            handleMenuSelection();
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc > 0 && argv[0]) {
        strncpy(launcherPath, argv[0], sizeof(launcherPath) - 1);
        launcherPath[sizeof(launcherPath) - 1] = '\0';
    }

    if (!initializeCoreServices()) {
        svcSleepThread(3000000000ULL);
        return 1;
    }

    if (!initializeFilesystem()) {
        hidExit();
        gfxExit();
        aptExit();
        srvExit();
        return 1;
    }

    srand(svcGetSystemTick());
    
    if (!netloaderShortcut) {
        initializeAudio();
        initializeApplicationSubsystems();
        scanInitialHomebrewDirectory();
    }
    
    u8 sdmcPrevious = 0;
    FSUSER_IsSdmcDetected(&sdmcCurrent);
    sdmcPrevious = sdmcCurrent;
    nextSdCheck = osGetTime() + 250;
    
    gamecardWasIn = !regionFreeGamecardIn;
    
    int frameRate = 60;
    int frameMs = 1000 / frameRate;
    
    if (netloaderShortcut) {
        enterNetloader();
    } else {
        char glInfo[320];
        snprintf(glInfo, sizeof(glInfo), "%s|%s", launcherPath, currentversion);
        logTextP(glInfo, "/3ds/gridlauncher/glinfo.txt", false);
        
        waitForSoundToFinishPlaying(&themeSoundBoot);
        startBGM();

    }
        
    int startMs, endMs, delayMs;
    unsigned long long int delayNs;
    
    while(aptMainLoop()) {
        if (die || dieImmediately) {
            break;
        }

        startMs = osGetTime();
        
        handleGamecardStatusChange();
        
        if (killTitleBrowser) {
            killTitleBrowser = false;
            closeTitleBrowser();
        }
        
        if (menuStatus == menuStatusOpenHomeMenuApps) {
            showHomeMenuTitleSelect();
        } else if (menuStatus == menuStatusOpenTitleFiltering) {
            showFilterTitleSelect();
        }
        
        if (nextSdCheck < osGetTime()) {
            regionFreeUpdate();
            FSUSER_IsSdmcDetected(&sdmcCurrent);
            
            if (sdmcCurrent == 1 && (sdmcPrevious == 0 || sdmcPrevious < 0)) {
                closeSDArchive();
                Result res = openSDArchive();
                if (R_SUCCEEDED(res)) {
                    scanHomebrewDirectory(&menu, currentFolder());
                } else {
                    printf("Failed to reopen SD archive after reinsertion");
                }
            } else if (sdmcCurrent < 1 && sdmcPrevious == 1) {
                clearMenuEntries(&menu);
            }
            
            sdmcPrevious = sdmcCurrent;
            nextSdCheck = osGetTime() + 250;
        }
        
        ACU_GetWifiStatus(&wifiStatus);
        PTMU_GetBatteryLevel(&batteryLevel);
        PTMU_GetBatteryChargeState(&charging);
        hidScanInput();
        
        updateBackground();

        processMainLoop();

        renderFrame();
        gfxFlip();
        
        endMs = osGetTime();
        delayMs = frameMs - (endMs - startMs);
        delayNs = delayMs * 1000000;
        svcSleepThread(delayNs);
    }

    if (!die && !dieImmediately) {
        exitServices();
        return 0;
    }

    if (dieImmediately) {
        exitServices();

        menuEntry_s* me = getMenuEntry(&menu, menu.selectedEntry);
        if (me && me->title_id > 0) {
            return regionFreeRun2(me->title_id, me->mediatype);
        }
    }

    menuEntry_s* me;
    
    if (netloader_boot) {
        me = malloc(sizeof(menuEntry_s));
        initMenuEntry(me, netloadedPath, "netloaded app", "", "", NULL);
    } else if (menuStatus == menuStatusSoftwareUpdate) {
        me = malloc(sizeof(menuEntry_s));
        initMenuEntry(me, "/3ds/gridlauncher/update/mglupdate.3dsx", "updater", "", "", NULL);
    } else {
        me = getMenuEntry(&menu, menu.selectedEntry);
    }
    
    scanMenuEntry(me);
    
    if (touchThreadNeedsRelease) {
        releaseTouchThread();
    }
    
    if (!strcmp(me->executablePath, REGIONFREE_PATH) && regionFreeAvailable && !netloader_boot) {
        regionFreeExit();
        audio_stop();
        Result res = regionFreeRun2(me->title_id, me->mediatype);
        if (R_FAILED(res)) {
            exitServices();
            return res;
        }
        while(1) svcSleepThread(1000000000ULL);
    }

    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
    gfxFlushBuffers();

    int bootResult = bootApp(me->executablePath,
                             &me->descriptor.executableMetadata,
                             me->arg);
    regionFreeExit();
    exitServices();

    return bootResult;
}