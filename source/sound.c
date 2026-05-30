#include "sound.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "filesystem.h"

themeSound themeSoundBGM = {NULL, 0, 0, false, 0};
themeSound themeSoundMove = {NULL, 0, 0, false, 0};
themeSound themeSoundSelect = {NULL, 0, 0, false, 0};
themeSound themeSoundBack = {NULL, 0, 0, false, 0};
themeSound themeSoundBoot = {NULL, 0, 0, false, 0};

bool audioActive = false;
bool waitForSounds = true;

#include "logText.h"

void audio_load(const char *audio, themeSound * aThemeSound){
	aThemeSound->loaded = false;
	aThemeSound->duration = 0;
	aThemeSound->sndbuffer = NULL;
	aThemeSound->sndsize = 0;

	if (!audioActive) return;

	const char * path = audio;
	if (strncmp(audio, "sdmc:", 5) == 0) path = audio + 5;

	FS_Path fsPath = fsMakePath(PATH_ASCII, path);
	Handle fileHandle;
	Result ret = FSUSER_OpenFile(&fileHandle, sdmcArchive, fsPath, FS_OPEN_READ, 0);
	if (R_FAILED(ret)) return;

	u64 fileSize = 0;
	FSFILE_GetSize(fileHandle, &fileSize);
	if (fileSize <= 0x48) {
		FSFILE_Close(fileHandle);
		return;
	}

	u32 dataSize = (u32)(fileSize - 0x48);
	aThemeSound->sndbuffer = linearAlloc(dataSize);
	if (!aThemeSound->sndbuffer) {
		FSFILE_Close(fileHandle);
		return;
	}

	u32 bytesRead = 0;
	ret = FSFILE_Read(fileHandle, &bytesRead, 0x48, aThemeSound->sndbuffer, dataSize);
	FSFILE_Close(fileHandle);

	if (R_FAILED(ret) || bytesRead != dataSize) {
		linearFree(aThemeSound->sndbuffer);
		aThemeSound->sndbuffer = NULL;
		return;
	}

	aThemeSound->sndsize = dataSize;
	aThemeSound->loaded = true;
	aThemeSound->duration = dataSize / 88244;
}

void audioPlay(themeSound * aThemeSound, bool loop) {
   if (aThemeSound->loaded && audioActive) {
    u32 flags;

    if (loop) {
        flags = SOUND_FORMAT_16BIT | SOUND_REPEAT;
    }
    else {
        flags = SOUND_FORMAT_16BIT;
    }

    csndPlaySound(aThemeSound->channel, flags, 44100, 1, 0, aThemeSound->sndbuffer, aThemeSound->sndbuffer, aThemeSound->sndsize);
   }
}

void audioFree(themeSound * aThemeSound) {
    if (!audioActive || !aThemeSound) {
        return;
    }
    
    if (!aThemeSound->sndbuffer || aThemeSound->sndsize == 0) {
        return;
    }
    
    if ((u32)aThemeSound->sndbuffer < 0x14000000 || 
        (u32)aThemeSound->sndbuffer > 0x1C000000) {
        printf("ERROR: Invalid audio buffer address 0x%08lX", 
               (unsigned long)aThemeSound->sndbuffer);
        return;
    }
    
    memset(aThemeSound->sndbuffer, 0, aThemeSound->sndsize);
    GSPGPU_FlushDataCache(aThemeSound->sndbuffer, aThemeSound->sndsize);
    linearFree(aThemeSound->sndbuffer);
    aThemeSound->sndbuffer = NULL;
    aThemeSound->sndsize = 0;
    aThemeSound->loaded = false;
}

void audio_stop(void){
    if (audioActive) {
        csndExecCmds(true);

        CSND_SetPlayState(0x8, 0);
        CSND_SetPlayState(0x9, 0);
        CSND_SetPlayState(0xA, 0);

        csndExecCmds(true);

        if (themeSoundBGM.loaded) audioFree(&themeSoundBGM);
        if (themeSoundMove.loaded) audioFree(&themeSoundMove);
        if (themeSoundSelect.loaded) audioFree(&themeSoundSelect);
        if (themeSoundBoot.loaded) audioFree(&themeSoundBoot);
        if (themeSoundBack.loaded) audioFree(&themeSoundBack);

        csndExecCmds(true);
    }
}

char* concat(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);
    //in real code you would check for errors in malloc here.
	//perhaps this should be done?
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

void loadThemeSoundOrDefault(char * filename, themeSound * aThemeSound, int channel) {
    char * themePath = currentThemePath();

    if (themePath) {
        char posixPath[256];
        snprintf(posixPath, sizeof(posixPath), "sdmc:%s%s", themePath, filename);
        free(themePath);
        audio_load(posixPath, aThemeSound);
        if (aThemeSound->loaded) {
            aThemeSound->channel = channel;
            return;
        }
    }

    char defaultPath[256];
    snprintf(defaultPath, sizeof(defaultPath), "sdmc:%s%s", defaultThemePath, filename);
    audio_load(defaultPath, aThemeSound);
    if (aThemeSound->loaded) {
        aThemeSound->channel = channel;
    }
}

void initThemeSounds() {
    if (!audioActive) {
        printf("Audio not active, skipping theme sounds");
        return;
    }
//        logTextP("Load BGM", "/bootlog.txt", true);
    loadThemeSoundOrDefault("BGM.bin", &themeSoundBGM, 8);
//        logTextP("Load move sound", "/bootlog.txt", true);
    loadThemeSoundOrDefault("movesound.bin", &themeSoundMove, 9);
//        logTextP("Load select sound", "/bootlog.txt", true);
    loadThemeSoundOrDefault("selectsound.bin", &themeSoundSelect, 10);
//        logTextP("Load back sound", "/bootlog.txt", true);
    loadThemeSoundOrDefault("backsound.bin", &themeSoundBack, 10);
}

void startBGM() {
    if (audioActive) {
//        logTextP("Play music", "/bootlog.txt", true);
        audioPlay(&themeSoundBGM, true);
    }
}

void playBootSound() {
    if (audioActive) {
        loadThemeSoundOrDefault("bootsound.bin", &themeSoundBoot, 10);
        audioPlay(&themeSoundBoot, false);
    }
}

static aptHookCookie s_audioAptHookCookie;

static void audioAptHook(APT_HookType hook, void* param) {
    (void)param;
    if (!audioActive) return;

    switch (hook) {
        case APTHOOK_ONSUSPEND:
        case APTHOOK_ONSLEEP:
            CSND_SetPlayState(0x8, 0);
            CSND_SetPlayState(0x9, 0);
            CSND_SetPlayState(0xA, 0);
            csndExecCmds(true);
            break;
        case APTHOOK_ONRESTORE:
        case APTHOOK_ONWAKEUP:
            if (themeSoundBGM.loaded) audioPlay(&themeSoundBGM, true);
            break;
        default:
            break;
    }
}

void registerAudioAptHook(void) {
    aptHook(&s_audioAptHookCookie, audioAptHook, NULL);
}

void waitForSoundToFinishPlaying(themeSound * aThemeSound) {
    if (waitForSounds && aThemeSound->loaded) {
        u8 playing = 0;
        csndIsPlaying(aThemeSound->channel, &playing);
        while (playing == 1) {
            csndIsPlaying(aThemeSound->channel, &playing);
        }
    }
}
