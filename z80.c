/*
 *  z80.c

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

#include "z80_dis.h"
#include "z80_ops.h"
#include "z80.h"
#include "memory.h"

U32 Z80Cycles;

Z80_Regs Z80_regs;

void Z80_SaveState(FILE *outStream)
{
	fwrite(&Z80_regs,1,sizeof(Z80_Regs),outStream);
}

void Z80_LoadState(FILE *inStream)
{
	if (sizeof(Z80_Regs)!=fread(&Z80_regs,1,sizeof(Z80_Regs),inStream))
	{
		return;
	}
}

/*----------------------------------*/

void IncrementRefreshRegister()
{
	U16 temp = Z80_regs.IR;

	temp++;
	temp&=0x007F;
	Z80_regs.IR&=0xFF80;
	Z80_regs.IR|=temp;
}

Z80_Ins Z80_CB_instructions[] =
{
	{"00111110","SRLm",Z80_CB_SRLm,Z80_DIS_CB_SRLm,0,{0},{0},{0},{{""}}},																																	/* M 4 T 15 (4,4,4,3) */
	{"00001110","RRCm",Z80_CB_RRCm,Z80_DIS_CB_RRCm,0,{0},{0},{0},{{""}}},																																	/* M 4 T 15 (4,4,4,3) */
	{"00100110","SLAm",Z80_CB_SLAm,Z80_DIS_CB_SLAm,0,{0},{0},{0},{{""}}},																																	/* M 4 T 15 (4,4,4,3) */
	{"00011110","RRm",Z80_CB_RRm,Z80_DIS_CB_RRm,0,{0},{0},{0},{{""}}},																																	/* M 4 T 15 (4,4,4,3) */
	{"00000110","RLCm",Z80_CB_RLCm,Z80_DIS_CB_RLCm,0,{0},{0},{0},{{""}}},																																	/* M 4 T 15 (4,4,4,3) */
	{"00101rrr","SRA",Z80_CB_SRA,Z80_DIS_CB_SRA,1,{0x07},{0},{7},{{"111","000","001","010","011","100","101"}}},		/* M 2 T 8 (4,4) */
	{"00010110","RLm",Z80_CB_RLm,Z80_DIS_CB_RLm,0,{0},{0},{0},{{""}}},																																	/* M 4 T 15 (4,4,4,3) */
	{"00011rrr","RR",Z80_CB_RR,Z80_DIS_CB_RR,1,{0x07},{0},{7},{{"111","000","001","010","011","100","101"}}},				/* M 2 T 8 (4,4) */
	{"00001rrr","RRC",Z80_CB_RRC,Z80_DIS_CB_RRC,1,{0x07},{0},{7},{{"111","000","001","010","011","100","101"}}},		/* M 2 T 8 (4,4) */ 
	{"00010rrr","RL",Z80_CB_RL,Z80_DIS_CB_RL,1,{0x07},{0},{7},{{"111","000","001","010","011","100","101"}}},				/* M 2 T 8 (4,4) */
	{"11bbbrrr","SET",Z80_CB_SET,Z80_DIS_CB_SET,2,{0x38,0x07},{3,0},{1,7},{{"rrr"},{"111","000","001","010","011","100","101"}}},	/* M 2 T 8 (4,4) */
	{"10bbbrrr","RES",Z80_CB_RES,Z80_DIS_CB_RES,2,{0x38,0x07},{3,0},{1,7},{{"rrr"},{"111","000","001","010","011","100","101"}}},	/* M 2 T 8 (4,4) */
	{"00000rrr","RLC",Z80_CB_RLC,Z80_DIS_CB_RLC,1,{0x07},{0},{7},{{"111","000","001","010","011","100","101"}}},		/* M 2 T 8 (4,4) */
	{"10bbb110","RESm",Z80_CB_RESm,Z80_DIS_CB_RESm,1,{0x38},{3},{1},{{"rrr"}}},																			/* M 4 T 12 (4,4,4,3) */
	{"11bbb110","SETm",Z80_CB_SETm,Z80_DIS_CB_SETm,1,{0x38},{3},{1},{{"rrr"}}},																			/* M 4 T 12 (4,4,4,3) */
	{"00111rrr","SRL",Z80_CB_SRL,Z80_DIS_CB_SRL,1,{0x07},{0},{7},{{"111","000","001","010","011","100","101"}}},		/* M 2 T 8 (4,4) */
	{"01bbbrrr","BIT",Z80_CB_BIT,Z80_DIS_CB_BIT,2,{0x38,0x07},{3,0},{1,7},{{"rrr"},{"111","000","001","010","011","100","101"}}},	/* M 2 T 8 (4,4) */
	{"00100rrr","SLA",Z80_CB_SLA,Z80_DIS_CB_SLA,1,{0x07},{0},{7},{{"111","000","001","010","011","100","101"}}},		/* M 2 T 8 (4,4) */
	{"01bbb110","BITm",Z80_CB_BITm,Z80_DIS_CB_BITm,1,{0x38},{3},{1},{{"rrr"}}},																			/* M 3 T 12 (4,4,4) */
	{"","",0,0,0,{0},{0},{0},{{""}}}
};

#if TRACE_OPCODES
U8	opcodesSeenBefore[3*256];
U8	opcodesSeenAfter[3*256];
U8 *opcodesSeen=opcodesSeenBefore;
#endif

/*NB HL never swapped for IX/IY */
Z80_Ins Z80_ED_instructions[] =
{
	{"01000101","RETN",Z80_ED_RETN,Z80_DIS_ED_RETN,0,{0},{0},{0},{{""}}},																										/* M 4 T 14 (4,4,3,3) */
	{"10100001","CPI",Z80_ED_CPI,Z80_DIS_ED_CPI,0,{0},{0},{0},{{""}}},																												/* M 4 T 16 (4,4,3,5) */
	{"10110011","OTIR",Z80_ED_OTIR,Z80_DIS_ED_OTIR,0,{0},{0},{0},{{""}}},																										/* M 5 T 21 (4,4,4,3,5)				when BC!=0 */
																																																				/* M 4 T 16 (4,3,3,5)					 when BC==0	(ie termination of loop) */
	{"10100011","OUTI",Z80_ED_OUTI,Z80_DIS_ED_OUTI,0,{0},{0},{0},{{""}}},																										/* M 4 T 16 (4,5,3,4) */
	{"01100111","RRD",Z80_ED_RRD,Z80_DIS_ED_RRD,0,{0},{0},{0},{{""}}},																												/* M 5 T 18 (4,4,3,4,3) */
	{"10100010","INI",Z80_ED_INI,Z80_DIS_ED_INI,0,{0},{0},{0},{{""}}},																												/* M 4 T 16 (4,5,3,4) */
	{"10111011","OTDR",Z80_ED_OTDR,Z80_DIS_ED_OTDR,0,{0},{0},{0},{{""}}},																										/* M 5 T 21 (4,4,4,3,5)				when BC!=0 */
																																																				/* M 4 T 16 (4,3,3,5)					when BC==0	(ie termination of loop) */
	{"10101000","LDD",Z80_ED_LDD,Z80_DIS_ED_LDD,0,{0},{0},{0},{{""}}},																												/* M 4 T 16 (4,3,3,5) */
	{"01010111","LDAI",Z80_ED_LDA_I,Z80_DIS_ED_LDA_I,0,{0},{0},{0},{{""}}},																									/* M 2 T 9 (4,5) */
	{"10101011","OUTD",Z80_ED_OUTD,Z80_DIS_ED_OUTD,0,{0},{0},{0},{{""}}},																										/* M 4 T 16 (4,5,3,4) */
	{"01001101","RETI",Z80_ED_RETI,Z80_DIS_ED_RETI,0,{0},{0},{0},{{""}}},																										/* M 4 T 14 (4,4,3,3) */
	{"01rrr001","OUTC_r",Z80_ED_OUTC_r,Z80_DIS_ED_OUTC_r,1,{0x38},{3},{7},{{"rrr"}}},											/* M 3 T 12 (4,4,4)					NB 110 is F (but special case outputs 0) */
	{"10110001","CPIR",Z80_ED_CPIR,Z80_DIS_ED_CPIR,0,{0},{0},{0},{{""}}},																										/* M 5 T 21 (4,4,4,3,5)				when BC!=0 */
																																																				/* M 4 T 16 (4,3,3,5)					when BC==0	(ie termination of loop) */
	{"01011111","LDAR",Z80_ED_LDA_R,Z80_DIS_ED_LDA_R,0,{0},{0},{0},{{""}}},																									/* M 2 T 9 (4,5) */
	{"01101111","RLD",Z80_ED_RLD,Z80_DIS_ED_RLD,0,{0},{0},{0},{{""}}},																												/* M 5 T 18 (4,4,3,4,3) */
	{"10100000","LDI",Z80_ED_LDI,Z80_DIS_ED_LDI,0,{0},{0},{0},{{""}}},																												/* M 4 T 16 (4,3,3,5) */
	{"01000100","NEG",Z80_ED_NEG,Z80_DIS_ED_NEG,0,{0},{0},{0},{{""}}},																												/* M 2 T 8 (4,4) */
	{"01dd1010","ADCHL",Z80_ED_ADCHL,Z80_DIS_ED_ADCHL,1,{0x30},{4},{1},{{"dd"}}},													/* M 4 T 15 (4,4,4,3) */
	{"01rrr000","INr_C",Z80_ED_INr_C,Z80_DIS_ED_INr_C,1,{0x38},{3},{7},{{"rrr"}}},												/* M 3 T 12 (4,4,4)					NB 110 is F (but special case for not updating a register) */
	{"01dd1011","LDdd_nn",Z80_ED_LDdd_nn,Z80_DIS_ED_LDdd_nn,1,{0x30},{4},{1},{{"dd"}}},										/* M 6 T 20 (4,4,3,3,3,3) */
	{"10111000","LDDR",Z80_ED_LDDR,Z80_DIS_ED_LDDR,0,{0},{0},{0},{{""}}},																										/* M 5 T 21 (4,4,4,3,5)				when BC!=0 */
																																																				/* M 4 T 16 (4,3,3,5)					when BC==0	(ie termination of loop) */
	{"01dd0011","LDnn_dd",Z80_ED_LDnn_dd,Z80_DIS_ED_LDnn_dd,1,{0x30},{4},{1},{{"dd"}}},										/* M 6 T 20 (4,4,3,3,3,3) */
	{"01dd0010","SBCHL",Z80_ED_SBCHL,Z80_DIS_ED_SBCHL,1,{0x30},{4},{1},{{"dd"}}},													/* M 4 T 15 (4,4,4,3) */
	{"01001111","LDRA",Z80_ED_LDR_A,Z80_DIS_ED_LDR_A,0,{0},{0},{0},{{""}}},																									/* M 2 T 9 (4,5) */
	{"01000111","LDIA",Z80_ED_LDI_A,Z80_DIS_ED_LDI_A,0,{0},{0},{0},{{""}}},																									/* M 2 T 9 (4,5) */
	{"10110000","LDIR",Z80_ED_LDIR,Z80_DIS_ED_LDIR,0,{0},{0},{0},{{""}}},																										/* M 5 T 21 (4,4,4,3,5,5)			when BC!=0 */
																																																				/* M 4 T 16 (4,3,3,5)					when BC==0	(ie termination of loop) */
	{"010mm110","IM",Z80_ED_IM,Z80_DIS_ED_IM,1,{0x18},{3},{3},{{"00","10","11"}}},												/* M 2 T 8 (4,4) */
	{"","",0,0,0,{0},{0},{0},{{""}}}
};

Z80_Ins Z80_instructions[] = 
{
	{"10011110","SBCm",Z80_SBCm,Z80_DIS_SBCm,0,{0},{0},{0},{{""}}},															/* M 2 T 7 (4,3) */
	{"11011110","SBCA_n",Z80_SBCA_n,Z80_DIS_SBCA_n,0,{0},{0},{0},{{""}}},												/* M 2 T 7 (4,3) */
	{"00001010","LDA_BC",Z80_LDA_BC,Z80_DIS_LDA_BC,0,{0},{0},{0},{{""}}},												/* M 2 T 7 (4,3) */
	{"00100111","DAA",Z80_DAA,Z80_DIS_DAA,0,{0},{0},{0},{{""}}},																	/* M 1 T 4 (4,3) */
	{"00000010","LDBC_A",Z80_LDBC_A,Z80_DIS_LDBC_A,0,{0},{0},{0},{{""}}},												/* M 2 T 7 (4,3) */
	{"10001110","ADCm",Z80_ADCm,Z80_DIS_ADCm,0,{0},{0},{0},{{""}}},															/* M 2 T 7 (4,3) */
	{"10110110","ORm",Z80_ORm,Z80_DIS_ORm,0,{0},{0},{0},{{""}}},																	/* M 2 T 7 (4,3) */
	{"10100110","ANDm",Z80_ANDm,Z80_DIS_ANDm,0,{0},{0},{0},{{""}}},															/* M 2 T 7 (4,3) */
	{"11011011","INA_n",Z80_INA_n,Z80_DIS_INA_n,0,{0},{0},{0},{{""}}},														/* M 2 T 7 (4,3) */
	{"10010110","SUBm",Z80_SUBm,Z80_DIS_SUBm,0,{0},{0},{0},{{""}}},															/* M 2 T 7 (4,3) */
	{"10011rrr","SBCA",Z80_SBCA_r,Z80_DIS_SBCA_r,1,{0x07},{0},{7},{{"111","000","001","010","011","100","101"}}},				/* M 1 T 4 */
	{"11ccc100","CALLcc",Z80_CALLcc,Z80_DIS_CALLcc,1,{0x38},{3},{1},{{"rrr"}}},			/* M 5 T 17 (4,3,4,3,3)		if cc is true */
																																									/* M 3 T 10 (4,3,3)				if cc is false */
	{"01110110","HALT",Z80_HALT,Z80_DIS_HALT,0,{0},{0},{0},{{""}}},															/* M 1 T 4 */
	{"11101110","XORA_n",Z80_XORA_n,Z80_DIS_XORA_n,0,{0},{0},{0},{{""}}},												/* M 2 T 7 (4,3) */
	{"00101111","CPL",Z80_CPL,Z80_DIS_CPL,0,{0},{0},{0},{{""}}},																	/* M 1 T 4 */
	{"10001rrr","ADCA",Z80_ADCA_r,Z80_DIS_ADCA_r,1,{0x07},{0},{7},{{"111","000","001","010","011","100","101"}}},				/* M 1 T 4 */
	{"11100011","EXSPHL",Z80_EXSP_HL,Z80_DIS_EXSP_HL,0,{0},{0},{0},{{""}}},											/* M 5 T 19 (4,3,4,3,5) */
	{"10000110","ADDm",Z80_ADDm,Z80_DIS_ADDm,0,{0},{0},{0},{{""}}},															/* M 2 T 7 (4,3) */
	{"00111111","CCF",Z80_CCF,Z80_DIS_CCF,0,{0},{0},{0},{{""}}},																	/* M 1 T 4 */
	{"10101110","XORm",Z80_XORm,Z80_DIS_XORm,0,{0},{0},{0},{{""}}},															/* M 2 T 7 (4,3) */
	{"00110111","SCF",Z80_SCF,Z80_DIS_SCF,0,{0},{0},{0},{{""}}},																	/* M 1 T 4 */
	{"11ccc000","RETcc",Z80_RETcc,Z80_DIS_RETcc,1,{0x38},{3},{1},{{"rrr"}}},	/* M 1 T 5						if condition false */
																																						/* M 3 T 11 (5,3,3)		if condition true */
	{"10010rrr","SUBA",Z80_SUBA_r,Z80_DIS_SUBA_r,1,{0x07},{0},{7},{{"111","000","001","010","011","100","101"}}},				/* M 1 T 4 */
	{"00101010","LDHL_nn",Z80_LDHL_nn,Z80_DIS_LDHL_nn,0,{0},{0},{0},{{""}}},											/* M 5 T 16 (4,3,3,3,3) */
	{"11101011","EXDE_HL",Z80_EXDE_HL,Z80_DIS_EXDE_HL,0,{0},{0},{0},{{""}}},											/* M 1 T 4 */
	{"00100010","LDnn_HL",Z80_LDnn_HL,Z80_DIS_LDnn_HL,0,{0},{0},{0},{{""}}},											/* M 5 T 16 (4,3,3,3,3) */
	{"00110101","DECHL",Z80_DECHL,Z80_DIS_DECHL,0,{0},{0},{0},{{""}}},														/* M 3 T 11 (4,3,3) */
	{"10100rrr","AND",Z80_AND,Z80_DIS_AND,1,{0x07},{0},{7},{{"111","000","001","010","011","100","101"}}},				/* M 1 T 4 */
	{"11010011","OUTn_A",Z80_OUTn_A,Z80_DIS_OUTn_A,0,{0},{0},{0},{{""}}},												/* M 3 T 11 (4,3,4) */
	{"00010010","LDDE_A",Z80_LDDE_A,Z80_DIS_LDDE_A,0,{0},{0},{0},{{""}}},												/* M 2 T 7 (4,3) */
	{"11qqq111","RST",Z80_RST,Z80_DIS_RST,1,{0x38},{3},{1},{{"ddd"}}},				/* M 3 T 11 (5,3,3) */
	{"10000rrr","ADDA",Z80_ADDA_r,Z80_DIS_ADDA_r,1,{0x07},{0},{7},{{"111","000","001","010","011","100","101"}}},				/* M 1 T 4 */
	{"11111011","EI",Z80_EI,Z80_DIS_EI,0,{0},{0},{0},{{""}}},																		/* M 1 T 4 */
	{"00011010","LDA_DE",Z80_LDA_DE,Z80_DIS_LDA_DE,0,{0},{0},{0},{{""}}},												/* M 2 T 7 (4,3) */
	{"11qq0101","PUSHRP",Z80_PUSHRP,Z80_DIS_PUSHRP,1,{0x30},{4},{1},{{"dd"}}},/* M 3 T 11 (5,3,3) */
	{"001cc000","JRcc",Z80_JRcc,Z80_DIS_JRcc,1,{0x18},{3},{1},{{"dd"}}},			/* M 3 T 12 (4,3,5)				Branch taken */
																																						/* M 2 T 7 (4,3)					Branch not taken */
	{"11111110","CP_n",Z80_CP_n,Z80_DIS_CP_n,0,{0},{0},{0},{{""}}},															/* M 2 T 7 (4,3) */
	{"11010110","SUBA_n",Z80_SUBA_n,Z80_DIS_SUBA_n,0,{0},{0},{0},{{""}}},												/* M 2 T 7 (4,3) */
	{"00011000","JR",Z80_JR,Z80_DIS_JR,0,{0},{0},{0},{{""}}},																		/* M 3 T 12 (4,3,5) */
	{"00110100","INCHL",Z80_INCHL,Z80_DIS_INCHL,0,{0},{0},{0},{{""}}},														/* M 3 T 11 (4,3,3) */
	{"10111110","CPHL",Z80_CPHL,Z80_DIS_CPHL,0,{0},{0},{0},{{""}}},															/* M 2 T 7 (4,3) */
	{"00010000","DJNZ",Z80_DJNZ,Z80_DIS_DJNZ,0,{0},{0},{0},{{""}}},															/* M 3 T 13 (5,3,5)					B!=0 */
																																						/* M 2 T 8  (5,3)						B==0 */
	{"00dd1001","ADDHL",Z80_ADDHL,Z80_DIS_ADDHL,1,{0x30},{4},{1},{{"dd"}}},		/* M 3 T 11 (4,4,3) */
	{"11100110","ANDA_n",Z80_ANDA_n,Z80_DIS_ANDA_n,0,{0},{0},{0},{{""}}},												/* M 2 T 7 (4,3) */
	{"00010111","RLA",Z80_RLA,Z80_DIS_RLA,0,{0},{0},{0},{{""}}},																	/* M 1 T 4 */
	{"00011111","RRA",Z80_RRA,Z80_DIS_RRA,0,{0},{0},{0},{{""}}},																	/* M 1 T 4 */
	{"10111rrr","CP",Z80_CP,Z80_DIS_CP,1,{0x07},{0},{7},{{"111","000","001","010","011","100","101"}}},				/* M 1 T 4 */
	{"11101001","JPHL",Z80_JPHL,Z80_DIS_JPHL,0,{0},{0},{0},{{""}}},															/* M 1 T 4 */
	{"00110110","LDHLn",Z80_LDHL_n,Z80_DIS_LDHL_n,0,{0},{0},{0},{{""}}},													/* M 3 T 10 (4,3,3) */
	{"11011001","EXX",Z80_EXX,Z80_DIS_EXX,0,{0},{0},{0},{{""}}},																	/* M 1 T 4 */
	{"00001000","EXAF",Z80_EXAF,Z80_DIS_EXAF,0,{0},{0},{0},{{""}}},															/* M 1 T 4 */
	{"11qq0001","POPRP",Z80_POPRP,Z80_DIS_POPRP,1,{0x30},{4},{1},{{"dd"}}},		/* M 3 T 10 (4,3,3) */
	{"11111101","FD TABLE",Z80_FD_TABLE,Z80_DIS_FD_TABLE,0,{0},{0},{0},{{""}}},									/* see FD table */
	{"11011101","DD TABLE",Z80_DD_TABLE,Z80_DIS_DD_TABLE,0,{0},{0},{0},{{""}}},									/* see DD table */
	{"11111001","LDSPHL",Z80_LDSP_HL,Z80_DIS_LDSP_HL,0,{0},{0},{0},{{""}}},											/* M 1 T 6 */
	{"00000000","NOP",Z80_NOP,Z80_DIS_NOP,0,{0},{0},{0},{{""}}},																	/* M 1 T 4 */
	{"00dd1011","DECRP",Z80_DECRP,Z80_DIS_DECRP,1,{0x30},{4},{1},{{"dd"}}},		/* M 1 T 6 */
	{"00dd0011","INCRP",Z80_INCRP,Z80_DIS_INCRP,1,{0x30},{4},{1},{{"dd"}}},		/* M 1 T 6 */
	{"11001110","ADCA_n",Z80_ADCA_n,Z80_DIS_ADCA_n,0,{0},{0},{0},{{""}}},												/* M 2 T 7 (4,3) */
	{"11000110","ADDA_n",Z80_ADDA_n,Z80_DIS_ADDA_n,0,{0},{0},{0},{{""}}},												/* M 2 T 7 (4,3) */
	{"01rrr110","LD_HL",Z80_LD_HL,Z80_DIS_LD_HL,1,{0x38},{3},{7},{{"111","000","001","010","011","100","101"}}},				/* M 2 T 7 (4,3) */
	{"11110110","ORA_n",Z80_ORA_n,Z80_DIS_ORA_n,0,{0},{0},{0},{{""}}},														/* M 2 T 7 (4,3) */
	{"00001111","RRCA",Z80_RRCA,Z80_DIS_RRCA,0,{0},{0},{0},{{""}}},															/* M 1 T 4 */
	{"00000111","RLCA",Z80_RLCA,Z80_DIS_RLCA,0,{0},{0},{0},{{""}}},															/* M 1 T 4 */
	{"00rrr101","DEC",Z80_DEC,Z80_DIS_DEC,1,{0x38},{3},{7},{{"111","000","001","010","011","100","101"}}},				/* M 1 T 4 */
	{"01rrraaa","LDrr",Z80_LDrr,Z80_DIS_LDrr,2,{0x38,0x07},{3,0},{7,7},{{"111","000","001","010","011","100","101"},{"111","000","001","010","011","100","101"}}},	/* M 1 T 4 */
	{"10110rrr","OR",Z80_OR,Z80_DIS_OR,1,{0x07},{0},{7},{{"111","000","001","010","011","100","101"}}},				/* M 1 T 4 */
	{"00111010","LDA_nn",Z80_LDA_nn,Z80_DIS_LDA_nn,0,{0},{0},{0},{{""}}},												/* M 4 T 13 (4,3,3,3) */
	{"00rrr100","INC",Z80_INC,Z80_DIS_INC,1,{0x38},{3},{7},{{"111","000","001","010","011","100","101"}}},				/* M 1 T 4 */
	{"11001001","RET",Z80_RET,Z80_DIS_RET,0,{0},{0},{0},{{""}}},																	/* M 3 T 10 (4,3,3) */
	{"00110010","LDm",Z80_LD_nn_A,Z80_DIS_LD_nn_A,0,{0},{0},{0},{{""}}},													/* M 4 T 13 (4,3,3,3) */
	{"01110rrr","LDm",Z80_LDm,Z80_DIS_LDm,1,{0x07},{0},{7},{{"111","000","001","010","011","100","101"}}},				/* M 2 T 7 (4,3) */
	{"11ccc010","JPcc",Z80_JPcc,Z80_DIS_JPcc,1,{0x38},{3},{1},{{"rrr"}}},			/* M 3 T 10 (4,3,3) */
	{"11001011","CB TABLE",Z80_CB_TABLE,Z80_DIS_CB_TABLE,0,{0},{0},{0},{{""}}},									/* see CB table */
	{"10101rrr","XOR",Z80_XOR,Z80_DIS_XOR,1,{0x07},{0},{7},{{"111","000","001","010","011","100","101"}}},				/* M 1 T 4 */
	{"00rrr110","LDn",Z80_LDn,Z80_DIS_LDn,1,{0x38},{3},{7},{{"111","000","001","010","011","100","101"}}},				/* M 2 T 7 (4,3) */
	{"11001101","CALL",Z80_CALL,Z80_DIS_CALL,0,{0},{0},{0},{{""}}},															/* M 5 T 17 (4,3,4,3,3) */
	{"11000011","JP",Z80_JP,Z80_DIS_JP,0,{0},{0},{0},{{""}}},																		/* M 3 T 10 (4,3,3) */
	{"11101101","ED TABLE",Z80_ED_TABLE,Z80_DIS_ED_TABLE,0,{0},{0},{0},{{""}}},									/* see ED table */
	{"00dd0001","LDRP",Z80_LDRP,Z80_DIS_LDRP,1,{0x30},{4},{1},{{"dd"}}},			/* M 3 T 10 (4,3,3) */
	{"11110011","DI",Z80_DI,Z80_DIS_DI,0,{0},{0},{0},{{""}}},																		/* M 1 T 4 */
	{"","",0,0,0,{0},{0},{0},{{""}}}
};

/* Base Table */
Z80_Function	Z80_JumpTable[256];
Z80_Decode	Z80_DisTable[256];
Z80_Ins		*Z80_Information[256];

/* CB Table */
Z80_Function	Z80_CB_JumpTable[256];
Z80_Decode	Z80_CB_DisTable[256];
Z80_Ins		*Z80_CB_Information[256];

/* ED Table */
Z80_Function	Z80_ED_JumpTable[256];
Z80_Decode	Z80_ED_DisTable[256];
Z80_Ins		*Z80_ED_Information[256];

U32 Z80_UNKNOWN(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80_regs.PC-=1;	/* account for prefetch */
	printf("ILLEGAL INSTRUCTION %08x\n",Z80_regs.PC);
	DEB_PauseEmulation(DEB_Mode_Z80,"ILLEGAL INSTRUCTION");
	return 0;
}

U32 Z80_TABLE_UNKNOWN(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
		Z80_regs.PC-=2;															/* Only true if DDCB or FDCB (arse) */
	Z80_regs.PC-=2;	/* account for prefetch */
	printf("ILLEGAL INSTRUCTION %08x\n",Z80_regs.PC);
	DEB_PauseEmulation(DEB_Mode_Z80,"ILLEGAL INSTRUCTION");
	return 0;
}

void Z80_TABLE(Z80_Ins **Infos)
{
	/* Fetch next instruction */
	Z80_regs.opcode = Z80_MEM_getByte(Z80_regs.PC);
	Z80_regs.PC+=1;

	if (Infos[Z80_regs.opcode])
	{
		int a;
		for (a=0;a<Infos[Z80_regs.opcode]->numOperands;a++)
		{
			Z80_regs.operands[a] = (Z80_regs.opcode & Infos[Z80_regs.opcode]->operandMask[a]) >> 
				Infos[Z80_regs.opcode]->operandShift[a];
		}
	}	
}

U32 Z80_CB_TABLE(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	switch (stage)
	{
	case 0:
		return 1;
	case 1:
		IncrementRefreshRegister();
		if (Z80_regs.ixAdjust || Z80_regs.iyAdjust)
		{
			Z80_regs.PC+=1;			/* skip displacement - due to DD CB dd Opcode */
		}	
		Z80_TABLE(Z80_CB_Information);
#if TRACE_OPCODES
		opcodesSeen[1*256+Z80_regs.opcode]=0x01 + (Z80_regs.ixAdjust*0x02) + (Z80_regs.iyAdjust*0x04);
#endif
		break;
	}

	return Z80_CB_JumpTable[Z80_regs.opcode](Z80_regs.stage,Z80_regs.operands[0],Z80_regs.operands[1],Z80_regs.operands[2],
				Z80_regs.operands[3],Z80_regs.operands[4],Z80_regs.operands[5],Z80_regs.operands[6],Z80_regs.operands[7]);
}

/* May be problem with this (DD DD DD would execute in 1 cycle i think) */
U32 Z80_DD_TABLE(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	switch (stage)
	{
	case 0:
		return 1;
	case 1:
	 	IncrementRefreshRegister();
		Z80_TABLE(Z80_Information);
		break;
	}
	Z80_regs.ixAdjust=1;
	Z80_regs.iyAdjust=0;

	return Z80_JumpTable[Z80_regs.opcode](Z80_regs.stage,Z80_regs.operands[0],Z80_regs.operands[1],Z80_regs.operands[2],
				Z80_regs.operands[3],Z80_regs.operands[4],Z80_regs.operands[5],Z80_regs.operands[6],Z80_regs.operands[7]);
}

U32 Z80_ED_TABLE(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	switch (stage)
	{
	case 0:
		return 1;
	case 1:
		IncrementRefreshRegister();
		Z80_TABLE(Z80_ED_Information);
#if TRACE_OPCODES
		opcodesSeen[2*256+Z80_regs.opcode]=0x01;
#endif
		break;
	}
	Z80_regs.ixAdjust=0;		/* as far as i know DD,FD have no effect on ED table */
	Z80_regs.iyAdjust=0;


	return Z80_ED_JumpTable[Z80_regs.opcode](Z80_regs.stage,Z80_regs.operands[0],Z80_regs.operands[1],Z80_regs.operands[2],
				Z80_regs.operands[3],Z80_regs.operands[4],Z80_regs.operands[5],Z80_regs.operands[6],Z80_regs.operands[7]);
}

U32 Z80_FD_TABLE(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	switch (stage)
	{
	case 0:
		return 1;
	case 1:
		IncrementRefreshRegister();
		Z80_TABLE(Z80_Information);
		break;
	}
	Z80_regs.ixAdjust=0;
	Z80_regs.iyAdjust=1;

	return Z80_JumpTable[Z80_regs.opcode](Z80_regs.stage,Z80_regs.operands[0],Z80_regs.operands[1],Z80_regs.operands[2],
				Z80_regs.operands[3],Z80_regs.operands[4],Z80_regs.operands[5],Z80_regs.operands[6],Z80_regs.operands[7]);
}

U16 Z80_DIS_TABLE(Z80_Ins **Infos,Z80_Decode *Decodes,U32 adr)
{
	U16 ret;
	S32	a;
	U8	opcode;
	U16	operands[8];

	opcode = Z80_MEM_getByte(adr);

	if (Infos[opcode])
	{
		for (a=0;a<Infos[opcode]->numOperands;a++)
		{
			operands[a] = (opcode & Infos[opcode]->operandMask[a]) >> 
									Infos[opcode]->operandShift[a];
		}
	}
			
	ret = Decodes[opcode](adr+1,operands[0],operands[1],operands[2],operands[3],operands[4],operands[5],operands[6],operands[7]) + 1;
	Z80_regs.ixDisAdjust=Z80_regs.iyDisAdjust=0;
	return ret;
}

U16 Z80_DIS_CB_TABLE(U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(adr);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	if (Z80_regs.ixDisAdjust || Z80_regs.iyDisAdjust)			/* DD CB dd Opcode (how annoying!) */
	{
		return Z80_DIS_TABLE(Z80_CB_Information,Z80_CB_DisTable,adr+1)+1;
	}
	return Z80_DIS_TABLE(Z80_CB_Information,Z80_CB_DisTable,adr);
}

U16 Z80_DIS_DD_TABLE(U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(adr);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.ixDisAdjust=1;
	Z80_regs.iyDisAdjust=0;
	return Z80_DIS_TABLE(Z80_Information,Z80_DisTable,adr);
}

U16 Z80_DIS_ED_TABLE(U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(adr);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.ixDisAdjust=0;
	Z80_regs.iyDisAdjust=0;
	return Z80_DIS_TABLE(Z80_ED_Information,Z80_ED_DisTable,adr);
}

U16 Z80_DIS_FD_TABLE(U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(adr);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);

	Z80_regs.ixDisAdjust=0;
	Z80_regs.iyDisAdjust=1;
	return Z80_DIS_TABLE(Z80_Information,Z80_DisTable,adr);
}

const char *z80_byte_to_binary(U32 x)
{
    U32 z;
    static char b[17] = {0};
	
    b[0]=0;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }
	
    return b;
}

U8 Z80_ValidateOpcode(int insNum,U8 opcode,Z80_Ins *Z80_Table)
{
    U8 invalidMask=0;
    int a,b,c;
    char *mask;
    int operandNum=0;
    U8 lastBase=0;
	
		for (a=0;a<8;a++)
		{
			if (Z80_Table[insNum].baseTable[a]!=lastBase && Z80_Table[insNum].baseTable[a]!='0' && Z80_Table[insNum].baseTable[a]!='1')
			{
				U8 operand = (opcode & Z80_Table[insNum].operandMask[operandNum]) >> Z80_Table[insNum].operandShift[operandNum];
				lastBase = Z80_Table[insNum].baseTable[a];

				for (b=0;b<Z80_Table[insNum].numValidMasks[operandNum];b++)
				{
					invalidMask = 0;
					mask = Z80_Table[insNum].validEffectiveAddress[operandNum][b];
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

void Z80_BuildCurTable(Z80_Function *Z80_Funcs,Z80_Decode *Z80_Decodes,Z80_Ins **Z80_Infos,Z80_Ins *Z80_Table,Z80_Function Unknown)
{
	int a,b,c,d;
	int numInstructions=0;

	for (a=0;a<256;a++)
	{
		Z80_Funcs[a]=Unknown;
		Z80_Decodes[a]=Z80_DIS_UNKNOWN;
		Z80_Infos[a]=0;
	}

	a=0;
	while (Z80_Table[a].opcode)
	{
		int modifiableBitCount=0;

		/* pre-count modifiable bits */
		for (b=0;b<8;b++)
		{
			switch (Z80_Table[a].baseTable[b])
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
			U8 opcode=0;

			d=0;
			/* Create instruction code */
			for (c=0;c<8;c++)
			{
				switch (Z80_Table[a].baseTable[c])
				{
				case '0':
					break;
				case '1':
					opcode|=1<<(7-c);
					break;
				case 'r':
				case 'b':
				case 'd':
				case 'q':
				case 'a':
				case 'A':
				case 'z':
				case 'm':
				case 'c':
					opcode|=((b&(1<<d))>>d)<<(7-c);
					d++;
					needValidation=1;
					break;
				}
			}
			if (needValidation)
			{
				if (!Z80_ValidateOpcode(a,opcode,Z80_Table))
				{
					validOpcode=0;
				}
			}
			if (validOpcode)
			{
/* 
				printf("Opcode Coding : %s : %02X %s\n", Z80_Table[a].opcodeName, opcode,z80_byte_to_binary(opcode));
*/
				if (Z80_Funcs[opcode]!=Unknown)
				{
					printf("[ERR] Cpu Coding For Instruction Overlap\n");
					exit(-1);
				}
				Z80_Funcs[opcode]=Z80_Table[a].opcode;
				Z80_Decodes[opcode]=Z80_Table[a].decode;
				Z80_Infos[opcode]=&Z80_Table[a];
				numInstructions++;
			}
			b++;
			if (b>=modifiableBitCount)
				break;
		}

		a++;
	}

	printf("Table contains %d / 256 entries\n",numInstructions);
}

void Z80_BuildTable()
{
	Z80_regs.resetLine=0;
	Z80_regs.stopped=1;
	Z80_BuildCurTable(Z80_JumpTable,Z80_DisTable,Z80_Information,Z80_instructions,Z80_UNKNOWN);
	Z80_BuildCurTable(Z80_CB_JumpTable,Z80_CB_DisTable,Z80_CB_Information,Z80_CB_instructions,Z80_TABLE_UNKNOWN);
	Z80_BuildCurTable(Z80_ED_JumpTable,Z80_ED_DisTable,Z80_ED_Information,Z80_ED_instructions,Z80_TABLE_UNKNOWN);
}

S8 Z80_signal = -1;

void Z80_SignalInterrupt(S8 level)
{
	if (level > Z80_signal)
	{
		Z80_signal=level;
	}
}

int Z80_CheckForInterrupt()
{
	if (Z80_signal==0 && Z80_regs.IFF1)
	{
		if (Z80_regs.InterruptMode==1)
		{
			Z80_signal=-1;
			
			Z80_regs.IFF2 = Z80_regs.IFF1;
			Z80_regs.IFF1 = 0;

			Z80_RST(0,7,0,0,0,0,0,0,0);
			Z80Cycles=13;
			Z80_regs.stopped=0;

			return 1;
		}
		else
		{
			if (Z80_regs.InterruptMode==2)
			{
				U16 newPC;
				U16 vecAddress;

				Z80_signal=-1;

				Z80_regs.IFF2 = Z80_regs.IFF1;
				Z80_regs.IFF1 = 0;

				/*DBUS contains 0xFF always*/

				vecAddress=(Z80_regs.IR&0xFF00)|0xFF;

				newPC=Z80_MEM_getByte(vecAddress);
				newPC|=Z80_MEM_getByte(vecAddress+1)<<8;

				Z80_regs.SP--;
				Z80_MEM_setByte(Z80_regs.SP,Z80_regs.PC>>8);
				Z80_regs.SP--;
				Z80_MEM_setByte(Z80_regs.SP,Z80_regs.PC&0xFF);

				Z80_regs.PC=newPC;

				Z80Cycles=19;

				Z80_regs.stopped=0;

				return 1;
			}
			else
			{
				Z80_signal=-1;

				Z80_regs.IFF2 = Z80_regs.IFF1;
				Z80_regs.IFF1 = 0;

				Z80_RST(0,7,0,0,0,0,0,0,0);				/* Spectrum BODGE! should execute instruction from bus */
																					/*but speccy will always return 0xFF which is RST 38*/
				Z80Cycles=13;
				Z80_regs.stopped=0;

				return 1;
			}
		}
	}
	else
	{
		Z80_signal=-1;			/* throw away request.. opertunity missed*/
	}

	return 0;
}

void Z80_Reset()
{
	int a;

#if TRACE_OPCODES
	for (a=0;a<256*3;a++)
	{
		opcodesSeenBefore[a]=0;
		opcodesSeenAfter[a]=0;
	}
#endif
	for (a=0;a<8;a++)
	{
		Z80_regs.R[a]=0;
		Z80_regs.R_[a]=0;
	}
	
	Z80_regs.PC = 0;

	Z80_regs.InterruptMode=0;

	Z80_regs.stage=0;

	Z80Cycles=0;

	Z80_regs.delayedInterruptEnable=0;
}

void Z80_Step()			/* needs to go at half speed of 68000! */
{
	static int cycles=0;
	int a;

	if (Z80_regs.resetLine)
	{
		Z80_Reset();
		Z80_regs.resetLine=0;			/* probably should wait 3 cycles for completion */
/*
		DEB_PauseEmulation(DEB_Mode_Z80,"");
*/
		return;
	}

	if (!Z80_regs.stage)
	{
		if (Z80_regs.delayedInterruptEnable)
		{
			Z80_regs.delayedInterruptEnable--;
			if (!Z80_regs.delayedInterruptEnable)
			{
				Z80_regs.IFF1=1;
				Z80_regs.IFF2=1;
			}
		}

		if (Z80Cycles)					/* crude cycle accuracy */
		{
			Z80Cycles--;
			return;
		}
	
		Z80_regs.ixAdjust=0;
		Z80_regs.iyAdjust=0;

		IncrementRefreshRegister();

		Z80_CheckForInterrupt();

		if (Z80_regs.stopped)
			return;

		Z80_regs.lastInstruction = Z80_regs.PC;
		
		/* Fetch next instruction */
		Z80_regs.opcode = Z80_MEM_getByte(Z80_regs.PC);
		Z80_regs.PC+=1;

		if (Z80_Information[Z80_regs.opcode])
		{
			for (a=0;a<Z80_Information[Z80_regs.opcode]->numOperands;a++)
			{
				Z80_regs.operands[a] = (Z80_regs.opcode & Z80_Information[Z80_regs.opcode]->operandMask[a]) >> 
										Z80_Information[Z80_regs.opcode]->operandShift[a];
			}
		}
		
		cycles=0;
		
	}

	Z80_regs.stage = Z80_JumpTable[Z80_regs.opcode](Z80_regs.stage,Z80_regs.operands[0],Z80_regs.operands[1],Z80_regs.operands[2],
				Z80_regs.operands[3],Z80_regs.operands[4],Z80_regs.operands[5],Z80_regs.operands[6],Z80_regs.operands[7]);
				
	cycles++;
}

int Z80_Cycle_Step()						/* Entry always assumed to be stage==0 */
{
	static int cycles=0;
	int a;

	if (Z80_regs.delayedInterruptEnable)
	{
		Z80_regs.delayedInterruptEnable--;
		if (!Z80_regs.delayedInterruptEnable)
		{
			Z80_regs.IFF1=1;
			Z80_regs.IFF2=1;
		}
	}

	Z80_regs.ixAdjust=0;
	Z80_regs.iyAdjust=0;

	IncrementRefreshRegister();

	if (Z80_CheckForInterrupt())
		return Z80Cycles;

	if (Z80_regs.stopped)
	{
		/* Perform NOP */
		Z80_NOP(0,0,0,0,0,0,0,0,0);
		return Z80Cycles;
	}

	Z80_regs.lastInstruction = Z80_regs.PC;
		
	/* Fetch next instruction */
	Z80_regs.opcode = Z80_MEM_getByte(Z80_regs.PC);
	Z80_regs.PC+=1;

#if TRACE_OPCODES
	opcodesSeen[0*256+Z80_regs.opcode]=0x01 + (Z80_regs.ixAdjust*0x02) + (Z80_regs.iyAdjust*0x04);
#endif

	if (Z80_Information[Z80_regs.opcode])
	{
		for (a=0;a<Z80_Information[Z80_regs.opcode]->numOperands;a++)
		{
			Z80_regs.operands[a] = (Z80_regs.opcode & Z80_Information[Z80_regs.opcode]->operandMask[a]) >> 
									Z80_Information[Z80_regs.opcode]->operandShift[a];
		}
	}
		
	cycles=0;

	do 
	{
		Z80_regs.stage = Z80_JumpTable[Z80_regs.opcode](Z80_regs.stage,Z80_regs.operands[0],Z80_regs.operands[1],Z80_regs.operands[2],
				Z80_regs.operands[3],Z80_regs.operands[4],Z80_regs.operands[5],Z80_regs.operands[6],Z80_regs.operands[7]);
	} while (Z80_regs.stage!=0);

	return Z80Cycles;
}
