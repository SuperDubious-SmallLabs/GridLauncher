#include "logText.h"
#include "filesystem.h"
#include <3ds.h>
#include <stdlib.h>
#include <string.h>

void logTextP(char *text, char const * path, bool append) {
    if (!text || !path) return;

    const char * cleanPath = path;
    if (strncmp(path, "sdmc:", 5) == 0) cleanPath = path + 5;

    size_t textLen = strlen(text);
    size_t lineLen = textLen + 1;
    char * line = malloc(lineLen);
    if (!line) return;
    memcpy(line, text, textLen);
    line[textLen] = '\n';

    FS_Path fsPath = fsMakePath(PATH_ASCII, cleanPath);
    Handle fileHandle;
    u64 offset = 0;
    Result ret;

    if (append) {
        ret = FSUSER_OpenFile(&fileHandle, sdmcArchive, fsPath, FS_OPEN_WRITE, 0);
        if (R_FAILED(ret)) {
            ret = FSUSER_CreateFile(sdmcArchive, fsPath, 0, 0);
            if (R_FAILED(ret)) { free(line); return; }
            ret = FSUSER_OpenFile(&fileHandle, sdmcArchive, fsPath, FS_OPEN_WRITE, 0);
            if (R_FAILED(ret)) { free(line); return; }
        }
        FSFILE_GetSize(fileHandle, &offset);
    } else {
        FSUSER_DeleteFile(sdmcArchive, fsPath);
        ret = FSUSER_CreateFile(sdmcArchive, fsPath, 0, lineLen);
        if (R_FAILED(ret)) { free(line); return; }
        ret = FSUSER_OpenFile(&fileHandle, sdmcArchive, fsPath, FS_OPEN_WRITE, 0);
        if (R_FAILED(ret)) { free(line); return; }
    }

    u32 bytesWritten = 0;
    FSFILE_Write(fileHandle, &bytesWritten, offset, line, lineLen, FS_WRITE_FLUSH);
    FSFILE_Close(fileHandle);

    free(line);
}

void logText(char *text) {
    logTextP(text, "/3ds/gridlauncher/log.txt", true);
}
