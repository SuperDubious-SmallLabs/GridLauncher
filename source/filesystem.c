#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "stdlib.h"

#include "installerIcon_bin.h"

#include "filesystem.h"
#include "smdh.h"
#include "utils.h"

#include "addmenuentry.h"
#include "config.h"
#include "logText.h"

FS_Archive sdmcArchive;

void initFilesystem(void)
{
    fsInit();
}

void exitFilesystem(void)
{
    fsExit();
}

Result openSDArchive()
{
    Result ret = FSUSER_OpenArchive(&sdmcArchive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
    return ret;
}

void closeSDArchive()
{
    FSUSER_CloseArchive(sdmcArchive);
    memset(&sdmcArchive, 0, sizeof(FS_Archive));
}

int loadFile(char* path, void* dst, FS_Archive* archive, u64 maxSize)
{
    if(!path || !dst || !archive)return -1;

    u64 size;
    u32 bytesRead;
    Result ret;
    Handle fileHandle;

    ret=FSUSER_OpenFile(&fileHandle, *archive, fsMakePath(PATH_ASCII, path), FS_OPEN_READ, 0);
    if(ret!=0)return ret;

    ret=FSFILE_GetSize(fileHandle, &size);
    if(ret!=0)goto loadFileExit;
    if(size>maxSize){ret=-2; goto loadFileExit;}

    ret=FSFILE_Read(fileHandle, &bytesRead, 0x0, dst, size);
    if(ret!=0)goto loadFileExit;
    if(bytesRead<size){ret=-3; goto loadFileExit;}

    loadFileExit:
    FSFILE_Close(fileHandle);
    return ret;
}

bool fileExists(char* path, FS_Archive* archive)
{
    if(!path || !archive)return false;

    Result ret;
    Handle fileHandle;

    ret=FSUSER_OpenFile(&fileHandle, *archive, fsMakePath(PATH_ASCII, path), FS_OPEN_READ, 0);
    if(ret!=0)return false;

    ret=FSFILE_Close(fileHandle);
    if(ret!=0)return false;

    return true;
}

void addExecutableToMenu(menu_s* m, char* execPath)
{
    if(! m || !execPath)return;
    static menuEntry_s tmpEntry;
    static smdh_s tmpSmdh;
    if(!fileExists(execPath, &sdmcArchive))return;
    int i, l=-1; for(i=0; execPath[i]; i++) if(execPath[i]=='/')l=i;

    initMenuEntry(&tmpEntry, execPath, &execPath[l+1], execPath, "", (u8*)installerIcon_bin);
    Handle fileHandle;
    Result ret = FSUSER_OpenFile(&fileHandle, sdmcArchive, fsMakePath(PATH_ASCII, execPath), FS_OPEN_READ, 0);
    
    if(ret == 0) {
        typedef struct {
            u32 magic;
            u16 headerSize, relocHdrSize;
            u32 formatVer;
            u32 flags;
            u32 codeSegSize, rodataSegSize, dataSegSize, bssSize;
            u32 smdhOffset, smdhSize;
            u32 fsOffset;
        } _3DSX_Header_s;
        
        _3DSX_Header_s header;
        u32 bytesRead = 0;
        
        ret = FSFILE_Read(fileHandle, &bytesRead, 0x0, &header, sizeof(header));
        if(ret == 0 && bytesRead == sizeof(header)) {
            if(header.magic == 0x58534433 && header.smdhOffset > 0 && header.smdhSize >= sizeof(smdh_s)) {
                bytesRead = 0;
                ret = FSFILE_Read(fileHandle, &bytesRead, header.smdhOffset, &tmpSmdh, sizeof(smdh_s));
                if(ret == 0 && bytesRead == sizeof(smdh_s)) {
                    if(tmpSmdh.header.magic == 0x48444D53) {
                        initEmptyMenuEntry(&tmpEntry);
                        initMenuEntry(&tmpEntry, execPath, &execPath[l+1], execPath, "", (u8*)installerIcon_bin);
                        extractSmdhData(&tmpSmdh, tmpEntry.name, tmpEntry.description, tmpEntry.author, tmpEntry.iconData);
                    }
                }
            }
        }
        FSFILE_Close(fileHandle);
    }

    static char xmlPath[128];
    snprintf(xmlPath, 128, "%s", execPath);
    l = strlen(xmlPath);
    if(l >= 4) {
        xmlPath[l-1] = 0;
        xmlPath[l-2] = 'l';
        xmlPath[l-3] = 'm';
        xmlPath[l-4] = 'x';

        if(fileExists(xmlPath, &sdmcArchive)) loadDescriptor(&tmpEntry.descriptor, xmlPath);
    }

    addMenuEntryCopy(m, &tmpEntry);
}

bool checkAddBannerPathToMenuEntry(char *dst, char * path, char *filenamePrefix, bool fullscreen, bool * isFullScreen) {
    static char bannerImagePath[128];
    strcpy(bannerImagePath, "");
    strcat(bannerImagePath, path);
    if (filenamePrefix) {
        strcat(bannerImagePath, "/");
        strcat(bannerImagePath, filenamePrefix);
    }
    if (fullscreen)
        strcat(bannerImagePath, "-banner-fullscreen.png");
    else
        strcat(bannerImagePath, "-banner.png");

    if (fileExists(bannerImagePath, &sdmcArchive)) {
        strncpy(dst, bannerImagePath, ENTRY_PATHLENGTH);
        return true;
    }

    return false;
}

void addBannerPathToMenuEntry(char *dst, char * path, char * filenamePrefix, bool * isFullScreen, bool * hasBanner) {
    if (checkAddBannerPathToMenuEntry(dst, path, filenamePrefix, true, isFullScreen)) {
        *hasBanner = true;
        *isFullScreen = true;
    }
    else if (checkAddBannerPathToMenuEntry(dst, path, filenamePrefix, false, isFullScreen)) {
        *hasBanner = true;
        *isFullScreen = false;
    }
    else {
        *hasBanner = false;
    }
}

void addDirectoryToMenu(menu_s* m, char* path)
{
    if(!m || !path)return;

    static menuEntry_s tmpEntry;
    static smdh_s tmpSmdh;
    static char execPath[128];
    static char iconPath[128];
    static char xmlPath[128];

    int i, l=-1; for(i=0; path[i]; i++) if(path[i]=='/') l=i;

    snprintf(execPath, 128, "%s/boot.3dsx", path);
    if(!fileExists(execPath, &sdmcArchive))
    {
        snprintf(execPath, 128, "%s/%s.3dsx", path, &path[l+1]);
        if(!fileExists(execPath, &sdmcArchive))return;
    }

    bool icon=false;
    snprintf(iconPath, 128, "%s/icon.bin", path);
    if(!icon && !(icon=fileExists(iconPath, &sdmcArchive)))snprintf(iconPath, 128, "%s/icon.smdh", path);
    if(!icon && !(icon=fileExists(iconPath, &sdmcArchive)))snprintf(iconPath, 128, "%s/icon.icn", path);
    if(!icon && !(icon=fileExists(iconPath, &sdmcArchive)))snprintf(iconPath, 128, "%s/%s.smdh", path, &path[l+1]);
    if(!icon && !(icon=fileExists(iconPath, &sdmcArchive)))snprintf(iconPath, 128, "%s/%s.icn", path, &path[l+1]);

    int ret=loadFile(iconPath, &tmpSmdh, &sdmcArchive, sizeof(smdh_s));

    if(!ret)
    {
        initEmptyMenuEntry(&tmpEntry);
        ret=extractSmdhData(&tmpSmdh, tmpEntry.name, tmpEntry.description, tmpEntry.author, tmpEntry.iconData);
        strncpy(tmpEntry.executablePath, execPath, ENTRY_PATHLENGTH);
    }

    if(ret)initMenuEntry(&tmpEntry, execPath, &path[l+1], execPath, "", (u8*)installerIcon_bin);

    snprintf(xmlPath, 128, "%s/descriptor.xml", path);

    if(!fileExists(xmlPath, &sdmcArchive))snprintf(xmlPath, 128, "%s/%s.xml", path, &path[l+1]);
    loadDescriptor(&tmpEntry.descriptor, xmlPath);

    addBannerPathToMenuEntry(tmpEntry.bannerImagePath, path, &path[l+1], &tmpEntry.bannerIsFullScreen, &tmpEntry.hasBanner);

    addMenuEntryCopy(m, &tmpEntry);
}

void scanHomebrewDirectory(menu_s* m, char* path) {
    if(!m) {
        logText("scanHomebrewDirectory: NULL menu pointer");
        return;
    }
    
    if(!path) {
        logText("scanHomebrewDirectory: NULL path pointer");
        return;
    }

    char logBuf[256];
    snprintf(logBuf, sizeof(logBuf), "scanHomebrewDirectory: Scanning path: %s", path);
    logText(logBuf);

    Handle dirHandle = 0;
    FS_Path dirPath = fsMakePath(PATH_ASCII, path);
    
    Result openResult = FSUSER_OpenDirectory(&dirHandle, sdmcArchive, dirPath);
    if(openResult != 0) {
        snprintf(logBuf, sizeof(logBuf), "scanHomebrewDirectory: Failed to open directory, error: 0x%08X", (unsigned int)openResult);
        logText(logBuf);
        return;
    }

    logText("scanHomebrewDirectory: Directory opened successfully");

    // Use heap allocation instead of giant stack array
    #define MAX_ENTRIES 1024
    char (*fullPath)[1024] = malloc(sizeof(char[MAX_ENTRIES][1024]));
    if(!fullPath) {
        logText("scanHomebrewDirectory: Failed to allocate memory for path array");
        FSDIR_Close(dirHandle);
        return;
    }

    u32 entriesRead;
    int totalentries = 0;
    
    logText("scanHomebrewDirectory: Starting directory read loop");
    
    do
    {
        FS_DirectoryEntry entry;
        memset(&entry, 0, sizeof(FS_DirectoryEntry));
        entriesRead = 0;
        
        Result readResult = FSDIR_Read(dirHandle, &entriesRead, 1, &entry);
        if(readResult != 0 && readResult != (Result)0xC8804478) { // 0xC8804478 = end of directory
            snprintf(logBuf, sizeof(logBuf), "scanHomebrewDirectory: Read error: 0x%08X", (unsigned int)readResult);
            logText(logBuf);
            break;
        }
        
        if(entriesRead)
        {
            if(totalentries >= MAX_ENTRIES) {
                logText("scanHomebrewDirectory: Max entries reached, stopping scan");
                break;
            }
            
            strncpy(fullPath[totalentries], path, 1023);
            fullPath[totalentries][1023] = '\0';
            
            int n = strlen(fullPath[totalentries]);
            
            if(n >= 1020) {
                logText("scanHomebrewDirectory: Path too long, skipping entry");
                continue;
            }
            
            unicodeToChar(&fullPath[totalentries][n], entry.name, 1024-n-1);
            fullPath[totalentries][1023] = '\0';
            
            if(entry.attributes & FS_ATTRIBUTE_DIRECTORY) {
                totalentries++;
            } else {
                // Check if it's a .3dsx or .xml file
                n = strlen(fullPath[totalentries]);
                if(n > 5 && strcmp(".3dsx", &fullPath[totalentries][n-5]) == 0) {
                    totalentries++;
                } else if(n > 4 && strcmp(".xml", &fullPath[totalentries][n-4]) == 0) {
                    totalentries++;
                }
            }
        }
    } while(entriesRead);

    snprintf(logBuf, sizeof(logBuf), "scanHomebrewDirectory: Found %d entries", totalentries);
    logText(logBuf);

    FSDIR_Close(dirHandle);
    logText("scanHomebrewDirectory: Directory closed");

    if(totalentries > 0) {
        logText("scanHomebrewDirectory: Calling addMenuEntries");
        bool sortAlpha = getConfigBoolForKey("sortAlpha", false, configTypeMain);
        addMenuEntries(fullPath, totalentries, strlen(path), m, sortAlpha);
        
        logText("scanHomebrewDirectory: Calling updateMenuIconPositions");
        updateMenuIconPositions(m);
        
        snprintf(logBuf, sizeof(logBuf), "scanHomebrewDirectory: Complete, menu now has %d entries", m->numEntries);
        logText(logBuf);
    } else {
        logText("scanHomebrewDirectory: No entries found");
    }

    free(fullPath);
    logText("scanHomebrewDirectory: Freed memory and returning");
}

void addShortcutToMenu(menu_s* m, char* shortcutPath)
{
    if(!m || !shortcutPath)return;

    static shortcut_s tmpShortcut;

    Result ret = createShortcut(&tmpShortcut, shortcutPath);
    if(!ret) {
        int i, l=-1; for(i=0; shortcutPath[i]; i++) if(shortcutPath[i]=='.') l=i;

        char bannerPath[128];
        strcpy(bannerPath, "");
        strncat(bannerPath, &shortcutPath[0], l);
        strcat(bannerPath, "");

        addBannerPathToMenuEntry(tmpShortcut.bannerImagePath, bannerPath, NULL, &tmpShortcut.bannerIsFullScreen, &tmpShortcut.hasBanner);

        createMenuEntryShortcut(m, &tmpShortcut);
    }

    freeShortcut(&tmpShortcut);
}

void createMenuEntryShortcut(menu_s* m, shortcut_s* s)
{
    if(!m || !s)return;

    static menuEntry_s tmpEntry;
    static smdh_s tmpSmdh;

    char* execPath = s->executable;

    if(!fileExists(execPath, &sdmcArchive))return;

    int i, l=-1; for(i=0; execPath[i]; i++) if(execPath[i]=='/') l=i;

    char* iconPath = s->icon;
    int ret = loadFile(iconPath, &tmpSmdh, &sdmcArchive, sizeof(smdh_s));

    if(!ret)
    {
        initEmptyMenuEntry(&tmpEntry);
        ret = extractSmdhData(&tmpSmdh, tmpEntry.name, tmpEntry.description, tmpEntry.author, tmpEntry.iconData);
        strncpy(tmpEntry.executablePath, execPath, ENTRY_PATHLENGTH);
    }

    if(ret) initMenuEntry(&tmpEntry, execPath, &execPath[l+1], execPath, "Unknown publisher", (u8*)installerIcon_bin);

    if(s->name) strncpy(tmpEntry.name, s->name, ENTRY_NAMELENGTH);
    if(s->description) strncpy(tmpEntry.description, s->description, ENTRY_DESCLENGTH);
    if(s->author) strncpy(tmpEntry.author, s->author, ENTRY_AUTHORLENGTH);

    if(s->arg)
    {
        strncpy(tmpEntry.arg, s->arg, ENTRY_ARGLENGTH);
    }

    if(fileExists(s->descriptor, &sdmcArchive)) loadDescriptor(&tmpEntry.descriptor, s->descriptor);

    tmpEntry.isShortcut = true;

    if (s->hasBanner) {
        strcpy(tmpEntry.bannerImagePath, s->bannerImagePath);
        tmpEntry.bannerIsFullScreen = s->bannerIsFullScreen;
    }
    else {
        tmpEntry.bannerImagePath[0] = '\0';
    }

    tmpEntry.hasBanner = s->hasBanner;

    addMenuEntryCopy(m, &tmpEntry);
}

char * currentThemePath() {
    char * currentThemeName = getConfigStringForKey("currentTheme", "Default", configTypeMain);
    if(!currentThemeName) {
        logText("currentThemePath: getConfigStringForKey returned NULL");
        currentThemeName = "Default";
    }
    
    int len = strlen(themesPath) + strlen(currentThemeName) + 2;
    char * path = malloc(len);
    if(!path) {
        logText("currentThemePath: malloc failed");
        path = malloc(strlen(defaultThemePath) + 1);
        if (path) {
            strcpy(path, defaultThemePath);
            return path;
        }
        static char fallback[256];
        strncpy(fallback, defaultThemePath, 255);
        fallback[255] = '\0';
        return fallback;
    }
    
    snprintf(path, len, "%s%s/", themesPath, currentThemeName);
    return path;
}

int compareStrings(const void *stringA, const void *stringB) {
    const char *a = *(const char**)stringA;
    const char *b = *(const char**)stringB;
    return strcmp(a, b);
}

directoryContents * contentsOfDirectoryAtPath(char * path, bool dirsOnly) {
    directoryContents * contents = malloc(sizeof(directoryContents));
    if(!contents) {
        logText("contentsOfDirectoryAtPath: Failed to allocate contents structure");
        return NULL;
    }

    int numPaths = 0;

    Handle dirHandle;
    FS_Path dirPath = fsMakePath(PATH_ASCII, path);
    Result openResult = FSUSER_OpenDirectory(&dirHandle, sdmcArchive, dirPath);
    
    if(openResult != 0) {
        char logBuf[256];
        snprintf(logBuf, sizeof(logBuf), "contentsOfDirectoryAtPath: Failed to open %s, error: 0x%08X", path, (unsigned int)openResult);
        logText(logBuf);
        contents->numPaths = 0;
        return contents;
    }

    u32 entriesRead;
    do
    {
        FS_DirectoryEntry entry;
        memset(&entry, 0, sizeof(FS_DirectoryEntry));
        entriesRead = 0;
        FSDIR_Read(dirHandle, &entriesRead, 1, &entry);
        
        if(entriesRead) {
            if(!dirsOnly || (dirsOnly && (entry.attributes & FS_ATTRIBUTE_DIRECTORY))) {
                char fullPath[1024];
                strncpy(fullPath, path, 1023);
                fullPath[1023] = '\0';
                int n = strlen(path);
                
                if(n < 1020) {
                    unicodeToChar(&fullPath[n], entry.name, 1024-n-1);
                    fullPath[1023] = '\0';

                    strcpy(contents->paths[numPaths], fullPath);
                    numPaths++;
                }
            }
        }
    } while(entriesRead);

    FSDIR_Close(dirHandle);

    contents->numPaths = numPaths;
    return contents;
}