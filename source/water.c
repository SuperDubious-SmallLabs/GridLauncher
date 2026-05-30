#include <string.h>
#include <math.h>
#include <malloc.h>
#include "water.h"

void initWaterEffect(waterEffect_s* we, u16 n, u16 s, float d,  float sf, u16 w, s16 offset)
{
	if(!we)return;

	we->numControlPoints=n;
	we->neighborhoodSize=s;
	we->dampFactor=d;
	we->springFactor=sf;
	we->width=w;
	we->offset=offset;
	we->controlPoints=calloc(n, sizeof(float));
	we->controlPointSpeeds=calloc(n, sizeof(float));
	we->tmpControlPoints=calloc(n, sizeof(float));
}

void killWaterEffect(waterEffect_s* we)
{
	if(!we)return;

	free(we->controlPoints);
	free(we->controlPointSpeeds);
	free(we->tmpControlPoints);
}

float getNeighborAverage(waterEffect_s* we, int k)
{
	if(!we || k<0 || k>=we->numControlPoints)return 0.0f;

	float sum=0.0f;
	float factors=0.0f;

	int i;
	for(i=k-we->neighborhoodSize; i<k+we->neighborhoodSize; i++)
	{
		if(i==k)continue;
		const int d=i-k;
		const float f=fabs(1.0f/d); // TODO : better function (gauss ?)
		float v=0.0f;
		if(i>=0 && i<we->numControlPoints)v=we->controlPoints[i];
		sum+=f*v;
		factors+=f;
	}

	return sum/factors;
}

float evaluateWater(waterEffect_s* we, u16 x)
{
	if(!we || x>=we->width)return 0.0f;

	const float vx=((float)((x-we->offset)*we->numControlPoints))/we->width;
	const int k=(int)vx;
	const float f=vx-(float)k;

	return we->controlPoints[k]*(1.0f-f)+we->controlPoints[k+1]*f;
}

void exciteWater(waterEffect_s* we, float v, u16 k, bool absolute)
{
	if(!we || k>=we->numControlPoints)return;

	if(absolute)
	{
		we->controlPoints[k]=v;
		we->controlPointSpeeds[k]=0.0f;
	}else we->controlPoints[k]+=v;
}

void updateWaterEffect(waterEffect_s* we)
{
	if(!we)return;

	memcpy(we->tmpControlPoints, we->controlPoints, sizeof(float)*we->numControlPoints);

	waterEffect_s tmpwe;
	tmpwe.numControlPoints = we->numControlPoints;
	tmpwe.neighborhoodSize = we->neighborhoodSize;
	tmpwe.controlPoints = we->tmpControlPoints;
	tmpwe.controlPointSpeeds = NULL;
	tmpwe.tmpControlPoints = NULL;

	int k;
	for(k=0; k<we->numControlPoints; k++)
	{
		float rest=getNeighborAverage(&tmpwe, k);
		we->controlPointSpeeds[k]*=we->dampFactor;
		we->controlPointSpeeds[k]+=(rest-we->controlPoints[k])*we->springFactor;
		we->controlPoints[k]+=we->controlPointSpeeds[k];
	}
}
