#include "touchblock.h"
#include <malloc.h>

Handle touchBlockThreadHandle, touchBlockThreadRequest;
u32 *touchBlockThreadStack;
#define STACKSIZE (4 * 1024)

bool touchesAreBlocked = false;
bool killThread = false;
bool touchThreadNeedsRelease = false;

void threadTouchBlock(void *arg) {
    while(1) {
        if (touchesAreBlocked) {
            if (!(hidKeysDown()&KEY_TOUCH) && !(hidKeysHeld()&KEY_TOUCH)) {
                touchesAreBlocked = false;
            }
        }
        else {
            svcExitThread();
        }
    }
}

void startBlockingTouches() {
    if (touchThreadNeedsRelease) {
        releaseTouchThread();
    }
    
    touchThreadNeedsRelease = true;
    touchesAreBlocked = true;
    svcCreateEvent(&touchBlockThreadRequest,0);
    touchBlockThreadStack = memalign(32, STACKSIZE);
    svcCreateThread(&touchBlockThreadHandle, threadTouchBlock, 0, &touchBlockThreadStack[STACKSIZE/4], 0x3f, 0);
}

void releaseTouchThread() {
    svcCloseHandle(touchBlockThreadRequest);
    svcCloseHandle(touchBlockThreadHandle);
    free(touchBlockThreadStack);
    touchThreadNeedsRelease = false;
}