#pragma once
#include <3ds.h>

extern bool clock24;
extern bool showDate;
extern bool statusbarNeedsUpdate;

extern u32 wifiStatus;
extern u8 batteryLevel;
extern u8 charging;

void drawStatusBar(bool wifiStatus, bool charging, int batteryLevel);
