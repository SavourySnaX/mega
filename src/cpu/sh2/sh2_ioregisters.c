/*
 * SH2_IORegisters.c

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

#include "config.h"
#include "mytypes.h"
#include "platform.h"

#include "sh2.h"
#include "sh2_ioregisters.h"

void SH2_IO_CCR_WRITE(SH2_State* cpu,U8 byte)
{
	if (byte&SH2_IO_CCR_CP)
	{
		U32 a;
		for (a=0;a<SH2_CACHE_ENTRY_SIZE*SH2_CACHE_NUM_WAYS;a++)
		{
			cpu->CacheAddress[a/SH2_CACHE_NUM_WAYS].valid[a%SH2_CACHE_NUM_WAYS]=0;
			cpu->CacheAddress[a/SH2_CACHE_NUM_WAYS].LRU=0;
		}
	}
	if (byte&(SH2_IO_CCR_ID|SH2_IO_CCR_OD))
	{
		DEB_PauseEmulation(cpu->pauseMode,"SH2 Cache bit unsupported");
	}
}

void SH2_IO_WRITE_DVDNTLL(SH2_State* cpu,U8 byte)
{
	U32 endTmp;
	U32 sorTmp;
	S32	dividend;
	S32 divisor;
	S32 remainder;
	S32 quotient;

	UNUSED_ARGUMENT(byte);

	/* TODO - Set flag indicating division in progress, so we can delay reads from results register until operation complete */
	
	/* For now we simply calculate result in place */

	sorTmp = ((U32)cpu->IOMemory[SH2_IO_DVSRHH])<<24;
	sorTmp|= ((U32)cpu->IOMemory[SH2_IO_DVSRHL])<<16;
	sorTmp|= ((U32)cpu->IOMemory[SH2_IO_DVSRLH])<<8;
	sorTmp|= ((U32)cpu->IOMemory[SH2_IO_DVSRLL]);

	endTmp = ((U32)cpu->IOMemory[SH2_IO_DVDNTHH])<<24;
	endTmp|= ((U32)cpu->IOMemory[SH2_IO_DVDNTHL])<<16;
	endTmp|= ((U32)cpu->IOMemory[SH2_IO_DVDNTLH])<<8;
	endTmp|= ((U32)cpu->IOMemory[SH2_IO_DVDNTLL]);

	divisor=sorTmp;
	dividend=endTmp;

	if (divisor!=0)
	{
		remainder=dividend%divisor;
		quotient=dividend/divisor;
	}
	else
	{
		remainder=quotient=0;
	}

	/* TODO handle overflow */

	cpu->IOMemory[SH2_IO_DVDNTHHH]=(remainder>>24)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTHHL]=(remainder>>16)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTHLH]=(remainder>>8)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTHLL]=(remainder>>0)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTHHH+0x08]=(remainder>>24)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTHHL+0x08]=(remainder>>16)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTHLH+0x08]=(remainder>>8)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTHLL+0x08]=(remainder>>0)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTHH]=(quotient>>24)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTHL]=(quotient>>16)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTLH]=(quotient>>8)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTLL]=(quotient>>0)&0xFF;
}

void SH2_IO_WRITE_DVDNTLLL(SH2_State* cpu,U8 byte)
{
	U64 endTmp;
	U32 sorTmp;
	S64	dividend;
	S32 divisor;
	S32 remainder;
	S64 quotient;

	UNUSED_ARGUMENT(byte);

	/* TODO - Set flag indicating division in progress, so we can delay reads from results register until operation complete */
	
	/* For now we simply calculate result in place */

	sorTmp = ((U32)cpu->IOMemory[SH2_IO_DVSRHH])<<24;
	sorTmp|= ((U32)cpu->IOMemory[SH2_IO_DVSRHL])<<16;
	sorTmp|= ((U32)cpu->IOMemory[SH2_IO_DVSRLH])<<8;
	sorTmp|= ((U32)cpu->IOMemory[SH2_IO_DVSRLL]);

	endTmp = ((U64)cpu->IOMemory[SH2_IO_DVDNTHHH])<<56;
	endTmp|= ((U64)cpu->IOMemory[SH2_IO_DVDNTHHL])<<48;
	endTmp|= ((U64)cpu->IOMemory[SH2_IO_DVDNTHLH])<<40;
	endTmp|= ((U64)cpu->IOMemory[SH2_IO_DVDNTHLL])<<32;
	endTmp|= ((U64)cpu->IOMemory[SH2_IO_DVDNTLHH])<<24;
	endTmp|= ((U64)cpu->IOMemory[SH2_IO_DVDNTLHL])<<16;
	endTmp|= ((U64)cpu->IOMemory[SH2_IO_DVDNTLLH])<<8;
	endTmp|= ((U64)cpu->IOMemory[SH2_IO_DVDNTLLL]);

	divisor=sorTmp;
	dividend=endTmp;

	if (divisor!=0)
	{
		remainder=dividend%divisor;
		quotient=dividend/divisor;
	}
	else
	{
		remainder=quotient=0;
	}

	/* TODO - handle overflow */

	cpu->IOMemory[SH2_IO_DVDNTHHH]=(remainder>>24)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTHHL]=(remainder>>16)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTHLH]=(remainder>>8)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTHLL]=(remainder>>0)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTLHH]=(quotient>>24)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTLHL]=(quotient>>16)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTLLH]=(quotient>>8)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTLLL]=(quotient>>0)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTHHH+0x08]=(remainder>>24)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTHHL+0x08]=(remainder>>16)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTHLH+0x08]=(remainder>>8)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTHLL+0x08]=(remainder>>0)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTLHH+0x08]=(quotient>>24)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTLHL+0x08]=(quotient>>16)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTLLH+0x08]=(quotient>>8)&0xFF;
	cpu->IOMemory[SH2_IO_DVDNTLLL+0x08]=(quotient>>0)&0xFF;
}

void SH2_IO_WRITE_DMAKICK(SH2_State* cpu,U8 byte,int regBase,int channel)
{
	if ((byte&SH2_IO_CHCRLL_TE)==0)
	{
		cpu->IOMemory[SH2_IO_CHCR0LL+regBase]&=~SH2_IO_CHCRLL_TE;			/* Not completely correct, we should ensure that 1 has been read before allowing clear */
	}

	if ((cpu->IOMemory[SH2_IO_CHCR0LL+regBase] & SH2_IO_CHCRLL_TE) && (byte&SH2_IO_CHCRLL_DE))
	{
		printf("DMA Missed due to TE still being flagged\n");	/* TODO check DMAOR (NMIF and AE bits too) */
		cpu->IOMemory[SH2_IO_CHCR0LL+regBase]&=~SH2_IO_CHCRLL_DE;
	}

	if ((cpu->IOMemory[SH2_IO_CHCR0LL+regBase] & SH2_IO_CHCRLL_DE) && (byte&SH2_IO_CHCRLL_DE))
	{
		printf("DMA Attempted to be started.. but already enabled!");
	}
	else
	{
		if (byte&SH2_IO_CHCRLL_DE)
		{
			/* Sanity check dma mode options */
			if ((cpu->IOMemory[SH2_IO_CHCR0LL+regBase]&SH2_IO_CHCRLL_TA)==SH2_IO_CHCRLL_TA)
			{
				DEB_PauseEmulation(cpu->pauseMode,"SH2 DMA Operation not implemented - skipping");
				printf("Unsupported DMA mode - skipping!\n");
				cpu->IOMemory[SH2_IO_CHCR0LL+regBase]|=SH2_IO_CHCRLL_TE;
				cpu->IOMemory[SH2_IO_CHCR0LL+regBase]&=~SH2_IO_CHCRLL_DE;
				return;
			}
			if ((cpu->IOMemory[SH2_IO_CHCR0LH+regBase]&(SH2_IO_CHCRLH_SM0|SH2_IO_CHCRLH_SM1))==(SH2_IO_CHCRLH_SM0|SH2_IO_CHCRLH_SM1))
			{
				printf("Invalid Source Address Mode - skipping!\n");
				DEB_PauseEmulation(cpu->pauseMode,"SH2 DMA Operation invalid - skipping");
				cpu->IOMemory[SH2_IO_CHCR0LL+regBase]|=SH2_IO_CHCRLL_TE;
				cpu->IOMemory[SH2_IO_CHCR0LL+regBase]&=~SH2_IO_CHCRLL_DE;
				return;
			}
			if ((cpu->IOMemory[SH2_IO_CHCR0LH+regBase]&(SH2_IO_CHCRLH_DM0|SH2_IO_CHCRLH_DM1))==(SH2_IO_CHCRLH_DM0|SH2_IO_CHCRLH_DM1))
			{
				printf("Invalid Destination Address Mode - skipping!\n");
				DEB_PauseEmulation(cpu->pauseMode,"SH2 DMA Operation invalid - skipping");
				cpu->IOMemory[SH2_IO_CHCR0LL+regBase]|=SH2_IO_CHCRLL_TE;
				cpu->IOMemory[SH2_IO_CHCR0LL+regBase]&=~SH2_IO_CHCRLL_DE;
				return;
			}
			cpu->IOMemory[SH2_IO_CHCR0LL+regBase]|=SH2_IO_CHCRLL_DE;
			cpu->dmaPhase[channel]=0;
		}
		else
		{
			cpu->IOMemory[SH2_IO_CHCR0LL+regBase]&=~SH2_IO_CHCRLL_DE;	/* Abort dma */
			cpu->dmaPhase[channel]=0;
		}
	}
}


void SH2_IO_WRITE_CHCR0LL(SH2_State* cpu,U8 byte)
{
	SH2_IO_WRITE_DMAKICK(cpu,byte,0x00,0);
}

void SH2_IO_WRITE_CHCR1LL(SH2_State* cpu,U8 byte)
{
	SH2_IO_WRITE_DMAKICK(cpu,byte,0x10,1);
}

/* IO Accessor Table */

SH2_IO_RegisterSettings	ioMemoryMap[]=
{
	{0xE00+SH2_IO_SMR				,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BRR				,{0xFF,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_SCR				,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_TDR				,{0xFF,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_SSR				,{0x84,0xFF,0xF9,0x00},NULL,NULL},
	{0xE00+SH2_IO_RDR				,{0x00,0xFF,0x00,0x00},NULL,NULL},

	{0xE00+SH2_IO_TIER			,{0x01,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_FTCSR			,{0x00,0x8F,0x8F,0x00},NULL,NULL},
	{0xE00+SH2_IO_FRCH			,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_FRCL			,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_OCRABH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},		/* Dual register - need special handling */
	{0xE00+SH2_IO_OCRABL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},		/* Dual register - need special handling */
	{0xE00+SH2_IO_TCR				,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_TOCR			,{0xE0,0x1F,0x1F,0x00},NULL,NULL},
	{0xE00+SH2_IO_FICRH			,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_FICRL			,{0x00,0xFF,0x00,0x00},NULL,NULL},

	{0xE00+SH2_IO_IPRAH			,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_IPRAL			,{0x00,0xFF,0xF0,0x00},NULL,NULL},
	{0xE00+SH2_IO_IPRBH			,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_IPRBL			,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRAH			,{0x00,0xFF,0x7F,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRAL			,{0x00,0xFF,0x7F,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRBH			,{0x00,0xFF,0x7F,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRBL			,{0x00,0xFF,0x7F,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRCH			,{0x00,0xFF,0x7F,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRCL			,{0x00,0xFF,0x7F,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRDH			,{0x00,0xFF,0x7F,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRDL			,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRWDTH		,{0x00,0xFF,0x7F,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRWDTL		,{0x00,0xFF,0x7F,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRDIVHH	,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRDIVHL	,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRDIVLH	,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRDIVLL	,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRDMA0HH	,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRDMA0HL	,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRDMA0LH	,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRDMA0LL	,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRDMA1HH	,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRDMA1HL	,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRDMA1LH	,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_VCRDMA1LL	,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_ICRH			,{0x00,0xFF,0x01,0x00},NULL,NULL},
	{0xE00+SH2_IO_ICRL			,{0x00,0xFF,0x01,0x00},NULL,NULL},

	{0xE00+SH2_IO_WTCSR			,{0x18,0xFF,0x00,0x00},NULL,NULL},	/* Write needs special handling */
	{0xE00+SH2_IO_WTCNT			,{0x00,0xFF,0x00,0x00},NULL,NULL},	/* Write needs special handling */
	{0xE00+SH2_IO_RSTCSRW		,{0x1F,0x00,0xE0,0x00},NULL,NULL},	/* Write needs to copy value to below */
	{0xE00+SH2_IO_RSTCSRR		,{0x1F,0xFF,0x00,0x00},NULL,NULL},

	{0xE00+SH2_IO_DVSRHH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DVSRHL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DVSRLH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DVSRLL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DVDNTHH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DVDNTHL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DVDNTLH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DVDNTLL		,{0x00,0xFF,0x00,0x00},NULL,SH2_IO_WRITE_DVDNTLL},			/* Handler will write the result byte, don't want normal io handling to trash it */
	{0xE00+SH2_IO_DVCRHH		,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_DVCRHL		,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_DVCRLH		,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_DVCRLL		,{0x00,0xFF,0x03,0x00},NULL,NULL},
	{0xE00+SH2_IO_DVDNTHHH	,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DVDNTHHL	,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DVDNTHLH	,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DVDNTHLL	,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DVDNTLHH	,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DVDNTLHL	,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DVDNTLLH	,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DVDNTLLL	,{0x00,0xFF,0x00,0x00},NULL,SH2_IO_WRITE_DVDNTLLL},			/* Handler will write the result byte, don't want normal io handling to trash it */

	{0xE00+SH2_IO_BARAHH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BARAHL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BARALH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BARALL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BAMRAHH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BAMRAHL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BAMRALH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BAMRALL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BBRAH			,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_BBRAL			,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BARBHH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BARBHL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BARBLH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BARBLL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BAMRBHH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BAMRBHL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BAMRBLH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BAMRBLL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BBRBH			,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_BBRBL			,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BDRBHH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BDRBHL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BDRBLH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BDRBLL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BDMRBHH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BDMRBHL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BDMRBLH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BDMRBLL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_BRCRH			,{0x00,0xFF,0xF4,0x00},NULL,NULL},
	{0xE00+SH2_IO_BRCRL			,{0x00,0xFF,0xDC,0x00},NULL,NULL},

	{0xE00+SH2_IO_SAR0HH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_SAR0HL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_SAR0LH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_SAR0LL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DAR0HH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DAR0HL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DAR0LH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DAR0LL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_TCR0HH		,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_TCR0HL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_TCR0LH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_TCR0LL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_CHCR0HH		,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_CHCR0HL		,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_CHCR0LH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_CHCR0LL		,{0x00,0xFF,0xFC,0x00},NULL,SH2_IO_WRITE_CHCR0LL},
	{0xE00+SH2_IO_SAR1HH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_SAR1HL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_SAR1LH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_SAR1LL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DAR1HH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DAR1HL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DAR1LH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_DAR1LL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_TCR1HH		,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_TCR1HL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_TCR1LH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_TCR1LL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_CHCR1HH		,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_CHCR1HL		,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_CHCR1LH		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_CHCR1LL		,{0x00,0xFF,0xFC,0x00},NULL,SH2_IO_WRITE_CHCR1LL},
	{0xE00+SH2_IO_DRCR0			,{0x00,0xFF,0x03,0x00},NULL,NULL},
	{0xE00+SH2_IO_DRCR1			,{0x00,0xFF,0x03,0x00},NULL,NULL},
	{0xE00+SH2_IO_DMAORHH		,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_DMAORHL		,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_DMAORLH		,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_DMAORLL		,{0x00,0xFF,0x0F,0x00},NULL,NULL},

	{0xE00+SH2_IO_BCR1H			,{0x03,0xFF,0x1F,0x00},NULL,NULL},
	{0xE00+SH2_IO_BCR1L			,{0xF0,0xFF,0xF7,0x00},NULL,NULL},
	{0xE00+SH2_IO_BCR2H			,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_BCR2L			,{0xFC,0xFF,0xFC,0x00},NULL,NULL},
	{0xE00+SH2_IO_WCRH			,{0xAA,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_WCRL			,{0xFF,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_MCRH			,{0x00,0xFF,0xFE,0x00},NULL,NULL},
	{0xE00+SH2_IO_MCRL			,{0x00,0xFF,0xFC,0x00},NULL,NULL},
	{0xE00+SH2_IO_RTCSRH		,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_RTCSRL		,{0x00,0xFF,0xF8,0x00},NULL,NULL},
	{0xE00+SH2_IO_RTCNTH		,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_RTCNTL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},
	{0xE00+SH2_IO_RTCORH		,{0x00,0xFF,0x00,0x00},NULL,NULL},
	{0xE00+SH2_IO_RTCORL		,{0x00,0xFF,0xFF,0x00},NULL,NULL},

	{0xE00+SH2_IO_CCR				,{0x00,0xCF,0xDF,0x00},NULL,SH2_IO_CCR_WRITE},

	{0xE00+SH2_IO_SBYCR			,{0x00,0xDF,0xDF,0x00},NULL,NULL},

	{0,{0,0,0,0},NULL,NULL}
};

void SH2_IO_Write_Byte(SH2_State* cpu,U32 reg,U8 data)
{
	if (cpu->IOTable[reg])
	{
		cpu->IOMemory[reg]&=~cpu->IOTable[reg]->regData.writeMask;
		cpu->IOMemory[reg]|=data & cpu->IOTable[reg]->regData.writeMask;
		if (cpu->IOTable[reg]->writeHandler)
		{
			cpu->IOTable[reg]->writeHandler(cpu,data);
		}

		return;
	}

	cpu->IOMemory[reg]=data;
}

U8 SH2_IO_Read_Byte(SH2_State* cpu,U32 reg)
{
	if (cpu->IOTable[reg])
	{
		if (cpu->IOTable[reg]->readHandler)
		{
			cpu->IOTable[reg]->readHandler(cpu);
		}
		return cpu->IOMemory[reg] & cpu->IOTable[reg]->regData.readMask;
	}
	return cpu->IOMemory[reg];
}

void SH2_IO_Reset(SH2_State* cpu)
{
	U32 a;

	for (a=0;a<0x200;a++)
	{
		cpu->IOTable[a]=NULL;
	}
	a=0;
	while (ioMemoryMap[a].regNumber!=0)
	{
		cpu->IOMemory[ioMemoryMap[a].regNumber - 0xE00]=ioMemoryMap[a].regData.initialValue;
		cpu->IOTable[ioMemoryMap[a].regNumber - 0xE00]=&ioMemoryMap[a];
		a++;
	}
}

