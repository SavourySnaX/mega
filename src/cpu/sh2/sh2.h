/*
 * SH2.h

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

#ifndef __SH2_H
#define __SH2_H

#include "mytypes.h"

#define SH2_CACHE_LINE_BITS			(4)
#define	SH2_CACHE_LINE_SIZE			(1<<SH2_CACHE_LINE_BITS)
#define SH2_CACHE_NUM_WAYS			(4)
#define SH2_CACHE_ENTRY_BITS		(6)
#define SH2_CACHE_ENTRY_SIZE		(1<<SH2_CACHE_ENTRY_BITS)
#define SH2_PIPELINE_SIZE				(5)

typedef struct SH2_State SH2_State;

typedef U8 (*SH2_READ_BYTE)(U32 adr);							/* Must be externally defined represents */
typedef void (*SH2_WRITE_BYTE)(U32 adr,U8 byte);  /*bus access.Note Adresses will be 27bit */
																									/*due to internal address space division.*/

typedef void (*SH2_IO_READ_HANDLER)(SH2_State* cpu);								/* Invoked prior to read masking */
typedef void (*SH2_IO_WRITE_HANDLER)(SH2_State* cpu,U8 data);				/* Invoked after write masking - with unmasked write data */


/* Status Register Flags */

#define SH2_SR_T		(1<<0)
#define SH2_SR_S		(1<<1)

#define SH2_SR_I0		(1<<4)
#define SH2_SR_I1		(1<<5)
#define SH2_SR_I2		(1<<6)
#define SH2_SR_I3		(1<<7)
#define SH2_SR_Q		(1<<8)
#define SH2_SR_M		(1<<9)

/* Global Register Hazards */

#define	SH2_HAZARD_R0			(1<<0)		/* Defined all, in case I switch to this method for all register hazards */
#define SH2_HAZARD_R1			(1<<1)
#define	SH2_HAZARD_R2			(1<<2)
#define SH2_HAZARD_R3			(1<<3)
#define	SH2_HAZARD_R4			(1<<4)
#define SH2_HAZARD_R5			(1<<5)
#define	SH2_HAZARD_R6			(1<<6)
#define SH2_HAZARD_R7			(1<<7)
#define	SH2_HAZARD_R8			(1<<8)
#define SH2_HAZARD_R9			(1<<9)
#define	SH2_HAZARD_R10		(1<<10)
#define SH2_HAZARD_R11		(1<<11)
#define	SH2_HAZARD_R12		(1<<12)
#define SH2_HAZARD_R13		(1<<13)
#define	SH2_HAZARD_R14		(1<<14)
#define SH2_HAZARD_R15		(1<<15)
#define	SH2_HAZARD_SR			(1<<16)
#define SH2_HAZARD_GBR		(1<<17)
#define	SH2_HAZARD_VBR		(1<<18)
#define SH2_HAZARD_MACL		(1<<19)
#define	SH2_HAZARD_MACH		(1<<20)
#define SH2_HAZARD_PR			(1<<21)
#define SH2_HAZARD_PC			(1<<22)

/* IO Register Structure */

typedef struct
{
	U8	initialValue;
	U8	readMask;
	U8	writeMask;
	U8	emulated;							/* Might use for indicating special handling required.. */
} SH2_IO_Register;

typedef struct
{
	U16										regNumber;
	SH2_IO_Register				regData;
	SH2_IO_READ_HANDLER		readHandler;
	SH2_IO_WRITE_HANDLER	writeHandler;
} SH2_IO_RegisterSettings;

/* Pipeline slot */

typedef struct  
{
	U32		PC;
	U32		stage;
	U16		opcode;
	U16		operands[8];

	U32		EA;			/* Effective Address */
	U32		VAL;

	U32*	RegHazard;
	U8		Discard;
	U8		Delay;
	U8		NoInterrupts;
}SH2_Slot;

typedef struct  
{
	U32		tagAddress[SH2_CACHE_NUM_WAYS];				/* tag of cache line */
	U8		valid[SH2_CACHE_NUM_WAYS];						/* entry is valid */
	U8		LRU;																	/* least recently used bits */
}SH2_CacheAddress;

/* CPU state declaration */

struct SH2_State
{
	U32 R[16];

	U32 SR;			/* -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  M  Q  I3 I2 I1 I0 -  -  S  T */
	U32 GBR;
	U32 VBR;

	U32	MACH;
	U32 MACL;
	U32 PR;
	U32 PC;

	/* Memory Mapped registers */

	U8												IOMemory[0x200];
	SH2_IO_RegisterSettings*	IOTable[0x200];

	/* Cache Memory */

	U8								CacheData[SH2_CACHE_LINE_SIZE*SH2_CACHE_ENTRY_SIZE*SH2_CACHE_NUM_WAYS];		/* split into 4 ways of 1024 bytes - 64 * 16byte lines */
	SH2_CacheAddress	CacheAddress[SH2_CACHE_ENTRY_SIZE];

	/* Internal registers */
	S64 op164;
	S64 op264;
	S64 res64;

	/* Pipeline */

	U32							IF;
	U32							IFMask;
	U32							PC2;					/* Stores PC for second instruction in IF */

	SH2_Slot				pipeLine[SH2_PIPELINE_SIZE];
	
	/* Global Register Hazards */

	U32							globalRegisterHazards;	/* see SH2_HAZARD_ */

	/* Overall state */

	U8 sleeping;
	U8 interrupt;

	/* DMA */
	U8	dmaPhase[2];
	U32	dmaTmp[2];
	U32	dAckCnt;

	/* Memory Access */
	SH2_READ_BYTE		readByte;
	SH2_WRITE_BYTE	writeByte;

	/* For Debugging */
	U8	traceEnabled;
	int	pauseMode;
};

typedef U16 (*SH2_Decode)(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
typedef U32 (*SH2_Function)(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);

typedef struct
{
	U8						baseTable[17];
	char					opcodeName[32];
	SH2_Function	opcode;
	SH2_Decode		decode;
	U8						numOperands;
	U16						operandMask[8];
	U16						operandShift[8];
	int						numValidMasks[8];
	char					validEffectiveAddress[8][64][16];
} SH2_Ins;

SH2_State *SH2_CreateCPU(SH2_READ_BYTE readByteFunction,SH2_WRITE_BYTE writeByteFunction,int pauseMode);

void SH2_Reset(SH2_State* cpu);

void SH2_Step(SH2_State* cpu);

void SH2_Interrupt(SH2_State* cpu,int level);

void SH2_SignalDAck(SH2_State* cpu);							/* Signals that data is ready - used by external dma mode */

#endif/*__SH2_H*/

