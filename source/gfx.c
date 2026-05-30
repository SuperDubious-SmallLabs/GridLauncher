#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <3ds.h>

#include "gfx.h"
//#include "font.h"
//#include "text.h"
//#include "costable.h"

//void gfxDrawText(gfxScreen_t screen, gfx3dSide_t side, font_s* f, char* str, s16 x, s16 y)
//{
//	if(!str)return;
//	if(!f)f=&fontDefault;
//
//	u16 fbWidth, fbHeight;
//	u8* fbAdr=gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);
//
//	drawString(fbAdr, f, str, y, x, fbHeight, fbWidth);
//}

//void gfxDrawTextN(gfxScreen_t screen, gfx3dSide_t side, font_s* f, char* str, u16 length, s16 x, s16 y)
//{
//	if(!str)return;
//	if(!f)f=&fontDefault;
//
//	u16 fbWidth, fbHeight;
//	u8* fbAdr=gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);
//
//	drawStringN(fbAdr, f, str, length, y, x, fbHeight, fbWidth);
//}

void gfxDrawSprite(gfxScreen_t screen, gfx3dSide_t side, u8* spriteData, u16 width, u16 height, s16 x, s16 y)
{
	if(!spriteData)return;

	u16 fbWidth, fbHeight;
	u8* fbAdr=gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);

	if(x+width<0 || x>=fbWidth)return;
	if(y+height<0 || y>=fbHeight)return;

	u16 xOffset=0, yOffset=0;
	u16 widthDrawn=width, heightDrawn=height;

	if(x<0)xOffset=-x;
	if(y<0)yOffset=-y;
	if(x+width>=fbWidth)widthDrawn=fbWidth-x;
	if(y+height>=fbHeight)heightDrawn=fbHeight-y;
	widthDrawn-=xOffset;
	heightDrawn-=yOffset;

	int j;
	for(j=yOffset; j<yOffset+heightDrawn; j++)
	{
		memcpy(&fbAdr[((x+xOffset)+(y+j)*fbWidth)*3], &spriteData[((xOffset)+(j)*width)*3], widthDrawn*3);
	}
}

//void gfxDrawDualSprite(u8* spriteData, u16 width, u16 height, s16 x, s16 y)
//{
//	if(!spriteData)return;
//
//	gfxDrawSprite(GFX_TOP, GFX_LEFT, spriteData, width, height, x-240, y);
//	gfxDrawSprite(GFX_BOTTOM, GFX_LEFT, spriteData, width, height, x, y-40);
//}

//void gfxDrawSpriteAlpha(gfxScreen_t screen, gfx3dSide_t side, u8* spriteData, u16 width, u16 height, s16 x, s16 y)
//{
//	if(!spriteData)return;
//
//	u16 fbWidth, fbHeight;
//	u8* fbAdr=gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);
//
//	if(x+width<0 || x>=fbWidth)return;
//	if(y+height<0 || y>=fbHeight)return;
//
//	u16 xOffset=0, yOffset=0;
//	u16 widthDrawn=width, heightDrawn=height;
//
//	if(x<0)xOffset=-x;
//	if(y<0)yOffset=-y;
//	if(x+width>=fbWidth)widthDrawn=fbWidth-x;
//	if(y+height>=fbHeight)heightDrawn=fbHeight-y;
//	widthDrawn-=xOffset;
//	heightDrawn-=yOffset;
//
//	//TODO : optimize
//	fbAdr+=(y+yOffset)*fbWidth*3;
//	spriteData+=yOffset*width*4;
//	int j, i;
//	for(j=yOffset; j<yOffset+heightDrawn; j++)
//	{
//		u8* fbd=&fbAdr[(x+xOffset)*3];
//		u8* data=&spriteData[(xOffset)*4];
//		for(i=xOffset; i<xOffset+widthDrawn; i++)
//		{
//			if(data[3])
//			{
//				fbd[0]=data[0];
//				fbd[1]=data[1];
//				fbd[2]=data[2];
//			}
//			fbd+=3;
//			data+=4;
//		}
//		fbAdr+=fbWidth*3;
//		spriteData+=width*4;
//	}
//}


void gfxDrawSpriteAlphaBlend(gfxScreen_t screen, gfx3dSide_t side, u8* spriteData, u16 width, u16 height, s16 x, s16 y)
{
	if(!spriteData)return;

	u16 fbWidth, fbHeight;
	u8* fbAdr=gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);

	if(x+width<0 || x>=fbWidth)return;
	if(y+height<0 || y>=fbHeight)return;

	u16 xOffset=0, yOffset=0;
	u16 widthDrawn=width, heightDrawn=height;

	if(x<0)xOffset=-x;
	if(y<0)yOffset=-y;
	if(x+width>=fbWidth)widthDrawn=fbWidth-x;
	if(y+height>=fbHeight)heightDrawn=fbHeight-y;
	widthDrawn-=xOffset;
	heightDrawn-=yOffset;

	fbAdr+=(y+yOffset)*fbWidth*3;
	spriteData+=yOffset*width*4;
	int j, i;
	for(j=yOffset; j<yOffset+heightDrawn; j++)
	{
		u8* fbd=&fbAdr[(x+xOffset)*3];
		u8* data=&spriteData[(xOffset)*4];
		for(i=xOffset; i<xOffset+widthDrawn; i++)
		{
			u8 a = data[3];
			if(a == 255)
			{
				fbd[0] = data[0];
				fbd[1] = data[1];
				fbd[2] = data[2];
			}
			else if(a)
			{
				u8 inv = 255 - a;
				fbd[0] = (data[0] * a + fbd[0] * inv) >> 8;
				fbd[1] = (data[1] * a + fbd[1] * inv) >> 8;
				fbd[2] = (data[2] * a + fbd[2] * inv) >> 8;
			}
			fbd+=3;
			data+=4;
		}
		fbAdr+=fbWidth*3;
		spriteData+=width*4;
	}
}

void gfxDrawSpriteAlphaBlendFade(gfxScreen_t screen, gfx3dSide_t side, u8* spriteData, u16 width, u16 height, s16 x, s16 y, u8 fadeValue)
{
	if(!spriteData)return;

	u16 fbWidth, fbHeight;
	u8* fbAdr=gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);

	if(x+width<0 || x>=fbWidth)return;
	if(y+height<0 || y>=fbHeight)return;

	u16 xOffset=0, yOffset=0;
	u16 widthDrawn=width, heightDrawn=height;

	if(x<0)xOffset=-x;
	if(y<0)yOffset=-y;
	if(x+width>=fbWidth)widthDrawn=fbWidth-x;
	if(y+height>=fbHeight)heightDrawn=fbHeight-y;
	widthDrawn-=xOffset;
	heightDrawn-=yOffset;

	fbAdr+=(y+yOffset)*fbWidth*3;
	spriteData+=yOffset*width*4;
	int j, i;
	for(j=yOffset; j<yOffset+heightDrawn; j++)
	{
		u8* fbd=&fbAdr[(x+xOffset)*3];
		u8* data=&spriteData[(xOffset)*4];
		for(i=xOffset; i<xOffset+widthDrawn; i++)
		{
			if(data[3])
			{
				u8 a = (fadeValue * data[3]) >> 8;
				if(a == 255)
				{
					fbd[0] = data[0];
					fbd[1] = data[1];
					fbd[2] = data[2];
				}
				else
				{
					u8 inv = 255 - a;
					fbd[0] = (data[0] * a + fbd[0] * inv) >> 8;
					fbd[1] = (data[1] * a + fbd[1] * inv) >> 8;
					fbd[2] = (data[2] * a + fbd[2] * inv) >> 8;
				}
			}
			fbd+=3;
			data+=4;
		}
		fbAdr+=fbWidth*3;
		spriteData+=width*4;
	}
}

void gfxFillRectAlphaBlend(gfxScreen_t screen, gfx3dSide_t side, u8 r, u8 g, u8 b, u8 a, s16 x, s16 y, u16 width, u16 height)
{
	if(!a) return;

	u16 fbWidth, fbHeight;
	u8* fbAdr = gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);

	if(x >= (s16)fbWidth || y >= (s16)fbHeight) return;
	if(x + (s16)width <= 0 || y + (s16)height <= 0) return;

	s16 x0 = x, y0 = y;
	s16 x1 = x + (s16)width, y1 = y + (s16)height;
	if(x0 < 0) x0 = 0;
	if(y0 < 0) y0 = 0;
	if(x1 > (s16)fbWidth) x1 = fbWidth;
	if(y1 > (s16)fbHeight) y1 = fbHeight;

	u16 w = x1 - x0;
	u16 h = y1 - y0;

	if(a == 255)
	{
		int j, i;
		for(j = 0; j < h; j++)
		{
			u8* fbd = &fbAdr[(x0 + (y0 + j) * fbWidth) * 3];
			for(i = 0; i < w; i++)
			{
				fbd[0] = b;
				fbd[1] = g;
				fbd[2] = r;
				fbd += 3;
			}
		}
		return;
	}

	u8 inv = 255 - a;
	u32 bA = (u32)b * a;
	u32 gA = (u32)g * a;
	u32 rA = (u32)r * a;

	int j, i;
	for(j = 0; j < h; j++)
	{
		u8* fbd = &fbAdr[(x0 + (y0 + j) * fbWidth) * 3];
		for(i = 0; i < w; i++)
		{
			fbd[0] = (bA + fbd[0] * inv) >> 8;
			fbd[1] = (gA + fbd[1] * inv) >> 8;
			fbd[2] = (rA + fbd[2] * inv) >> 8;
			fbd += 3;
		}
	}
}

void gfxFillColor(gfxScreen_t screen, gfx3dSide_t side, u8 rgbColor[3])
{
	u16 fbWidth, fbHeight;
	u8* fbAdr=gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);

	//TODO : optimize; use GX command ?
	int i;
	for(i=0; i<fbWidth*fbHeight; i++)
	{
		*(fbAdr++)=rgbColor[2];
		*(fbAdr++)=rgbColor[1];
		*(fbAdr++)=rgbColor[0];
	}
}

//void gfxFillColorGradient(gfxScreen_t screen, gfx3dSide_t side, u8 rgbColorStart[3], u8 rgbColorEnd[3])
//{
//	u16 fbWidth, fbHeight;
//	u8* fbAdr=gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);
//	u8 colorLine[fbWidth*3];
//
//	//TODO : optimize; use GX command ?
//	int i;
//	float n;
//	float total = (float)(fbWidth - 1);
//	// make slightly bigger to prevent gradients from blending around.  SHould be removed and have the gradient color be better later.
//	total *= 1.5f;
//	for(i=0; i<fbWidth; i++)
//	{
//		n = (float)i / total;
//		colorLine[i*3+0]=(float)rgbColorStart[2] * (1.0f-n) + (float)rgbColorEnd[2] * n;
//		colorLine[i*3+1]=(float)rgbColorStart[1] * (1.0f-n) + (float)rgbColorEnd[1] * n;
//		colorLine[i*3+2]=(float)rgbColorStart[0] * (1.0f-n) + (float)rgbColorEnd[0] * n;
//	}
//
//	for(i=0; i<fbHeight; i++)
//	{
//		memcpy(fbAdr, colorLine, fbWidth*3);
//		fbAdr+=fbWidth*3;
//	}
//}

void gfxDrawRectangle(gfxScreen_t screen, gfx3dSide_t side, u8 rgbColor[3], s16 x, s16 y, u16 width, u16 height)
{
	u16 fbWidth, fbHeight;
	u8* fbAdr=gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);

	if(x+width<0 || x>=fbWidth)return;
	if(y+height<0 || y>=fbHeight)return;

	if(x<0){width+=x; x=0;}
	if(y<0){height+=y; y=0;}
	if(x+width>=fbWidth)width=fbWidth-x;
	if(y+height>=fbHeight)height=fbHeight-y;

	u8 colorLine[width*3];

	int j;
	for(j=0; j<width; j++)
	{
		colorLine[j*3+0]=rgbColor[2];
		colorLine[j*3+1]=rgbColor[1];
		colorLine[j*3+2]=rgbColor[0];
	}

	fbAdr+=fbWidth*3*y;
	for(j=0; j<height; j++)
	{
		memcpy(&fbAdr[x*3], colorLine, width*3);
		fbAdr+=fbWidth*3;
	}
}

//void gfxFadeScreen(gfxScreen_t screen, gfx3dSide_t side, u32 f)
//{
//	u16 fbWidth, fbHeight;
//	u8* fbAdr=gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);
//
//	int i; for(i=0; i<fbWidth*fbHeight/2; i++)
//	{
//		*fbAdr=(*fbAdr*f)>>8;fbAdr++;
//		*fbAdr=(*fbAdr*f)>>8;fbAdr++;
//		*fbAdr=(*fbAdr*f)>>8;fbAdr++;
//		*fbAdr=(*fbAdr*f)>>8;fbAdr++;
//		*fbAdr=(*fbAdr*f)>>8;fbAdr++;
//		*fbAdr=(*fbAdr*f)>>8;fbAdr++;
//	}
//}

void gfxDrawWave(gfxScreen_t screen, gfx3dSide_t side, u8 rgbColorStart[3], u8 rgbColorEnd[3], u16 level, u16 amplitude, u16 width, gfxWaveCallback cb, void* p)
{
	u16 fbWidth, fbHeight;
	u8* fbAdr=gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);

	static u8 colorLine[2][400*3];
	static u8 cachedStart[2][3];
	static u8 cachedEnd[2][3];
	static u16 cachedFbWidth[2] = {0, 0};
	static int cacheValid[2] = {0, 0};

	if(fbWidth*3 > 400*3) return;

	int slot = (width == 0) ? 1 : 0;
	u8* line = colorLine[slot];

	int rebuild = !cacheValid[slot] || cachedFbWidth[slot] != fbWidth ||
	              cachedStart[slot][0] != rgbColorStart[0] || cachedStart[slot][1] != rgbColorStart[1] || cachedStart[slot][2] != rgbColorStart[2] ||
	              (slot == 1 && (cachedEnd[slot][0] != rgbColorEnd[0] || cachedEnd[slot][1] != rgbColorEnd[1] || cachedEnd[slot][2] != rgbColorEnd[2]));

	int j;

	if(width)
	{
		if(rebuild)
		{
			for(j=0; j<fbWidth; j++)
			{
				line[j*3+0]=rgbColorStart[2];
				line[j*3+1]=rgbColorStart[1];
				line[j*3+2]=rgbColorStart[0];
			}
			cachedStart[slot][0]=rgbColorStart[0]; cachedStart[slot][1]=rgbColorStart[1]; cachedStart[slot][2]=rgbColorStart[2];
			cachedFbWidth[slot]=fbWidth;
			cacheValid[slot]=1;
		}
		for(j=0; j<fbHeight; j++)
		{
		    int wl = (int)level + (int)(cb(p, j) * amplitude);
		    if(wl < (int)width) wl = width;
		    if(wl > (int)fbWidth) wl = fbWidth;
		    u16 waveLevel = (u16)wl;
		    memcpy(&fbAdr[(waveLevel-width)*3], line, width*3);
		    fbAdr+=fbWidth*3;
		}
	}else{
		if(rebuild)
		{
			int i;
			float n;
			float total = (float)(fbWidth - 1);
			// make slightly bigger to prevent gradients from blending around.  SHould be removed and have the gradient color be better later.
			total *= 1.5f;
			for(i=0; i<fbWidth; i++)
			{
				n = (float)i / total;
				line[i*3+0]=(float)rgbColorStart[2] * (1.0f-n) + (float)rgbColorEnd[2] * n;
				line[i*3+1]=(float)rgbColorStart[1] * (1.0f-n) + (float)rgbColorEnd[1] * n;
				line[i*3+2]=(float)rgbColorStart[0] * (1.0f-n) + (float)rgbColorEnd[0] * n;
			}
			cachedStart[slot][0]=rgbColorStart[0]; cachedStart[slot][1]=rgbColorStart[1]; cachedStart[slot][2]=rgbColorStart[2];
			cachedEnd[slot][0]=rgbColorEnd[0]; cachedEnd[slot][1]=rgbColorEnd[1]; cachedEnd[slot][2]=rgbColorEnd[2];
			cachedFbWidth[slot]=fbWidth;
			cacheValid[slot]=1;
		}
		for(j=0; j<fbHeight; j++)
		{
		    int wl = (int)level + (int)(cb(p, j) * amplitude);
		    if(wl < 0) wl = 0;
		    if(wl > (int)fbWidth) wl = fbWidth;
		    u16 waveLevel = (u16)wl;
		    memcpy(fbAdr, line, waveLevel*3);
		    fbAdr+=fbWidth*3;
		}
	}
}

void gfxFlip() {
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
}