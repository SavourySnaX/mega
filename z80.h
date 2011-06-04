/*
 *  z80.h

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

#include "mytypes.h"

typedef struct 
{
	U8	R[8];				/* B,C,D,E,H,L,F,A */
	U8	R_[8];			/* B',C',D',E',H',L',F',A' */
	U16	PC;
	U16	SP;
	U16 IX;
	U16 IY;
	U16 IR;					/* I interrupt vector  -  R memory refresh */
	
	U8	IFF1,IFF2;			/* either 0 or 1 */

/* Hidden registers used by emulation */

	U8	InterruptMode;

	U16	operands[8];
	U16	opcode;

	U32	ixAdjust;
	U32 iyAdjust;

	U32	ixDisAdjust;
	U32 iyDisAdjust;

	U32	stage;
	
	U32 resetLine;
	U32	stopped;
	
	U32 delayedInterruptEnable;

	U32	lastInstruction;		/* Used by debugger (since sub stage instruction emulation shifts PC as it goes) */
}Z80_Regs;

typedef U16 (*Z80_Decode)(U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
typedef U32 (*Z80_Function)(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);

typedef struct
{
	char baseTable[9];
	char opcodeName[32];
	Z80_Function opcode;
	Z80_Decode   decode;
	int numOperands;
	U16   operandMask[8];
	U16   operandShift[8];
	int numValidMasks[8];
	char validEffectiveAddress[8][64][10];
} Z80_Ins;

extern Z80_Ins		Z80_instructions[];
extern Z80_Function	Z80_JumpTable[256];				/* Going to need more tables for extended instruction blocks */
extern Z80_Decode	Z80_DisTable[256];
extern Z80_Ins		*Z80_Information[256];

#define Z80_REG_B		(0)
#define Z80_REG_C		(1)
#define Z80_REG_D		(2)
#define Z80_REG_E		(3)
#define Z80_REG_H		(4)
#define Z80_REG_L		(5)
#define Z80_REG_F		(6)
#define Z80_REG_A		(7)

#define Z80_STATUS_V		(1<<9)					/* These 2 are used to decide how we treat the PV flag in flag calculations */
#define Z80_STATUS_P		(1<<8)

#define	Z80_STATUS_S		(1<<7)
#define	Z80_STATUS_Z		(1<<6)
#define	Z80_STATUS_X_B5	(1<<5)					/* Unknown in official z80 document		(actually copy of bit 5 of operation) */
#define	Z80_STATUS_H		(1<<4)
#define	Z80_STATUS_X_B3	(1<<3)					/* Unknown in official z80 document		(actually copy of bit 3 of operation) */
#define	Z80_STATUS_PV		(1<<2)
#define	Z80_STATUS_N		(1<<1)
#define	Z80_STATUS_C		(1<<0)

extern Z80_Regs Z80_regs;

void Z80_BuildTable();

void Z80_Reset();

void Z80_Step();
int Z80_Cycle_Step();						/* Entry always assumed to be stage==0 */

void Z80_SignalInterrupt(S8 level);

void Z80_SaveState(FILE *outStream);
void Z80_LoadState(FILE *inStream);
