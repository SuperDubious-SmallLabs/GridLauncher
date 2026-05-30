#include <string.h>
#include <stdio.h>
#include <3ds.h>
#include "boot.h"
#include "scanner.h"

#define HBLDR_ARGV_BUF_SIZE 0x400

int targetProcessId = 0;
titleInfo_s target_title;

static Handle hbldrHandle = 0;
static bool rosalinaAvailable = false;

static bool initRosalina(void)
{
    Result res = svcConnectToPort(&hbldrHandle, "hb:ldr");
    rosalinaAvailable = R_SUCCEEDED(res);
    return rosalinaAvailable;
}

static Result HBLDR_SetTarget(const char* path)
{
    u32 pathLen = strlen(path) + 1;
    u32* cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = IPC_MakeHeader(2, 0, 2); // 0x20002
    cmdbuf[1] = IPC_Desc_StaticBuffer(pathLen, 0);
    cmdbuf[2] = (u32)path;
    
    Result rc = svcSendSyncRequest(hbldrHandle);
    if (R_SUCCEEDED(rc)) rc = cmdbuf[1];
    return rc;
}

static Result HBLDR_SetArgv(const void* buffer, u32 size)
{
    u32* cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = IPC_MakeHeader(3, 0, 2); // 0x30002
    cmdbuf[1] = IPC_Desc_StaticBuffer(size, 1);
    cmdbuf[2] = (u32)buffer;
    
    Result rc = svcSendSyncRequest(hbldrHandle);
    if (R_SUCCEEDED(rc)) rc = cmdbuf[1];
    return rc;
}

bool isRosalina(void)
{
    if (hbldrHandle == 0)
        initRosalina();
    return rosalinaAvailable;
}

bool isNinjhax2(void)
{
    return !isRosalina();
}

static struct {
    u32 argc;
    char args[HBLDR_ARGV_BUF_SIZE - sizeof(u32)];
} s_argBuf;

int bootApp(char* executablePath, executableMetadata_s* em, char* arg)
{
    if (!executablePath)
        return -1;

    if (hbldrHandle == 0 && !initRosalina())
        return -2;

    const char* path = executablePath;
    if (strncmp(path, "sdmc:/", 6) == 0)
        path += 5;

    memset(&s_argBuf, 0, sizeof(s_argBuf));

    char* dst   = s_argBuf.args;
    size_t space = sizeof(s_argBuf.args);

    char fullPath[ENTRY_PATHLENGTH + 6];
    snprintf(fullPath, sizeof(fullPath), "sdmc:%s", path);

    size_t pathLen = strlen(fullPath) + 1;
    if (pathLen <= space) {
        memcpy(dst, fullPath, pathLen);
        dst   += pathLen;
        space -= pathLen;
        s_argBuf.argc++;
    }

    if (arg && arg[0] != '\0') {
        size_t argLen = strlen(arg) + 1;
        if (argLen <= space) {
            memcpy(dst, arg, argLen);
            s_argBuf.argc++;
        }
    }

    Result res = HBLDR_SetTarget(path);
    if (R_FAILED(res))
        return -3;

    res = HBLDR_SetArgv(&s_argBuf, sizeof(s_argBuf));
    if (R_FAILED(res))
        return -4;

    if (hbldrHandle != 0) {
        svcCloseHandle(hbldrHandle);
        hbldrHandle = 0;
    }

    return 0;
}

void bootExit(void)
{
    if (hbldrHandle != 0)
    {
        svcCloseHandle(hbldrHandle);
        hbldrHandle = 0;
    }
}