#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cartridge.h"
#include "sound.h"

bool regionFreeAvailable = false;
bool regionFreeGamecardIn = false;

smdh_s gamecardSmdh;
menuEntry_s gamecardMenuEntry;

static u64 gamecardTitleId = 0;
static u8 gamecardMediaType = 0;
static FS_CardType gamecardCardType = CARD_CTR;
static u8 gamecardSPIType = 0;
static bool gamecardIsNDS = false;
static char gamecardGameCode[6] = {0};
static char gamecardCardTitle[65] = {0};

static inline Result cartridgeGetNDSMetadata(void)
{
    u8 *hdr = malloc(0x3B4), *ban = malloc(0x23C0);
    if(!hdr || !ban) return -1;
    
    Result ret = FSUSER_GetLegacyRomHeader(MEDIATYPE_GAME_CARD, 0, hdr);
    if(R_FAILED(ret)) { free(hdr); free(ban); return ret; }
    
    memcpy(gamecardGameCode, hdr + 12, 4);
    gamecardGameCode[4] = 0;
    free(hdr);
    
    ret = FSUSER_GetLegacyBannerData(MEDIATYPE_GAME_CARD, 0, ban);
    if(R_FAILED(ret)) { free(ban); return ret; }
    
    typedef struct { 
        u16 v, c; 
        u8 res[28];
        u8 data[512]; 
        u16 pal[16]; 
    } nds_banner_s;
    
    nds_banner_s *b = (nds_banner_s*)ban;
    
	memset(gamecardMenuEntry.iconData, 0, ENTRY_ICONSIZE);

    // make icon 48x48 and smooth
	for(int destX = 0; destX < 48; destX++) {
	    for(int destY = 0; destY < 48; destY++) {
	        float srcXf = (destX * 32.0f) / 48.0f;
	        float srcYf = (destY * 32.0f) / 48.0f;
	        
	        int srcX = (int)srcXf;
	        int srcY = (int)srcYf;
	        
	        float fracX = srcXf - srcX;
	        float fracY = srcYf - srcY;
	        
	        u16 pixels[4] = {0, 0, 0, 0};
	        
	        for(int dy = 0; dy <= 1; dy++) {
	            for(int dx = 0; dx <= 1; dx++) {
	                int x = srcX + dx;
	                int y = srcY + dy;
	                
	                if(x >= 0 && x < 32 && y >= 0 && y < 32) {
	                    u32 so = (((y >> 3) * 4 + (x >> 3)) * 8 + (y & 7)) * 4 + ((x & 7) >> 1);
	                    u16 pi = (b->data[so] >> ((x & 1) * 4)) & 0xF;
	                    if(pi) {
	                        pixels[dy * 2 + dx] = b->pal[pi];
	                    }
	                }
	            }
	        }
	        
	        u16 r[4], g[4], bl[4];
	        for(int i = 0; i < 4; i++) {
	            r[i] = pixels[i] & 0x1F;
	            g[i] = (pixels[i] >> 5) & 0x1F;
	            bl[i] = (pixels[i] >> 10) & 0x1F;
	        }
	        
	        float w00 = (1.0f - fracX) * (1.0f - fracY);
	        float w10 = fracX * (1.0f - fracY);
	        float w01 = (1.0f - fracX) * fracY;
	        float w11 = fracX * fracY;
	        
	        u8 rFinal = (u8)(r[0] * w00 + r[1] * w10 + r[2] * w01 + r[3] * w11);
	        u8 gFinal = (u8)(g[0] * w00 + g[1] * w10 + g[2] * w01 + g[3] * w11);
	        u8 blFinal = (u8)(bl[0] * w00 + bl[1] * w10 + bl[2] * w01 + bl[3] * w11);
	        
	        int destIndex = ((47 - destY) + destX * 48) * 3;
	        
			gamecardMenuEntry.iconData[destIndex + 0] = (blFinal & 0x1F) << 3;
	        u8 greenExtended = (gFinal << 1) | (gFinal >> 4);
			gamecardMenuEntry.iconData[destIndex + 1] = (greenExtended & 0x3F) << 2;
			gamecardMenuEntry.iconData[destIndex + 2] = (rFinal & 0x1F) << 3;
	    }
	}
    
    u16 *englishTitle = (u16*)(ban + 0x340);
    char authorName[ENTRY_AUTHORLENGTH + 1] = {0};
    int titleLen = 0;
    int authorLen = 0;
    int parsingAuthor = 0;
    
    for(int i = 0; i < 128 && englishTitle[i] != 0; i++) {
        if(englishTitle[i] == 0x0A) {
            parsingAuthor = 1;
            continue;
        }
        
        if(englishTitle[i] < 0x80) {
            if(!parsingAuthor) {
                if(titleLen < 63) {
                    gamecardCardTitle[titleLen++] = (char)englishTitle[i];
                }
            } else {
                if(authorLen < ENTRY_AUTHORLENGTH) {
                    authorName[authorLen++] = (char)englishTitle[i];
                }
            }
        }
    }
    
    gamecardCardTitle[titleLen] = 0;
    authorName[authorLen] = 0;
    free(ban);
    
    strncpy(gamecardMenuEntry.name, gamecardCardTitle, ENTRY_NAMELENGTH);
    gamecardMenuEntry.name[ENTRY_NAMELENGTH] = 0;    
    strncpy(gamecardMenuEntry.description, gamecardCardTitle, ENTRY_DESCLENGTH);
    gamecardMenuEntry.description[ENTRY_DESCLENGTH] = 0;
    strncpy(gamecardMenuEntry.author, authorName, ENTRY_AUTHORLENGTH);
    gamecardMenuEntry.author[ENTRY_AUTHORLENGTH] = 0;
    
    return 0;
}

static inline Result cartridgeScanCard(void)
{
	static bool scanning = false;
	if(scanning) return -1;
	scanning = true;

	FS_CardType ct;
	if(R_FAILED(FSUSER_GetCardType(&ct))) { scanning = false; return -1; }

	if(ct == CARD_CTR) {
		u32 cnt = 0;
		if(R_FAILED(AM_GetTitleCount(MEDIATYPE_GAME_CARD, &cnt)) || cnt == 0) { scanning = false; return -1; }
		if(R_FAILED(AM_GetTitleList(NULL, MEDIATYPE_GAME_CARD, 1, &gamecardTitleId))) { scanning = false; return -1; }
		gamecardMediaType = MEDIATYPE_GAME_CARD;
		gamecardCardType = CARD_CTR;
		gamecardIsNDS = false;
		if(R_FAILED(loadGamecardIcon(&gamecardSmdh))) { scanning = false; return -1; }
		extractSmdhData(&gamecardSmdh, gamecardMenuEntry.name, gamecardMenuEntry.description, gamecardMenuEntry.author, gamecardMenuEntry.iconData);
	} else {
		gamecardMediaType = MEDIATYPE_GAME_CARD;
		gamecardCardType = ct;
		gamecardIsNDS = true;
		cartridgeGetNDSMetadata();
	}

	regionFreeGamecardIn = true;
	scanning = false;
	return 0;
}

Result regionFreeInit()
{
	regionFreeAvailable = true;
	regionFreeGamecardIn = false;
	bool cardIn = false;
	if(R_SUCCEEDED(FSUSER_CardSlotIsInserted(&cardIn)) && cardIn)
		cartridgeScanCard();
	return 0;
}

Result regionFreeExit()
{
	return 0;
}

void regionFreeUpdate()
{
	if(!regionFreeAvailable) return;

	bool cardIn = false;
	if(R_FAILED(FSUSER_CardSlotIsInserted(&cardIn))) { regionFreeGamecardIn = false; return; }

	if(! cardIn) {
		if(regionFreeGamecardIn) {
			regionFreeGamecardIn = false;
			gamecardTitleId = 0;
			gamecardIsNDS = false;
			memset(gamecardGameCode, 0, 6);
			memset(gamecardCardTitle, 0, 14);
		}
		return;
	}

	if(! regionFreeGamecardIn && R_SUCCEEDED(cartridgeScanCard()))
		regionFreeGamecardIn = true;
}

Result loadGamecardIcon(smdh_s* out)
{
    if(!out || gamecardIsNDS) return -1;

    Handle fh;
    u32 archivePath[] = {
        gamecardTitleId & 0xFFFFFFFF,
        (gamecardTitleId >> 32) & 0xFFFFFFFF,
        MEDIATYPE_GAME_CARD,  // 2
        0x00000000
    };
    static const u32 filePath[] = {0x00000000, 0x00000000, 0x00000002, 0x6E6F6369, 0x00000000};

    FS_Path fsArchivePath = {PATH_BINARY, 0x10, (u8*)archivePath};
    FS_Path fsFilePath    = {PATH_BINARY, 0x14, (u8*)filePath};

    if(R_FAILED(FSUSER_OpenFileDirectly(&fh, ARCHIVE_SAVEDATA_AND_CONTENT,
                                        fsArchivePath, fsFilePath, FS_OPEN_READ, 0)))
        return -1;

    u32 br;
    Result ret = FSFILE_Read(fh, &br, 0, out, sizeof(smdh_s));
    FSFILE_Close(fh);
    return ret;
}

Result regionFreeRun2(u64 titleid, u8 mediatype)
{
    Result ret = APT_PrepareToDoApplicationJump(0, titleid, mediatype);
    if(R_FAILED(ret)) return ret;

    u8 param[0x300];
    u8 hmac[0x20];
    memset(param, 0, sizeof(param));
    memset(hmac, 0, sizeof(hmac));

    ret = APT_DoApplicationJump(param, sizeof(param), hmac);
    return ret;
}

Result TWLFirmRebootToTitle(void)
{
    if(!gamecardIsNDS || !regionFreeGamecardIn) {
        return -1;
    }
    
    u8 *hdr = malloc(0x3B4);
    if(!hdr) return -1;
    
    Result ret = FSUSER_GetLegacyRomHeader(MEDIATYPE_GAME_CARD, 0, hdr);
    if(R_FAILED(ret)) {
        free(hdr);
        return ret;
    }

    u32 gameCodeValue = *(u32*)(hdr + 0xC);
    free(hdr);
    
    u64 titleid = 0x0004800000000000ULL | (u64)gameCodeValue;
        
    ret = NS_RebootToTitle(MEDIATYPE_GAME_CARD, titleid, 0);

    if(R_SUCCEEDED(ret)) {
        while(1) {
            svcSleepThread(1000000000ULL);
        }
    }
    
    return ret; 
}

menuEntry_s* cartridgeGetMenuEntry(void) 
{ 
	return &gamecardMenuEntry; 
}

u64 cartridgeGetCurrentTitleId(void) { return gamecardTitleId; }
u8 cartridgeGetCurrentMediaType(void) { return gamecardMediaType; }
FS_CardType cartridgeGetCurrentCardType(void) { return gamecardCardType; }
u8 cartridgeGetCurrentSPIType(void) { return gamecardSPIType; }
bool cartridgeIsNDS(void) { return gamecardIsNDS; }
bool cartridgeIsInserted(void) { return regionFreeGamecardIn; }
char* cartridgeGetNDSGameCode(void) { return gamecardGameCode; }
char* cartridgeGetNDSCardTitle(void) { return gamecardCardTitle; }