#pragma once
#include <3ds.h>
#include "menu.h"
#include "shortcut.h"

#define rootPath          "/3ds/gridlauncher/"
#define themesPath        "/3ds/gridlauncher/themes/"
#define foldersPath       "/3ds/gridlauncher/folders/"
#define defaultThemePath  "/3ds/gridlauncher/themes/Default/"
#define screenshotPath    "/3ds/gridlauncher/screenshots/"
#define configFilePath    "/3ds/gridlauncher/hbl.cfg"
#define ignoredTitlesPath "/3ds/gridlauncher/ignoredtitles.txt"
#define titleBannersPath  "/3ds/gridlauncher/titlebanners"

#define LAUNCHER_DEFAULT_PATH "sdmc:/3ds/gridlauncher/gridlauncher.3dsx"
extern char launcherPath[];

typedef struct {
    int numPaths;
    char paths[1024][1024];
} directoryContents;

extern FS_Archive sdmcArchive;

//system stuff
void initFilesystem(void);
void exitFilesystem(void);

Result openSDArchive(void);
void closeSDArchive();

//general fs stuff
int loadFile(char* path, void* dst, FS_Archive* archive, u64 maxSize);
bool fileExists(char* path, FS_Archive* archive);

//menu fs stuff
void addDirectoryToMenu(menu_s* m, char* path);
void addExecutableToMenu(menu_s* m, char* execPath);
void addShortcutToMenu(menu_s* m, char* shortcutPath);
void scanHomebrewDirectory(menu_s* m, char* path);
directoryContents * contentsOfDirectoryAtPath(char * path, bool dirsOnly);
void addBannerPathToMenuEntry(char *dst, char * path, char * filenamePrefix, bool * isFullScreen, bool * hasBanner);

char * currentThemePath();

void createMenuEntryShortcut(menu_s* m, shortcut_s* s);
