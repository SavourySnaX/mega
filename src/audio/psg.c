/*
 *  psg.c

Copyright (c) 2011 Lee Hammerton

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "psg.h"

U8 PSG_AddressLatch=0;

U16 PSG_NoiseShiftRegister=0x8000;
U16 PSG_ToneNoise[4]={0,0,0,0};
U16	PSG_ToneCounter[4]={0,0,0,0};
U16 PSG_ToneOut[4]={0,0,0,0};
U16 PSG_NoiseCounter=0;
U8	PSG_Vol[4]={0xF,0xF,0xF,0xF};

U16 PSG_MasterClock=0;

void PSG_UpdateTones()
{
	int a;
	for (a=0;a<3;a++)
	{
		if (PSG_ToneCounter[a]==0)	/* special case, always set high */
		{
			PSG_ToneOut[a]=1;
			PSG_ToneCounter[a]=PSG_ToneNoise[a]&0x3FF;
		}
		else
		{
			PSG_ToneCounter[a]--;
			if (PSG_ToneCounter[a]==0)
			{
				PSG_ToneOut[a]^=1;
				PSG_ToneCounter[a]=PSG_ToneNoise[a]&0x3FF;
			}
		}
	}
}

void PSG_UpdateNoiseShiftRegister()
{
	U16 newBit=0;
	PSG_ToneOut[3]^=PSG_NoiseShiftRegister&0x01;
	if (PSG_ToneNoise[3]&0x4)
	{
		newBit = ((PSG_NoiseShiftRegister&0x08)>>3) ^ (PSG_NoiseShiftRegister&0x01);
	}
	else
	{
		newBit = (PSG_NoiseShiftRegister&0x01);
	}
	PSG_NoiseShiftRegister>>=1;
	PSG_NoiseShiftRegister|=newBit<<15;
}

void PSG_UpdateNoise()
{
	if (PSG_ToneCounter[3]==0)		/* special case, use tone 2 value as input - but tone2 was off, so re-load */
	{
		PSG_ToneCounter[3]=PSG_ToneCounter[2];
	}
	else
	{
		PSG_ToneCounter[3]--;
		if (PSG_ToneCounter[3]==0)
		{
			if (PSG_NoiseCounter==0)
			{
				PSG_UpdateNoiseShiftRegister();
			}
			PSG_NoiseCounter^=1;
			PSG_ToneCounter[3]=(PSG_ToneNoise[3]+1)&0x3;
			if (PSG_ToneCounter[3]==0)
			{
				PSG_ToneCounter[3]=PSG_ToneCounter[2];
			}
			else
			{
				PSG_ToneCounter[3]<<=4;	
			}
		}
	}
}

#define MAX_OUTPUT_LEVEL		(0x1FFF)

#define VOL_TYPE		U16

VOL_TYPE logScale[16]=													/* stolen from speccy, log may be wrong */
{
	(VOL_TYPE)(MAX_OUTPUT_LEVEL * 0.00000f),
	(VOL_TYPE)(MAX_OUTPUT_LEVEL * 0.01370f),
	(VOL_TYPE)(MAX_OUTPUT_LEVEL * 0.02050f),
	(VOL_TYPE)(MAX_OUTPUT_LEVEL * 0.02910f),
	(VOL_TYPE)(MAX_OUTPUT_LEVEL * 0.04230f),
	(VOL_TYPE)(MAX_OUTPUT_LEVEL * 0.06180f),
	(VOL_TYPE)(MAX_OUTPUT_LEVEL * 0.08470f),
	(VOL_TYPE)(MAX_OUTPUT_LEVEL * 0.13690f),
	(VOL_TYPE)(MAX_OUTPUT_LEVEL * 0.16910f),
	(VOL_TYPE)(MAX_OUTPUT_LEVEL * 0.26470f),
	(VOL_TYPE)(MAX_OUTPUT_LEVEL * 0.35270f),
	(VOL_TYPE)(MAX_OUTPUT_LEVEL * 0.44990f),
	(VOL_TYPE)(MAX_OUTPUT_LEVEL * 0.57040f),
	(VOL_TYPE)(MAX_OUTPUT_LEVEL * 0.68730f),
	(VOL_TYPE)(MAX_OUTPUT_LEVEL * 0.84820f),
	(VOL_TYPE)(MAX_OUTPUT_LEVEL * 1.00000f)
};

U16 PSG_Output()
{
	int a;
	U16 sample=0;

	for (a=0;a<4;a++)
	{
		if (PSG_ToneOut[a])
		{
			sample+=logScale[15-PSG_Vol[a]];
		}
	}

	return sample;
}

void _AudioAddData(int channel,S16 dacValue);		/* Externally defined route to sound player */

void PSG_Update()
{
	/* just do tones for now */
	PSG_MasterClock++;
	if (PSG_MasterClock>=16)
	{
		PSG_MasterClock=0;

		PSG_UpdateTones();
		PSG_UpdateNoise();

		_AudioAddData(1,PSG_Output());
	}
}

void PSG_UpdateRegLo(U8 data)
{
	int channel=(PSG_AddressLatch&0x60)>>5;
	if (PSG_AddressLatch&0x10)
	{
		PSG_Vol[channel]=data&0x0F;
	}
	else
	{
		PSG_ToneNoise[channel]&=~0x0F;
		PSG_ToneNoise[channel]|=data&0x0F;
		if (channel==3)
		{
			PSG_NoiseShiftRegister=0x8000;
			PSG_ToneCounter[3]=(PSG_ToneNoise[3]+1)&0x3;
			if (PSG_ToneCounter[3]==0)
			{
				PSG_ToneCounter[3]=PSG_ToneCounter[2];
			}
			else
			{
				PSG_ToneCounter[3]<<=4;	
			}
		}
	}
}

void PSG_UpdateRegHi(U8 data)
{
	int channel=(PSG_AddressLatch&0x60)>>5;
	if (PSG_AddressLatch&0x10)
	{
		PSG_Vol[channel]=data&0x0F;
	}
	else
	{
		if (channel==3)		/* special case (noise reg is 4 bit) */
		{
			PSG_ToneNoise[channel]&=~0x0F;
			PSG_ToneNoise[channel]|=data&0x0F;
			PSG_NoiseShiftRegister=0x8000;
			PSG_ToneCounter[3]=(PSG_ToneNoise[3]+1)&0x3;
			if (PSG_ToneCounter[3]==0)
			{
				PSG_ToneCounter[3]=PSG_ToneCounter[2];
			}
			else
			{
				PSG_ToneCounter[3]<<=4;	
			}
		}
		else
		{
			PSG_ToneNoise[channel]&=~0x3F0;
			PSG_ToneNoise[channel]|=(data&0x3F)<<4;
		}
	}
}

void PSG_Write(U8 data)
{
	if (data&0x80)
	{
		PSG_AddressLatch=data;
		PSG_UpdateRegLo(data&0xF);
	}
	else
	{
		PSG_UpdateRegHi(data&0x3F);
	}
}
