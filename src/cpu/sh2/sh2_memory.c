/*
 * SH2_memory.c

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

#include "config.h"
#include "mytypes.h"
#include "platform.h"

#include "sh2.h"
#include "sh2_memory.h"
#include "sh2_ioregisters.h"

U8 SH2_WayTableUpdate[SH2_CACHE_NUM_WAYS]			={0x00,0x20,0x14,0x0B};
U8 SH2_WayTableUpdateMask[SH2_CACHE_NUM_WAYS]	={0x07,0x19,0x2A,0x34};

U8 SH2_WayTableReplacement[SH2_CACHE_NUM_WAYS]		={0x38,0x06,0x01,0x00};
U8 SH2_WayTableReplacementMask[SH2_CACHE_NUM_WAYS]={0x38,0x26,0x15,0x0B};

U8* SH2_CacheGet(SH2_State* cpu,U32 address,U8 dataFetch,U8 onWrite)
{
	U32 way,b;
	U32 lineFetchAddress = address & (~(SH2_CACHE_LINE_SIZE-1));
	U32 tag = address & (~(((SH2_CACHE_ENTRY_SIZE-1)<<SH2_CACHE_LINE_BITS)|(SH2_CACHE_LINE_SIZE-1)));
	U16 cacheEntry = (address>>SH2_CACHE_LINE_BITS)&(SH2_CACHE_ENTRY_SIZE-1);
	U8	lineOffset = (address&(SH2_CACHE_LINE_SIZE-1));

	UNUSED_ARGUMENT(dataFetch);

	/* We have the cache line entry, now check 4 ways to see if the tag matches */
	for (way=0;way<SH2_CACHE_NUM_WAYS;way++)
	{
		if (cpu->CacheAddress[cacheEntry].valid[way])
		{
			if (cpu->CacheAddress[cacheEntry].tagAddress[way]==tag)
			{
				/* Cache hit */
				cpu->CacheAddress[cacheEntry].LRU&=SH2_WayTableUpdateMask[way];
				cpu->CacheAddress[cacheEntry].LRU|=SH2_WayTableUpdate[way];
				return &cpu->CacheData[cacheEntry*SH2_CACHE_LINE_SIZE + SH2_CACHE_LINE_SIZE*SH2_CACHE_ENTRY_SIZE*way + lineOffset];
			}
		}
	}

	if (onWrite)
	{
		return NULL;
	}

	/* Cache miss */

	if (cpu->IOMemory[SH2_IO_CCR]&SH2_IO_CCR_TW)
	{
		/* 2 way mode */
		for (way=0;way<SH2_CACHE_NUM_WAYS;way++)
		{
			U8 LRU=cpu->CacheAddress[cacheEntry].LRU & SH2_WayTableReplacementMask[way];

			if (LRU==SH2_WayTableReplacement[way])
			{
				if (way<2)			/* HACK */
					way+=2;

				cpu->CacheAddress[cacheEntry].valid[way]=1;
				cpu->CacheAddress[cacheEntry].tagAddress[way]=tag;
				cpu->CacheAddress[cacheEntry].LRU&=SH2_WayTableUpdateMask[way];
				cpu->CacheAddress[cacheEntry].LRU|=SH2_WayTableUpdate[way];

				for(b=0;b<SH2_CACHE_LINE_SIZE;b++)
				{
					cpu->CacheData[cacheEntry*SH2_CACHE_LINE_SIZE + SH2_CACHE_LINE_SIZE*SH2_CACHE_ENTRY_SIZE*way + b]=cpu->readByte(lineFetchAddress+b);
				}

				return &cpu->CacheData[cacheEntry*SH2_CACHE_LINE_SIZE + SH2_CACHE_LINE_SIZE*SH2_CACHE_ENTRY_SIZE*way + lineOffset];
			}
		}
	}
	else
	{
		/* 4 way mode */
		for (way=0;way<SH2_CACHE_NUM_WAYS;way++)
		{
			U8 LRU=cpu->CacheAddress[cacheEntry].LRU & SH2_WayTableReplacementMask[way];

			if (LRU==SH2_WayTableReplacement[way])
			{
				cpu->CacheAddress[cacheEntry].valid[way]=1;
				cpu->CacheAddress[cacheEntry].tagAddress[way]=tag;
				cpu->CacheAddress[cacheEntry].LRU&=SH2_WayTableUpdateMask[way];
				cpu->CacheAddress[cacheEntry].LRU|=SH2_WayTableUpdate[way];

				for(b=0;b<SH2_CACHE_LINE_SIZE;b++)
				{
					cpu->CacheData[cacheEntry*SH2_CACHE_LINE_SIZE + SH2_CACHE_LINE_SIZE*SH2_CACHE_ENTRY_SIZE*way + b]=cpu->readByte(lineFetchAddress+b);
				}

				return &cpu->CacheData[cacheEntry*SH2_CACHE_LINE_SIZE + SH2_CACHE_LINE_SIZE*SH2_CACHE_ENTRY_SIZE*way + lineOffset];
			}
		}
	}

	DEB_PauseEmulation(cpu->pauseMode,"SH2 LRU Failure - cache implementation is flawed");
	return NULL;
}

void SH2_Write_Cache_Byte(SH2_State* cpu,U32 address,U8 data)
{
	if ((cpu->IOMemory[SH2_IO_CCR]&SH2_IO_CCR_CE))		/* Cache enabled - cache is write through so this is relatively simple */
	{
		U8 *cachePtr=SH2_CacheGet(cpu,address,1,1);

		if (cachePtr)
		{
			*cachePtr=data;
		}
	}
	cpu->writeByte(address,data);
	return;
}

void SH2_Write_Cache_Purge(SH2_State* cpu,U32 address,U8 data)
{
	U32 way;
	U32 tag = address & (~(((SH2_CACHE_ENTRY_SIZE-1)<<SH2_CACHE_LINE_BITS)|(SH2_CACHE_LINE_SIZE-1)));
	U16 cacheEntry = (address>>SH2_CACHE_LINE_BITS)&(SH2_CACHE_ENTRY_SIZE-1);

	UNUSED_ARGUMENT(data);

	/* We have the cache line entry, now check 4 ways to see if the tag matches */
	for (way=0;way<SH2_CACHE_NUM_WAYS;way++)
	{
		if (cpu->CacheAddress[cacheEntry].valid[way])
		{
			if (cpu->CacheAddress[cacheEntry].tagAddress[way]==tag)
			{
				cpu->CacheAddress[cacheEntry].valid[way]=0;			/* What about LRU?? */
			}
		}
	}
	return;
}

void SH2_Write_Cache_Address(SH2_State* cpu,U32 address,U8 data)
{
	UNUSED_ARGUMENT(address);
	UNUSED_ARGUMENT(data);
	DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to write to cache Address");
}

void SH2_Write_Special(SH2_State* cpu,U32 address,U8 data)
{
	if (address<0x07FF8000)			/* F8000000 - FFFF7FFF */	/* Reserved Ranges */
	{
		DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to write to reserved address range");
		return;
	}
	if (address<0x07FFC000)				/* FFFF8000 - FFFFBFFF */ /* Synchronous DRAM */
	{
		switch(address&0x7FF)
		{
		case 0x426:
		case 0x427:
		case 0x848:
		case 0x849:
		case 0x84A:
		case 0x84B:
			/* CAS Latency 1 */
			return;
		case 0x446:
		case 0x447:
		case 0x888:
		case 0x889:
		case 0x88A:
		case 0x88B:
			/* CAS Latency 2 */
			return;
		case 0x466:
		case 0x467:
		case 0x8C8:
		case 0x8C9:
		case 0x8CA:
		case 0x8CB:
			/* CAS Latency 3 */
			return;
		}
		DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to write to unimplemented address range");
		return;
	}
	if (address<0x07FFFE00)				/* FFFFC000 - FFFFFDFF */ /* Reserved Ranges */
	{
		DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to write to reserved address range");
		return;
	}
																/* FFFFFE00 - FFFFFFFF */ /* System Registers */

	SH2_IO_Write_Byte(cpu,address-0x07FFFE00,data);
}

void SH2_Write_Byte(SH2_State* cpu,U32 adr,U8 data)
{
	U32 externalAddress=adr&0x07FFFFFF;

	switch (adr&0xF8000000)
	{
	case 0x00000000:	/* 0x00000000 - 0x07FFFFFF */			/* Cache Access */
		SH2_Write_Cache_Byte(cpu,externalAddress,data);
		return;
	case 0x08000000:	/* 0x08000000 - 0x0FFFFFFF */			/* Reserved Ranges */
	case 0x10000000:	/* 0x10000000 - 0x17FFFFFF */
	case 0x18000000:	/* 0x18000000 - 0x1FFFFFFF */
		DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to write to reserved address range");
		break;
	case 0x20000000:	/* 0x20000000 - 0x27FFFFFF */			/* Cache Through Access */
		cpu->writeByte(externalAddress,data);
		return;
	case 0x28000000:	/* 0x28000000 - 0x2FFFFFFF */			/* Reserved Ranges */
	case 0x30000000:	/* 0x30000000 - 0x37FFFFFF */
	case 0x38000000:	/* 0x38000000 - 0x3FFFFFFF */
		DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to write to reserved address range");
		break;
	case 0x40000000:	/* 0x40000000 - 0x47FFFFFF */			/* Cache Associative Purge */
		SH2_Write_Cache_Purge(cpu,externalAddress,data);
		return;
	case 0x48000000:	/* 0x48000000 - 0x4FFFFFFF */			/* Reserved Ranges */
	case 0x50000000:	/* 0x50000000 - 0x57FFFFFF */
	case 0x58000000:	/* 0x58000000 - 0x5FFFFFFF */
		DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to write to reserved address range");
		break;
	case 0x60000000:	/* 0x60000000 - 0x67FFFFFF */			/* Address Array Write */
	case 0x68000000:	/* 0x68000000 - 0x6FFFFFFF */
	case 0x70000000:	/* 0x70000000 - 0x77FFFFFF */
	case 0x78000000:	/* 0x78000000 - 0x7FFFFFFF */
		SH2_Write_Cache_Address(cpu,externalAddress,data);
		return;
	case 0x80000000:	/* 0x80000000 - 0x87FFFFFF */			/* Reserved Ranges */
	case 0x88000000:	/* 0x88000000 - 0x8FFFFFFF */
	case 0x90000000:	/* 0x90000000 - 0x97FFFFFF */
	case 0x98000000:	/* 0x98000000 - 0x9FFFFFFF */
	case 0xA0000000:	/* 0xA0000000 - 0xA7FFFFFF */
	case 0xA8000000:	/* 0xA8000000 - 0xAFFFFFFF */
	case 0xB0000000:	/* 0xB0000000 - 0xB7FFFFFF */
	case 0xB8000000:	/* 0xB8000000 - 0xBFFFFFFF */
		DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to write to reserved address range");
		break;
	case 0xC0000000:	/* 0xC0000000 - 0xC0000FFF */			/* Data Array Write */
		if (externalAddress<0x1000)
		{
			cpu->CacheData[externalAddress]=data;
			return;
		}
										/* 0xC0001000 - 0xC7FFFFFF */
		DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to write to reserved address range");
		break;
	case 0xC8000000:	/* 0xC8000000 - 0xCFFFFFFF */			/* Reserved Ranges */
	case 0xD0000000:	/* 0xD0000000 - 0xD7FFFFFF */
	case 0xD8000000:	/* 0xD8000000 - 0xDFFFFFFF */
	case 0xE0000000:	/* 0xE0000000 - 0xE7FFFFFF */
	case 0xE8000000:	/* 0xE8000000 - 0xEFFFFFFF */
	case 0xF0000000:	/* 0xF0000000 - 0xF7FFFFFF */
		DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to write to reserved address range");
		break;
	case 0xF8000000:	/* 0xF8000000 - 0xFFFFFFFF */			/* Special Registers */
		SH2_Write_Special(cpu,externalAddress,data);
		return;
	}
}

U8 SH2_Read_Cache_Byte(SH2_State* cpu,U32 address,U8 dataFetch)
{
	U8 *cachePtr;

	if ((cpu->IOMemory[SH2_IO_CCR]&SH2_IO_CCR_CE)==0)		/* Cache disabled */
	{
		return cpu->readByte(address);
	}

	cachePtr=SH2_CacheGet(cpu,address,dataFetch,0);

	return *cachePtr;
}

U8 SH2_Read_Cache_Purge(SH2_State* cpu,U32 address)
{
	UNUSED_ARGUMENT(address);
	DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to read cache purge area");
	return 0x00;
}

U8 SH2_Read_Cache_Address(SH2_State* cpu,U32 address)
{
	UNUSED_ARGUMENT(address);
	DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to read cache address area");
	return 0x00;
}

U8 SH2_Read_Special(SH2_State* cpu,U32 address)
{
	if (address<0x07FF8000)			/* F8000000 - FFFF7FFF */	/* Reserved Ranges */
	{
		DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to read reserved address range");
		return 0x00;
	}
	if (address<0x07FFC000)				/* FFFF8000 - FFFFBFFF */ /* Synchronous DRAM */
	{
		DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to read unimplemented address range");
		return 0x00;
	}
	if (address<0x07FFFE00)				/* FFFFC000 - FFFFFDFF */ /* Reserved Ranges */
	{
		DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to read reserved address range");
		return 0x00;
	}
																/* FFFFFE00 - FFFFFFFF */ /* System Registers */

	return SH2_IO_Read_Byte(cpu,address-0x07FFFE00);
}

U8 SH2_Read_Byte(SH2_State* cpu,U32 adr,U8 dataFetch)
{
	U32 externalAddress=adr&0x07FFFFFF;

	switch (adr&0xF8000000)
	{
	case 0x00000000:	/* 0x00000000 - 0x07FFFFFF */			/* Cache Access */
		return SH2_Read_Cache_Byte(cpu,externalAddress,dataFetch);
	case 0x08000000:	/* 0x08000000 - 0x0FFFFFFF */			/* Reserved Ranges */
	case 0x10000000:	/* 0x10000000 - 0x17FFFFFF */
	case 0x18000000:	/* 0x18000000 - 0x1FFFFFFF */
		DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to read reserved address range");
		break;
	case 0x20000000:	/* 0x20000000 - 0x27FFFFFF */			/* Cache Through Access */
		return cpu->readByte(externalAddress);
	case 0x28000000:	/* 0x28000000 - 0x2FFFFFFF */			/* Reserved Ranges */
	case 0x30000000:	/* 0x30000000 - 0x37FFFFFF */
	case 0x38000000:	/* 0x38000000 - 0x3FFFFFFF */
		DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to read reserved address range");
		break;
	case 0x40000000:	/* 0x40000000 - 0x47FFFFFF */			/* Cache Associative Purge */
		return SH2_Read_Cache_Purge(cpu,externalAddress);
	case 0x48000000:	/* 0x48000000 - 0x4FFFFFFF */			/* Reserved Ranges */
	case 0x50000000:	/* 0x50000000 - 0x57FFFFFF */
	case 0x58000000:	/* 0x58000000 - 0x5FFFFFFF */
		DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to read reserved address range");
		break;
	case 0x60000000:	/* 0x60000000 - 0x67FFFFFF */			/* Address Array Read */
	case 0x68000000:	/* 0x68000000 - 0x6FFFFFFF */
	case 0x70000000:	/* 0x70000000 - 0x77FFFFFF */
	case 0x78000000:	/* 0x78000000 - 0x7FFFFFFF */
		return SH2_Read_Cache_Address(cpu,externalAddress);
	case 0x80000000:	/* 0x80000000 - 0x87FFFFFF */			/* Reserved Ranges */
	case 0x88000000:	/* 0x88000000 - 0x8FFFFFFF */
	case 0x90000000:	/* 0x90000000 - 0x97FFFFFF */
	case 0x98000000:	/* 0x98000000 - 0x9FFFFFFF */
	case 0xA0000000:	/* 0xA0000000 - 0xA7FFFFFF */
	case 0xA8000000:	/* 0xA8000000 - 0xAFFFFFFF */
	case 0xB0000000:	/* 0xB0000000 - 0xB7FFFFFF */
	case 0xB8000000:	/* 0xB8000000 - 0xBFFFFFFF */
		DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to read reserved address range");
		break;
	case 0xC0000000:	/* 0xC0000000 - 0xC0000FFF */			/* Data Array Read */
		if (externalAddress<0x1000)
			return cpu->CacheData[externalAddress];
										/* 0xC0001000 - 0xC7FFFFFF */
		DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to read reserved address range");
		break;
	case 0xC8000000:	/* 0xC8000000 - 0xCFFFFFFF */			/* Reserved Ranges */
	case 0xD0000000:	/* 0xD0000000 - 0xD7FFFFFF */
	case 0xD8000000:	/* 0xD8000000 - 0xDFFFFFFF */
	case 0xE0000000:	/* 0xE0000000 - 0xE7FFFFFF */
	case 0xE8000000:	/* 0xE8000000 - 0xEFFFFFFF */
	case 0xF0000000:	/* 0xF0000000 - 0xF7FFFFFF */
		DEB_PauseEmulation(cpu->pauseMode,"SH2 Attempt to read reserved address range");
		break;
	case 0xF8000000:	/* 0xF8000000 - 0xFFFFFFFF */			/* Special Registers */
		return SH2_Read_Special(cpu,externalAddress);
	}
	return 0x00;
}

void SH2_Write_Long(SH2_State* cpu,U32 adr,U32 data)
{
	SH2_Write_Byte(cpu,adr+0,(data>>24)&0xFF);
	SH2_Write_Byte(cpu,adr+1,(data>>16)&0xFF);
	SH2_Write_Byte(cpu,adr+2,(data>>8)&0xFF);
	SH2_Write_Byte(cpu,adr+3,(data>>0)&0xFF);
}

void SH2_Write_Word(SH2_State* cpu,U32 adr,U16 data)
{
	SH2_Write_Byte(cpu,adr+0,(data>>8)&0xFF);
	SH2_Write_Byte(cpu,adr+1,(data>>0)&0xFF);
}

U32 SH2_Read_Long(SH2_State* cpu,U32 adr,U8 dataFetch)
{
	U32 result;

	result =SH2_Read_Byte(cpu,adr+0,dataFetch)<<24;
	result|=SH2_Read_Byte(cpu,adr+1,dataFetch)<<16;
	result|=SH2_Read_Byte(cpu,adr+2,dataFetch)<<8;
	result|=SH2_Read_Byte(cpu,adr+3,dataFetch)<<0;

	return result;
}

U16 SH2_Read_Word(SH2_State* cpu,U32 adr,U8 dataFetch)
{
	U16 result;

	result =SH2_Read_Byte(cpu,adr+0,dataFetch)<<8;
	result|=SH2_Read_Byte(cpu,adr+1,dataFetch)<<0;

	return result;
}

/* For now, just go straight to memory read function - this won't work for cached memory */
U16 SH2_Debug_Read_Word(SH2_State* cpu,U32 adr)
{
	U16 result;

	result =cpu->readByte(adr+0)<<8;/*SH2_Read_Byte(cpu,adr+0)<<8;*/
	result|=cpu->readByte(adr+1);/*SH2_Read_Byte(cpu,adr+1)<<0;*/

	return result;
}


