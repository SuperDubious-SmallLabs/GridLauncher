#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <3ds.h>
#include "menu.h"
#include "smdh.h"

#define REGIONFREE_PATH "regionfree: /"

extern bool regionFreeAvailable;
extern bool regionFreeGamecardIn;
extern smdh_s gamecardSmdh;
extern menuEntry_s gamecardMenuEntry;

Result regionFreeInit();
Result regionFreeExit();
Result regionFreeRun2(u64 titleid, u8 mediatype);
Result loadGamecardIcon(smdh_s* out);
Result TWLFirmRebootToTitle();
void regionFreeUpdate();

u64 cartridgeGetCurrentTitleId(void);
u8 cartridgeGetCurrentMediaType(void);
FS_CardType cartridgeGetCurrentCardType(void);
u8 cartridgeGetCurrentSPIType(void);
bool cartridgeIsNDS(void);
bool cartridgeIsInserted(void);
char* cartridgeGetNDSGameCode(void);
char* cartridgeGetNDSCardTitle(void);
menuEntry_s* cartridgeGetMenuEntry(void);

#endif