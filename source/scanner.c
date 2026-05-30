#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include "scanner.h"
#include "menu.h"
#include "filesystem.h"

#define _3DSX_MAGIC 0x58534433 // '3DSX'

typedef struct
{
	u32 magic;
	u16 headerSize, relocHdrSize;
	u32 formatVer;
	u32 flags;
 
	// Sizes of the code, rodata and data segments +
	// size of the BSS section (uninitialized latter half of the data segment)
	u32 codeSegSize, rodataSegSize, dataSegSize, bssSize;
} _3DSX_Header;

const char* servicesThatMatter[] =
{
	"soc:U",
	"csnd:SND",
	"qtm:s",
	"nfc:u",
	"http:C"
};

void initMetadata(executableMetadata_s* em)
{
	if(!em)return;

	em->scanned = false;

	em->sectionSizes[0] = 0;
	em->sectionSizes[1] = 0;
	em->sectionSizes[2] = 0;

	memset(em->servicesThatMatter, 0x00, sizeof(em->servicesThatMatter));
}

Result scan3dsx(char* path, char** patterns, int num_patterns, u32* sectionSizes, bool* patternsFound)
{
	if(!path)return -1;

	const char* cleanPath = path;
	if(strncmp(path, "sdmc:", 5) == 0) cleanPath = path + 5;

	FS_Path fsPath = fsMakePath(PATH_ASCII, cleanPath);
	Handle fileHandle;
	if(R_FAILED(FSUSER_OpenFile(&fileHandle, sdmcArchive, fsPath, FS_OPEN_READ, 0)))
		return -2;

	Result ret = 0;
	u64 readPos = 0;
	u32 bytesRead = 0;

	_3DSX_Header hdr;
	Result rdRet = FSFILE_Read(fileHandle, &bytesRead, readPos, &hdr, sizeof(_3DSX_Header));
	if(R_FAILED(rdRet) || bytesRead != sizeof(_3DSX_Header))
	{
		ret = -2;
		goto end;
	}
	readPos += sizeof(_3DSX_Header);

	if(hdr.magic != _3DSX_MAGIC)
	{
		ret = -3;
		goto end;
	}

	if(sectionSizes)
	{
		sectionSizes[0] = hdr.codeSegSize;
		sectionSizes[1] = hdr.rodataSegSize;
		sectionSizes[2] = hdr.dataSegSize + hdr.bssSize;
	}

	if(patterns && num_patterns && patternsFound)
	{
		const int buffer_size = 0x1000;
		const int max_pattern_size = 0x10;

		static u8 buffer[0x1000 + 0x10];

		int j;
		for(j=0; j<num_patterns; j++)patternsFound[j] = false;

		readPos += hdr.codeSegSize;

		int elements;
		int total_scanned = 0;
		do
		{
			u32 chunkRead = 0;
			FSFILE_Read(fileHandle, &chunkRead, readPos, &buffer[max_pattern_size], buffer_size);
			elements = (int)chunkRead;
			readPos += chunkRead;

			int i, j;
			int patternsCount[num_patterns];
			for(j=0; j<num_patterns; j++)patternsCount[j] = 0;
			for(i=0; i<elements + max_pattern_size; i++)
			{
				const char v = buffer[i];
				for(j=0; j<num_patterns; j++)
				{
					if(!patternsFound[j])
					{
						if(v == patterns[j][patternsCount[j]])
						{
							patternsCount[j]++;
						}else if(v == patterns[j][0])
						{
							patternsCount[j] = 1;
						}else{
							patternsCount[j] = 0;
						}

						if(patterns[j][patternsCount[j]] == 0x00)
						{
							patternsFound[j] = true;
						}
					}
				}
			}

			memcpy(buffer, &buffer[buffer_size], max_pattern_size);
			total_scanned += elements;
		}while(elements == buffer_size && total_scanned < hdr.rodataSegSize);
	}

	end:
	FSFILE_Close(fileHandle);
	return ret;
}

void scanExecutable(executableMetadata_s* em, char* path)
{
	if(!em || !path || em->scanned)return;

	Result ret = scan3dsx(path, (char**)servicesThatMatter, NUM_SERVICESTHATMATTER, em->sectionSizes, (bool*)em->servicesThatMatter);

	if(!ret)em->scanned = true;
	else em->scanned = false;
}

void scanMenuEntry(menuEntry_s* me)
{
	if(!me)return;

	executableMetadata_s* em = &me->descriptor.executableMetadata;

	static char tmp[0x200];
	snprintf(tmp, 0x200, "sdmc:%s", me->executablePath);

	if(me->descriptor.autodetectServices)
	{
		// if autodetection is enabled (default), we just scan the 3dsx for service names (not ideal but whatchagonnado)
		scanExecutable(em, tmp);
	}else{
		// if it's disabled, then we just populate the metadata structure with section sizes and requested services from descriptor
		int i, j;
		scan3dsx(tmp, NULL, 0, em->sectionSizes, NULL);

		for(i=0; i<me->descriptor.numRequestedServices; i++)
		{
			for(j=0; j<NUM_SERVICESTHATMATTER; j++)
			{
				if(!strcmp(me->descriptor.requestedServices[i].name, servicesThatMatter[j]))
				{
					em->servicesThatMatter[j] = me->descriptor.requestedServices[i].priority;
					break;
				}
			}
		}
		em->scanned = true;
	}
}
