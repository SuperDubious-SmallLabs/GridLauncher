#pragma once
#include <3ds.h>

extern bool touchesAreBlocked;
extern bool touchThreadNeedsRelease;

void startBlockingTouches();
void endBlockingTouches();
void releaseTouchThread();