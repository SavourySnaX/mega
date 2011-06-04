/*
 *  cpu.c

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

#include "cpu_dis.h"
#include "cpu_ops.h"
#include "cpu.h"
#include "memory.h"

CPU_Regs	cpu_regs;

U8	cpuUsedTable[65536];

int		startDebug=0;

void CPU_SaveState(FILE *outStream)
{
	fwrite(&cpu_regs,1,sizeof(CPU_Regs),outStream);
}

void CPU_LoadState(FILE *inStream)
{
	if (sizeof(CPU_Regs)!=fread(&cpu_regs,1,sizeof(CPU_Regs),inStream))
	{
		return;
	}
}

void CPU_Reset()
{
	int a;
	
	for (a=0;a<8;a++)
	{
		cpu_regs.D[a]=0;
		cpu_regs.A[a]=0;
	}
	cpu_regs.SR = CPU_STATUS_S | CPU_STATUS_I2 | CPU_STATUS_I1 | CPU_STATUS_I0;
	
	cpu_regs.A[7] = MEM_getLong(/*0xFC0000*/0);
	cpu_regs.PC = MEM_getLong(/*0xFC0004*/4);

	printf("A7 Reset %08x : %08x\n",MEM_getLong(0xFC0000),cpu_regs.A[7]);
	printf("PC Reset %08x : %08x\n",MEM_getLong(0xFC0004),cpu_regs.PC);

	for (a=0;a<65536;a++)
		cpuUsedTable[a]=0;
		
	cpu_regs.stage=0;
}

const char *decodeWord(U32 address)
{
	static char buffer[256];
	
	sprintf(buffer,"%04X",MEM_getWord(address));
	
	return buffer;
}

const char *decodeLong(U32 address)
{
	static char buffer[256];
	
	sprintf(buffer,"%04X %04X",MEM_getWord(address),MEM_getWord(address+2));
	
	return buffer;
}

/*///////////////////////////////////////////////////////////////////////*/


CPU_Ins cpu_instructions[] = 
{
/* Supervisor instructions */
{"0100111001110010","STOP",CPU_STOP,CPU_DIS_STOP,0,{0},{0},{0},{{""}}},
{"0100011011aaaaaa","MOVESR",CPU_MOVETOSR,CPU_DIS_MOVETOSR,1,{0x003F},{0},{11},{{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"0000001001111100","ANDSR",CPU_ANDSR,CPU_DIS_ANDSR,0,{0},{0},{0},{{""}}},
{"0100111001110011","RTE",CPU_RTE,CPU_DIS_RTE,0,{0},{0},{0},{{""}}},
{"0000000001111100","ORSR",CPU_ORSR,CPU_DIS_ORSR,0,{0},{0},{0},{{""}}},
{"010011100110mrrr","MOVEUSP",CPU_MOVEUSP,CPU_DIS_MOVEUSP,2,{0x0008,0x0007},{3,0},{1,1},{{"r"},{"rrr"}}},
{"0000101001111100","EORISR",CPU_EORISR,CPU_DIS_EORISR,0,{0},{0},{0},{{""}}},
{"0100111001110000","RESET",CPU_RESET,CPU_DIS_RESET,0,{0},{0},{0},{{""}}},
/* Illegal instructions - that programs use - need to retest the 2 below, because of change to process exception to push pc-2 and not pc for linea */
/*{"0100001011aaaaaa","MOVEfromCCR",CPU_ILLEGAL,CPU_DIS_ILLEGAL,1,{0x003F},{0},{8},{{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},	// ODYSSEY demo uses this*/
/*{"0100111001111011","ILLEGAL",CPU_ILLEGAL,CPU_DIS_ILLEGAL,0},		// KS does this one*/
{"1010aaaaaaaaaaaa","LINEA",CPU_LINEA,CPU_DIS_LINEA,1,{0x0FFF},{0},{1},{{"rrrrrrrrrrrr"}}},					/* Star cruiser megadrive */
/* User instructions */
{"1110011011aaaaaa","ROR",CPU_RORm,CPU_DIS_RORm,1,{0x003F},{0},{7},{{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1110011111aaaaaa","ROL",CPU_ROLm,CPU_DIS_ROLm,1,{0x003F},{0},{7},{{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1110010011aaaaaa","ROXR",CPU_ROXRm,CPU_DIS_ROXRm,1,{0x003F},{0},{7},{{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"0100111001110110","TRAPV",CPU_TRAPV,CPU_DIS_TRAPV,0,{0},{0},{0},{{""}}},
{"1110001111aaaaaa","LSL",CPU_LSLm,CPU_DIS_LSLm,1,{0x003F},{0},{7},{{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"0100111001110111","RTR",CPU_RTR,CPU_DIS_RTR,0,{0},{0},{0},{{""}}},
{"1000rrr100001aaa","SBCD",CPU_SBCDm,CPU_DIS_SBCDm,2,{0x0E00,0x0007},{9,0},{1,1},{{"rrr","rrr"}}},
{"0000ddd101001aaa","MOVEP",CPU_MOVEP_m_L,CPU_DIS_MOVEP_m_L,2,{0x0E00,0x0007},{9,0},{1,1},{{"rrr","rrr"}}},
{"1110010111aaaaaa","ROXL",CPU_ROXLm,CPU_DIS_ROXLm,1,{0x003F},{0},{7},{{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1100rrr100001aaa","ABCD",CPU_ABCDm,CPU_DIS_ABCDm,2,{0x0E00,0x0007},{9,0},{1,1},{{"rrr","rrr"}}},
{"0000ddd100001aaa","MOVEP",CPU_MOVEP_m_W,CPU_DIS_MOVEP_m_W,2,{0x0E00,0x0007},{9,0},{1,1},{{"rrr","rrr"}}},
{"0000ddd111001aaa","MOVEP",CPU_MOVEP_L_m,CPU_DIS_MOVEP_L_m,2,{0x0E00,0x0007},{9,0},{1,1},{{"rrr","rrr"}}},
{"0000ddd110001aaa","MOVEP",CPU_MOVEP_W_m,CPU_DIS_MOVEP_W_m,2,{0x0E00,0x0007},{9,0},{1,1},{{"rrr","rrr"}}},
{"1100rrr100000ddd","ABCD",CPU_ABCD,CPU_DIS_ABCD,2,{0x0E00,0x0007},{9,0},{1,1},{{"rrr","rrr"}}},
{"1000rrr100000ddd","SBCD",CPU_SBCD,CPU_DIS_SBCD,2,{0x0E00,0x0007},{9,0},{1,1},{{"rrr","rrr"}}},
{"01000000zzaaaaaa","NEGX",CPU_NEGX,CPU_DIS_NEGX,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"0000000000111100","ORCCR",CPU_ORICCR,CPU_DIS_ORICCR,0,{0},{0},{0},{{""}}},
{"0000001000111100","ANDCCR",CPU_ANDICCR,CPU_DIS_ANDICCR,0,{0},{0},{0},{{""}}},
{"1110000011aaaaaa","ASR",CPU_ASRm,CPU_DIS_ASRm,1,{0x003F},{0},{7},{{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1110000111aaaaaa","ASL",CPU_ASLm,CPU_DIS_ASLm,1,{0x003F},{0},{7},{{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
/* 1001rrr1zz001ddd  9100 -> 9FCF	SUBX */
{"1001rrr1zz000ddd","SUBX",CPU_SUBX,CPU_DIS_SUBX,3,{0x0E00,0x00C0,0x0007},{9,6,0},{1,3,1},{{"rrr"},{"00","01","10"},{"rrr"}}},
/*{"1101rrr1zz001ddd","ADDX",CPU_ADDXm,CPU_DIS_ADDXm,  D100 -> DFC0      ADDX*/
{"0100000011aaaaaa","MOVESR",CPU_MOVEFROMSR,CPU_DIS_MOVEFROMSR,1,{0x003F},{0},{8},{{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1000rrr111aaaaaa","DIVS",CPU_DIVS,CPU_DIS_DIVS,2,{0x0E00,0x003F},{9,0},{1,11},{{"rrr"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"1101rrr1zz000ddd","ADDX",CPU_ADDX,CPU_DIS_ADDX,3,{0x0E00,0x00C0,0x0007},{9,6,0},{1,3,1},{{"rrr"},{"00","01","10"},{"rrr"}}},
{"010011100100rrrr","TRAP",CPU_TRAP,CPU_DIS_TRAP,1,{0x000F},{0},{1},{{"rrrr"}}},
{"0100010011aaaaaa","MOVECCR",CPU_MOVETOCCR,CPU_DIS_MOVETOCCR,1,{0x003F},{0},{11},{{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"1110ccc0zzm10rrr","ROXR",CPU_ROXR,CPU_DIS_ROXR,4,{0x0E00,0x000C0,0x0020,0x0007},{9,6,5,0},{1,3,1,1},{{"rrr"},{"00","01","10"},{"r"},{"rrr"}}},
{"1110ccc1zzm10rrr","ROXL",CPU_ROXL,CPU_DIS_ROXL,4,{0x0E00,0x000C0,0x0020,0x0007},{9,6,5,0},{1,3,1,1},{{"rrr"},{"00","01","10"},{"r"},{"rrr"}}},
{"0000101000111100","EORICCR",CPU_EORICCR,CPU_DIS_EORICCR,0,{0},{0},{0},{{""}}},
{"00001010zzaaaaaa","EORI",CPU_EORI,CPU_DIS_EORI,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1110001011aaaaaa","LSR",CPU_LSRm,CPU_DIS_LSRm,1,{0x003F},{0},{7},{{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"0000100001aaaaaa","BCHGI",CPU_BCHGI,CPU_DIS_BCHGI,1,{0x003F},{0},{8},{{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"0000rrr101aaaaaa","BCHG",CPU_BCHG,CPU_DIS_BCHG,2,{0x0E00,0x003F},{9,0},{1,8},{{"rrr"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"0100111001110001","NOP",CPU_NOP,CPU_DIS_NOP,0,{0},{0},{0},{{""}}},
{"1110ccc0zzm11rrr","ROR",CPU_ROR,CPU_DIS_ROR,4,{0x0E00,0x000C0,0x0020,0x0007},{9,6,5,0},{1,3,1,1},{{"rrr"},{"00","01","10"},{"r"},{"rrr"}}},
{"1110ccc1zzm11rrr","ROL",CPU_ROL,CPU_DIS_ROL,4,{0x0E00,0x000C0,0x0020,0x0007},{9,6,5,0},{1,3,1,1},{{"rrr"},{"00","01","10"},{"r"},{"rrr"}}},
{"0000rrr100aaaaaa","BTST",CPU_BTST,CPU_DIS_BTST,2,{0x0E00,0x003F},{9,0},{1,11},{{"rrr"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"1011rrr1mmaaaaaa","EOR",CPU_EORd,CPU_DIS_EORd,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,8},{{"rrr"},{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"0000rrr110aaaaaa","BCLR",CPU_BCLR,CPU_DIS_BCLR,2,{0x0E00,0x003F},{9,0},{1,8},{{"rrr"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1000rrr011aaaaaa","DIVU",CPU_DIVU,CPU_DIS_DIVU,2,{0x0E00,0x003F},{9,0},{1,11},{{"rrr"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"1110ccc0zzm00rrr","ASR",CPU_ASR,CPU_DIS_ASR,4,{0x0E00,0x000C0,0x0020,0x0007},{9,6,5,0},{1,3,1,1},{{"rrr"},{"00","01","10"},{"r"},{"rrr"}}},
{"1110ccc1zzm00rrr","ASL",CPU_ASL,CPU_DIS_ASL,4,{0x0E00,0x000C0,0x0020,0x0007},{9,6,5,0},{1,3,1,1},{{"rrr"},{"00","01","10"},{"r"},{"rrr"}}},
{"00000000zzaaaaaa","ORI",CPU_ORI,CPU_DIS_ORI,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1100rrr1mmaaaaaa","AND",CPU_ANDd,CPU_DIS_ANDd,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,7},{{"rrr"},{"00","01","10"},{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1000rrr0mmaaaaaa","OR",CPU_ORs,CPU_DIS_ORs,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,11},{{"rrr"},{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"0100111001011rrr","UNLK",CPU_UNLK,CPU_DIS_UNLK,1,{0x0007},{0},{1},{{"rrr"}}},
{"1101rrr1mmaaaaaa","ADD",CPU_ADDd,CPU_DIS_ADDd,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,7},{{"rrr"},{"00","01","10"},{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1011rrr1zz001ddd","CMPM",CPU_CMPM,CPU_DIS_CMPM,3,{0x0E00,0x00C0,0x0007},{9,6,0},{1,3,1},{{"rrr"},{"00","01","10"},{"rrr"}}},
{"0100111001010rrr","LINK",CPU_LINK,CPU_DIS_LINK,1,{0x0007},{0},{1},{{"rrr"}}},
{"0100100001aaaaaa","PEA",CPU_PEA,CPU_DIS_PEA,1,{0x003F},{0},{7},{{"010rrr","101rrr","110rrr","111000","111001","111010","111011"}}},
{"0101cccc11aaaaaa","Scc",CPU_SCC,CPU_DIS_SCC,2,{0x0F00,0x003F},{8,0},{1,8},{{"rrrr"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"01000100zzaaaaaa","NEG",CPU_NEG,CPU_DIS_NEG,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1100rrr111aaaaaa","MULS",CPU_MULS,CPU_DIS_MULS,2,{0x0E00,0x003F},{9,0},{1,11},{{"rrr"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"01001000mm000rrr","EXT",CPU_EXT,CPU_DIS_EXT,2,{0x00C0,0x0007},{6,0},{2,1},{{"10","11"},{"rrr"}}},
{"00000110zzaaaaaa","ADDI",CPU_ADDI,CPU_DIS_ADDI,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1110ccc1zzm01rrr","LSL",CPU_LSL,CPU_DIS_LSL,4,{0x0E00,0x000C0,0x0020,0x0007},{9,6,5,0},{1,3,1,1},{{"rrr"},{"00","01","10"},{"r"},{"rrr"}}},
{"1100rrr011aaaaaa","MULU",CPU_MULU,CPU_DIS_MULU,2,{0x0E00,0x003F},{9,0},{1,11},{{"rrr"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"0000100011aaaaaa","BSETI",CPU_BSETI,CPU_DIS_BSETI,1,{0x003F},{0},{8},{{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"0000rrr111aaaaaa","BSET",CPU_BSET,CPU_DIS_BSET,2,{0x0E00,0x003F},{9,0},{1,8},{{"rrr"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1001rrr1mmaaaaaa","SUB",CPU_SUBd,CPU_DIS_SUBd,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,7},{{"rrr"},{"00","01","10"},{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1100rrr0mmaaaaaa","AND",CPU_ANDs,CPU_DIS_ANDs,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,11},{{"rrr"},{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"0000100010aaaaaa","BCLRI",CPU_BCLRI,CPU_DIS_BCLRI,1,{0x003F},{0},{8},{{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"0100111010aaaaaa","JSR",CPU_JSR,CPU_DIS_JSR,1,{0x003F},{0},{7},{{"010rrr","101rrr","110rrr","111000","111001","111010","111011"}}},
{"1100rrr1mmmmmddd","EXG",CPU_EXG,CPU_DIS_EXG,3,{0x0E00,0x00F8,0x0007},{9,3,0},{1,3,1},{{"rrr"},{"01000","01001","10001"},{"rrr"}}},
{"00000010zzaaaaaa","ANDI",CPU_ANDI,CPU_DIS_ANDI,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"01000010zzaaaaaa","CLR",CPU_CLR,CPU_DIS_CLR,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"0101ddd0zzaaaaaa","ADDQ",CPU_ADDQ,CPU_DIS_ADDQ,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,9},{{"rrr"},{"00","01","10"},{"000rrr","001!!!","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1000rrr1mmaaaaaa","OR",CPU_ORd,CPU_DIS_ORd,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,7},{{"rrr"},{"00","01","10"},{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"0100111001110101","RTS",CPU_RTS,CPU_DIS_RTS,0,{0},{0},{0},{{""}}},
{"01100001dddddddd","BSR",CPU_BSR,CPU_DIS_BSR,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
{"00000100zzaaaaaa","SUBI",CPU_SUBI,CPU_DIS_SUBI,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"010010001zaaaaaa","MOVEM",CPU_MOVEMd,CPU_DIS_MOVEMd,2,{0x0040,0x003F},{6,0},{1,6},{{"r"},{"010rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"010011001zaaaaaa","MOVEM",CPU_MOVEMs,CPU_DIS_MOVEMs,2,{0x0040,0x003F},{6,0},{1,8},{{"r"},{"010rrr","011rrr","101rrr","110rrr","111000","111001","111010","111011"}}},
{"0100100001000rrr","SWAP",CPU_SWAP,CPU_DIS_SWAP,1,{0x0007},{0},{1},{{"rrr"}}},
{"1110ccc0zzm01rrr","LSR",CPU_LSR,CPU_DIS_LSR,4,{0x0E00,0x000C0,0x0020,0x0007},{9,6,5,0},{1,3,1,1},{{"rrr"},{"00","01","10"},{"r"},{"rrr"}}},
{"1001rrr0mmaaaaaa","SUB",CPU_SUBs,CPU_DIS_SUBs,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,12},{{"rrr"},{"00","01","10"},{"000rrr","001!!!","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"0111rrr0dddddddd","MOVEQ",CPU_MOVEQ,CPU_DIS_MOVEQ,2,{0x0E00,0x00FF},{9,0},{1,1},{{"rrr"},{"rrrrrrrr"}}},
{"0100111011aaaaaa","JMP",CPU_JMP,CPU_DIS_JMP,1,{0x003F},{0},{7},{{"010rrr","101rrr","110rrr","111000","111001","111010","111011"}}},
{"01001010zzaaaaaa","TST",CPU_TST,CPU_DIS_TST,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1101rrrm11aaaaaa","ADDA",CPU_ADDA,CPU_DIS_ADDA,3,{0x0E00,0x0100,0x003F},{9,8,0},{1,1,12},{{"rrr"},{"r"},{"000rrr","001rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"1001rrrm11aaaaaa","SUBA",CPU_SUBA,CPU_DIS_SUBA,3,{0x0E00,0x0100,0x003F},{9,8,0},{1,1,12},{{"rrr"},{"r"},{"000rrr","001rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"01000110zzaaaaaa","NOT",CPU_NOT,CPU_DIS_NOT,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1101rrr0mmaaaaaa","ADD",CPU_ADDs,CPU_DIS_ADDs,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,12},{{"rrr"},{"00","01","10"},{"000rrr","001!!!","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"0000100000aaaaaa","BTSTI",CPU_BTSTI,CPU_DIS_BTSTI,1,{0x003F},{0},{10},{{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111010","111011"}}},
{"01100000dddddddd","BRA",CPU_BRA,CPU_DIS_BRA,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
{"0101cccc11001rrr","DBCC",CPU_DBCC,CPU_DIS_DBCC,2,{0x0F00,0x0007},{8,0},{1,1},{{"rrrr"},{"rrr"}}},
{"00zzddd001aaaaaa","MOVEA",CPU_MOVEA,CPU_DIS_MOVEA,3,{0x3000,0x0E00,0x003F},{12,9,0},{2,1,12},{{"11","10"},{"rrr"},{"000rrr","001rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"00001100zzaaaaaa","CMPI",CPU_CMPI,CPU_DIS_CMPI,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1011rrr0mmaaaaaa","CMP",CPU_CMP,CPU_DIS_CMP,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,12},{{"rrr"},{"00","01","10"},{"000rrr","001!!!","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"1011rrrm11aaaaaa","CMPA",CPU_CMPA,CPU_DIS_CMPA,3,{0x0E00,0x0100,0x003F},{9,8,0},{1,1,12},{{"rrr"},{"r"},{"000rrr","001rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"0110ccccdddddddd","BCC",CPU_BCC,CPU_DIS_BCC,2,{0x0F00,0x00FF},{8,0},{3,1},{{"rr1r","r1rr","1rrr"},{"rrrrrrrr"}}},
{"0101ddd1zzaaaaaa","SUBQ",CPU_SUBQ,CPU_DIS_SUBQ,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,9},{{"rrr"},{"00","01","10"},{"000rrr","001!!!","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"00zzaaaaaaAAAAAA","MOVE",CPU_MOVE,CPU_DIS_MOVE,3,{0x3000,0x0FC0,0x003F},{12,6,0},{3,8,12},{{"01","10","11"},{"rrr000","rrr010","rrr011","rrr100","rrr101","rrr110","000111","001111"},{"000rrr","001???","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"0100rrr111aaaaaa","LEA",CPU_LEA,CPU_DIS_LEA,2,{0x0E00,0x003F},{9,0},{1,7},{{"rrr"},{"010rrr","101rrr","110rrr","111000","111001","111010","111011"}}},
{"","",0,0,0,{0},{0},{0},{{""}}}
};

CPU_Function	CPU_JumpTable[65536];
CPU_Decode	CPU_DisTable[65536];
CPU_Ins		*CPU_Information[65536];

/*
/// 0100100001001vvv  4848 -> 484F	 BKPT
/// 0100rrrss0aaaaaa  4000 -> 4FBF	 CHK
/// 0100101011111100  4AFC -> 4AFC	 ILLEGAL
/// 0100100000aaaaaa  4800 -> 483F	 NBCD
/// 0100101011aaaaaa  4AC0 -> 4AFF	 TAS
/// 1111aaaaaaaaaaaa  F000 -> FFFF	 LINE F
*/
U32 CPU_UNKNOWN(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	cpu_regs.PC-=2;	/* account for prefetch */
	printf("ILLEGAL INSTRUCTION %08x\n",cpu_regs.PC);
	DEB_PauseEmulation(DEB_Mode_68000,"ILLEGAL INSTRUCTION");
	return 0;
}

const char *byte_to_binary(U32 x)
{
    U32 z;
    static char b[17] = {0};
	
    b[0]=0;
    for (z = 32768; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }
	
    return b;
}

U8 ValidateOpcode(int insNum,U16 opcode)
{
    U8 invalidMask=0;
    int a,b,c;
    char *mask;
    int operandNum=0;
    U8 lastBase=0;
	
    for (a=0;a<16;a++)
    {
		if (cpu_instructions[insNum].baseTable[a]!=lastBase && cpu_instructions[insNum].baseTable[a]!='0' && cpu_instructions[insNum].baseTable[a]!='1')
		{
			U16 operand = (opcode & cpu_instructions[insNum].operandMask[operandNum]);
			operand >>= cpu_instructions[insNum].operandShift[operandNum];
			lastBase = cpu_instructions[insNum].baseTable[a];
			
			for (b=0;b<cpu_instructions[insNum].numValidMasks[operandNum];b++)
			{
				invalidMask = 0;
				mask = cpu_instructions[insNum].validEffectiveAddress[operandNum][b];
				while (*mask!=0)
				{
					c=strlen(mask)-1;
					switch (*mask)
					{
						case '0':
							if ((operand & (1<<c)) != 0)
								invalidMask=1;
							break;
						case '1':
							if ((operand & (1<<c)) == 0)
								invalidMask=1;
							break;
						case 'r':
							break;
						case '?':
							if ( (opcode & 0x3000) == 0x1000)
								invalidMask=1;			/* special case - address register direct not supported for byte size operations */
							break;
						case '!':
							if ( (opcode & 0x00C0) == 0x0000)
								invalidMask=1;			/* special case - address register direct not supported for byte size operations */
					}
					mask++;
				}
				if (!invalidMask)
					break;
			}
			if (invalidMask)
				return 0;
			operandNum++;
		}
    }
	
    return 1;
}

void CPU_BuildTable()
{
	int a,b,c,d;
	int numInstructions=0;
	
	for (a=0;a<65536;a++)
	{
		CPU_JumpTable[a]=CPU_UNKNOWN;
		CPU_DisTable[a]=CPU_DIS_UNKNOWN;
		CPU_Information[a]=0;
	}
	
	a=0;
	while (cpu_instructions[a].opcode)
	{
	    int modifiableBitCount=0;
		
	    /* precount modifiable bits */
	    for (b=0;b<16;b++)
	    {
			switch (cpu_instructions[a].baseTable[b])
			{
				case '0':
				case '1':
					/* Fixed code */
					break;
				default:
					modifiableBitCount++;
			}
	    }
	    modifiableBitCount=1<<modifiableBitCount;
		
	    b=0;
	    while (1)
	    {
			U8 needValidation=0;
			U8 validOpcode=1;
			U16 opcode=0;
			
			d=0;
			/* Create instruction code */
			for (c=0;c<16;c++)
			{
				switch (cpu_instructions[a].baseTable[c])
				{
					case '0':
						break;
					case '1':
						opcode|=1<<(15-c);
						break;
					case 'r':
					case 'd':
					case 'a':
					case 'A':
					case 'z':
					case 'm':
					case 'c':
						opcode|=((b&(1<<d))>>d)<<(15-c);
						d++;
						needValidation=1;
						break;
				}
			}
			if (needValidation)
			{
				if (!ValidateOpcode(a,opcode))
				{
					validOpcode=0;
				}
			}
			if (validOpcode)
			{
/*
				printf("Opcode Coding : %s : %04X %s\n", cpu_instructions[a].opcodeName, opcode,byte_to_binary(opcode));
*/
				if (CPU_JumpTable[opcode]!=CPU_UNKNOWN)
				{
					printf("[ERR] Cpu Coding For Instruction Overlap\n");
					exit(-1);
				}
				CPU_JumpTable[opcode]=cpu_instructions[a].opcode;
				CPU_DisTable[opcode]=cpu_instructions[a].decode;
				CPU_Information[opcode]=&cpu_instructions[a];
				numInstructions++;
			}
			b++;
			if (b>=modifiableBitCount)
				break;
	    }
		
	    a++;
	}

	printf("68000 %d out of %d instructions\n",numInstructions,65536);
}

void DumpEmulatorState()
{
    printf("\n");
    printf("D0=%08X\tD1=%08X\tD2=%08x\tD3=%08x\n",cpu_regs.D[0],cpu_regs.D[1],cpu_regs.D[2],cpu_regs.D[3]);
    printf("D4=%08X\tD5=%08X\tD6=%08x\tD7=%08x\n",cpu_regs.D[4],cpu_regs.D[5],cpu_regs.D[6],cpu_regs.D[7]);
    printf("A0=%08X\tA1=%08X\tA2=%08x\tA3=%08x\n",cpu_regs.A[0],cpu_regs.A[1],cpu_regs.A[2],cpu_regs.A[3]);
    printf("A4=%08X\tA5=%08X\tA6=%08x\tA7=%08x\n",cpu_regs.A[4],cpu_regs.A[5],cpu_regs.A[6],cpu_regs.A[7]);
    printf("USP=%08X,ISP=%08x\n",cpu_regs.USP,cpu_regs.ISP);
    printf("\n");
    printf("          [ T1:T0: S: M:  :I2:I1:I0:  :  :  : X: N: Z: V: C ]\n");
    printf("SR = %04X [ %s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s ]\n", cpu_regs.SR, 
		   cpu_regs.SR & 0x8000 ? " 1" : " 0",
		   cpu_regs.SR & 0x4000 ? " 1" : " 0",
		   cpu_regs.SR & 0x2000 ? " 1" : " 0",
		   cpu_regs.SR & 0x1000 ? " 1" : " 0",
		   cpu_regs.SR & 0x0800 ? " 1" : " 0",
		   cpu_regs.SR & 0x0400 ? " 1" : " 0",
		   cpu_regs.SR & 0x0200 ? " 1" : " 0",
		   cpu_regs.SR & 0x0100 ? " 1" : " 0",
		   cpu_regs.SR & 0x0080 ? " 1" : " 0",
		   cpu_regs.SR & 0x0040 ? " 1" : " 0",
		   cpu_regs.SR & 0x0020 ? " 1" : " 0",
		   cpu_regs.SR & 0x0010 ? " 1" : " 0",
		   cpu_regs.SR & 0x0008 ? " 1" : " 0",
		   cpu_regs.SR & 0x0004 ? " 1" : " 0",
		   cpu_regs.SR & 0x0002 ? " 1" : " 0",
		   cpu_regs.SR & 0x0001 ? " 1" : " 0");
    printf("\n");
}

/* HACK */
extern U8  VDP_Registers[0x20];

S8 CPU_signal = -1;

void CPU_SignalInterrupt(S8 level)
{
	if (level > CPU_signal)
	{
		CPU_signal=level;
	}
}

void CPU_CheckForInterrupt()
{
	/* Level 6 */
	/* Frame interrupt */
	if ((cpu_regs.SR & 0x0700) >= 0x0600)
		return;

	if ((VDP_Registers[1]&0x20) && CPU_signal==6)	/* Can't do EXACT check here due to instruction */
	{						/* lengths, so move external and flag! */
		CPU_GENERATE_EXCEPTION(0x78);
		cpu_regs.SR&=0xF8FF;
		cpu_regs.SR|=0x0600;
		CPU_signal=-1;
		cpu_regs.stopped=0;
		return;
	}

	/* Level 4 */
	/* Line interrupt */
	if ((cpu_regs.SR & 0x0700) >= 0x0400)
		return;

	if ((VDP_Registers[0]&0x10) && CPU_signal==4)
	{
		CPU_GENERATE_EXCEPTION(0x70);
		cpu_regs.SR&=0xF8FF;
		cpu_regs.SR|=0x0400;
		CPU_signal=-1;
		cpu_regs.stopped=0;
		return;
	}
}

void CPU_Step()
{
	static int cycles=0;
    int a;

	if (!cpu_regs.stage)
	{
		CPU_CheckForInterrupt();

		if (cpu_regs.stopped)
			return;

		cpu_regs.lastInstruction = cpu_regs.PC;
		
		/* Fetch next instruction */
		cpu_regs.opcode = MEM_getWord(cpu_regs.PC);
		cpu_regs.PC+=2;

		if (CPU_Information[cpu_regs.opcode])
		{
			for (a=0;a<CPU_Information[cpu_regs.opcode]->numOperands;a++)
			{
				cpu_regs.operands[a] = (cpu_regs.opcode & CPU_Information[cpu_regs.opcode]->operandMask[a]) >> 
										CPU_Information[cpu_regs.opcode]->operandShift[a];
			}
		}
		
		if (startDebug)
		{
			U32	insCount;
			U32 b;
			
			printf("Cycles %d\n",cycles);
			DumpEmulatorState();
			
			insCount=CPU_DisTable[cpu_regs.opcode](cpu_regs.PC,cpu_regs.operands[0],cpu_regs.operands[1],cpu_regs.operands[2],
						cpu_regs.operands[3],cpu_regs.operands[4],cpu_regs.operands[5],cpu_regs.operands[6],cpu_regs.operands[7]);
			
			printf("%08X\t",cpu_regs.PC-2);
			
			for (b=0;b<(insCount+2)/2;b++)
			{
				printf("%s ",decodeWord(cpu_regs.PC-2+b*2));
			}
			
			printf("\t%s\n",mnemonicData);
			
			startDebug=1;	/* just so i can break after disassembly. */
		}
		cycles=0;
		
	}

	cpu_regs.stage = CPU_JumpTable[cpu_regs.opcode](cpu_regs.stage,cpu_regs.operands[0],cpu_regs.operands[1],cpu_regs.operands[2],
				cpu_regs.operands[3],cpu_regs.operands[4],cpu_regs.operands[5],cpu_regs.operands[6],cpu_regs.operands[7]);
				
	cycles++;
}

