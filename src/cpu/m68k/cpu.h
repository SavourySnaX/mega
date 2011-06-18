/*
 *  cpu.h

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

#ifndef __CPU_H
#define __CPU_H

#include "mytypes.h"

typedef struct 
{
	U32	D[8];
	U32	A[8];
	U32	PC;
	U16	SR;		/* T1 T0 S M 0 I2 I1 I0 0 0 0 X N Z V C		(bit 15-0) NB low 8 = CCR */
	
	U32	USP,ISP;
	
/* Hidden registers used by emulation */
    U16	operands[8];
    U16	opcode;
	
	U32	stage;
	
	U32	stopped;
	
	U32	eas;
	U32	ead;
	U32	ear;
	U32	eat;
	
	U32	len;
	U32	iMask;
	U32	nMask;
	U32	zMask;
	
	U32	tmpL;
	U16	tmpW;

	U32	lastInstruction;		/* Used by debugger (since sub stage instruction emulation shifts PC as it goes) */
}CPU_Regs;

typedef U16 (*CPU_Decode)(U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
typedef U32 (*CPU_Function)(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);

typedef struct
{
	U8 baseTable[17];
	char opcodeName[32];
	CPU_Function opcode;
	CPU_Decode   decode;
	int numOperands;
	U16   operandMask[8];
	U16   operandShift[8];
	int numValidMasks[8];
	char validEffectiveAddress[8][64][16];
} CPU_Ins;

extern CPU_Ins		cpu_instructions[];
extern CPU_Function	CPU_JumpTable[65536];
extern CPU_Decode	CPU_DisTable[65536];
extern CPU_Ins		*CPU_Information[65536];

#define	CPU_STATUS_T1		(1<<15)
#define	CPU_STATUS_T0		(1<<14)
#define	CPU_STATUS_S		(1<<13)
#define	CPU_STATUS_M		(1<<12)
#define	CPU_STATUS_I2		(1<<10)
#define	CPU_STATUS_I1		(1<<9)
#define	CPU_STATUS_I0		(1<<8)
#define	CPU_STATUS_X		(1<<4)
#define	CPU_STATUS_N		(1<<3)
#define	CPU_STATUS_Z		(1<<2)
#define	CPU_STATUS_V		(1<<1)
#define	CPU_STATUS_C		(1<<0)

extern CPU_Regs cpu_regs;

void CPU_BuildTable();

void CPU_Reset();

void CPU_Step();

void CPU_SignalInterrupt(S8 level);

void CPU_SaveState(FILE *outStream);
void CPU_LoadState(FILE *inStream);

#endif/*__CPU_H*/

