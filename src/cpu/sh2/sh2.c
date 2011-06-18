/*
 * SH2.c

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

/*

 TO DO :

  Illegal slot instructions
	Delay slot dependancy issue : JMP @R5; ADD R5,R5			-> should jump to r5*2
	Remaining opcodes
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "mytypes.h"
#include "platform.h"

#include "sh2.h"
#include "sh2_dis.h"
#include "sh2_ops.h"
#include "sh2_memory.h"
#include "sh2_ioregisters.h"

#define		INTERRUPTS_ENABLED		1

#if 1
U32 BPAddress=0xFFFFFFFF;
#else
//U32 BPAddress=0x06030A14;			/* main */
//U32 BPAddress=0x06030A8A;			/* calling line start */
//U32 BPAddress=0x06030AAE;			/* calling init edges */
//U32 BPAddress=0x06030AB2;			/* calling init layers */
//U32 BPAddress=0x06030AB6;			/* calling init objects */
//U32 BPAddress=0x06030ABA;			/* calling alloc object */
//U32 BPAddress=0x06030B5C;			/* @loop calling clear edges */
//U32 BPAddress=0x06030B7C;			/* calling queue objects */
//U32 BPAddress=0x06030936;			/* calling multiply matrices (3x3 * n*1x3) */
//U32 BPAddress=0x0603093E;			/* calling queue faces */
//U32 BPAddress=0x06030942;			/* after calling queue faces */
U32 BPAddress=0x020045d6;
/*0x060007bc;*//*0x06000788;*//*0x06000432;*//*0x060305e6;*//*0x06030628;*/
#endif

SH2_Ins SH2_Instructions[] = 
{
	{"0000mmmm00100011","BRAF Rm",SH2_BRAF_RM,SH2_DIS_BRAF_RM,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0110nnnnmmmm1010","NEGC Rm,Rn",SH2_NEGC_RM_RN,SH2_DIS_NEGC_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0100mmmm00010111","LDC.L @Rm+,GBR",SH2_LDCL_AT_RM_INC_GBR,SH2_DIS_LDCL_AT_RM_INC_GBR,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0000000000101011","RTE",SH2_RTE,SH2_DIS_RTE,0,{0},{0},{0},{{""}}},
	{"11001100iiiiiiii","TST #i,@(R0,GBR)",SH2_TSTB_IMMEDIATE_AT_R0_GBR,SH2_DIS_TSTB_IMMEDIATE_AT_R0_GBR,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
	{"11001111iiiiiiii","OR #i,@(R0,GBR)",SH2_ORB_IMMEDIATE_AT_R0_GBR,SH2_DIS_ORB_IMMEDIATE_AT_R0_GBR,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
	{"0011nnnnmmmm0101","DMULU.L Rm,Rn",SH2_DMULUL_RM_RN,SH2_DIS_DMULUL_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0100nnnn00000011","STS.L SR,@-Rn",SH2_STSL_SR_AT_DEC_RN,SH2_DIS_STSL_SR_AT_DEC_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0100mmmm00000111","LDC.L @Rm+,SR",SH2_LDCL_AT_RM_INC_SR,SH2_DIS_LDCL_AT_RM_INC_SR,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0100nnnn00000101","ROTR Rn",SH2_ROTR_RN,SH2_DIS_ROTR_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0110nnnnmmmm1110","EXTS.B Rm,Rn",SH2_EXTSB_RM_RN,SH2_DIS_EXTSB_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0000nnnn00101001","MOVT Rn",SH2_MOVT_RN,SH2_DIS_MOVT_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0100nnnn00011001","SHLR8 Rn",SH2_SHLR8_RN,SH2_DIS_SHLR8_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0100mmmm00101010","LDS Rm,PR",SH2_LDS_RM_PR,SH2_DIS_LDS_RM_PR,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0000nnnnmmmm1111","MAC.L @Rm+,@Rn+",SH2_MACL_AT_RM_INC_AT_RN_INC,SH2_DIS_MACL_AT_RM_INC_AT_RN_INC,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0000000000101000","CLRMAC",SH2_CLRMAC,SH2_DIS_CLRMAC,0,{0},{0},{0},{{""}}},
	{"0100nnnn00100000","SHAL Rn",SH2_SHAL_RN,SH2_DIS_SHAL_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"10000101mmmmdddd","MOV.W @(DISP,Rm),R0",SH2_MOVW_AT_DISP_RM_R0,SH2_DIS_MOVW_AT_DISP_RM_R0,2,{0x00F0,0x000F},{4,0},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0011nnnnmmmm1101","DMULS.L Rm,Rn",SH2_DMULSL_RM_RN,SH2_DIS_DMULSL_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0011nnnnmmmm0010","CMP/HS Rm,Rn",SH2_CMPHS_RM_RN,SH2_DIS_CMPHS_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0100nnnn00010011","STC.L GBR,@-Rn",SH2_STCL_GBR_AT_DEC_RN,SH2_DIS_STCL_GBR_AT_DEC_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"10000001nnnndddd","MOV.W R0,@(DISP,Rn)",SH2_MOVW_R0_AT_DISP_RN,SH2_DIS_MOVW_R0_AT_DISP_RN,2,{0x00F0,0x000F},{4,0},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0010nnnnmmmm0101","MOV.W Rm,@-Rn",SH2_MOVW_RM_AT_DEC_RN,SH2_DIS_MOVW_RM_AT_DEC_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0000000000001000","CLRT",SH2_CLRT,SH2_DIS_CLRT,0,{0},{0},{0},{{""}}},
	{"0000nnnn00001010","STS MACH,Rn",SH2_STS_MACH_RN,SH2_DIS_STS_MACH_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0000nnnn00101010","STS PR,Rn",SH2_STS_PR_RN,SH2_DIS_STS_PR_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0000nnnn00010010","STC GBR,Rn",SH2_STC_GBR_RN,SH2_DIS_STC_GBR_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0000nnnn00000010","STC SR,Rn",SH2_STC_SR_RN,SH2_DIS_STC_SR_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0000nnnn00100010","STC VBR,Rn",SH2_STC_VBR_RN,SH2_DIS_STC_VBR_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0000000000011011","SLEEP",SH2_SLEEP,SH2_DIS_SLEEP,0,{0},{0},{0},{{""}}},
	{"11000101dddddddd","MOV.W @(DISP,GBR),R0",SH2_MOVW_AT_DISP_GBR_R0,SH2_DIS_MOVW_AT_DISP_GBR_R0,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
	{"0010nnnnmmmm1111","MULS.W Rm,Rn",SH2_MULSW_RM_RN,SH2_DIS_MULSW_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0011nnnnmmmm0110","CMP/HI Rm,Rn",SH2_CMPHI_RM_RN,SH2_DIS_CMPHI_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"10001101dddddddd","BT/S #i*2+PC",SH2_BTS,SH2_DIS_BTS,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
	{"11001011iiiiiiii","OR #i,R0",SH2_OR_IMMEDIATE_R0,SH2_DIS_OR_IMMEDIATE_R0,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
	{"0100nnnn00101001","SHLR16 Rn",SH2_SHLR16_RN,SH2_DIS_SHLR16_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0000000000011001","DIV0U",SH2_DIV0U,SH2_DIS_DIV0U,0,{0},{0},{0},{{""}}},
	{"0000nnnnmmmm0111","MUL.L Rm,Rn",SH2_MULL_RM_RN,SH2_DIS_MULL_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"11000111dddddddd","MOVA @(DISP,PC),R0",SH2_MOVA_AT_DISP_PC_R0,SH2_DIS_MOVA_AT_DISP_PC_R0,2,{0x0F00,0x00FF},{8,0},{1,1},{{"rrrr"},{"rrrrrrrr"}}},
	{"1011dddddddddddd","BSR #i*2+PC",SH2_BSR,SH2_DIS_BSR,1,{0x0FFF},{0},{1},{{"rrrrrrrrrrrr"}}},
	{"0110nnnnmmmm0111","NOT Rm,Rn",SH2_NOT_RM_RN,SH2_DIS_NOT_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"11000000dddddddd","MOV.B R0,@(DISP,GBR)",SH2_MOVB_R0_AT_DISP_GBR,SH2_DIS_MOVB_R0_AT_DISP_GBR,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
	{"11000110dddddddd","MOV.L @(DISP,GBR),R0",SH2_MOVL_AT_DISP_GBR_R0,SH2_DIS_MOVL_AT_DISP_GBR_R0,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
	{"10000000nnnndddd","MOV.B R0,@(DISP,Rn)",SH2_MOVB_R0_AT_DISP_RN,SH2_DIS_MOVB_R0_AT_DISP_RN,2,{0x00F0,0x000F},{4,0},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0011nnnnmmmm0011","CMP/GE Rm,Rn",SH2_CMPGE_RM_RN,SH2_DIS_CMPGE_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0110nnnnmmmm1011","NEG Rm,Rn",SH2_NEG_RM_RN,SH2_DIS_NEG_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0011nnnnmmmm1110","ADDC Rm,Rn",SH2_ADDC_RM_RN,SH2_DIS_ADDC_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0011nnnnmmmm0100","DIV1 Rm,Rn",SH2_DIV1_RM_RN,SH2_DIS_DIV1_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0011nnnnmmmm1010","SUBC Rm,Rn",SH2_SUBC_RM_RN,SH2_DIS_SUBC_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0010nnnnmmmm0111","DIV0S Rm,Rn",SH2_DIV0S_RM_RN,SH2_DIS_DIV0S_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0100mmmm00100110","LDS.L @Rn+,PR",SH2_LDSL_AT_RM_INC_PR,SH2_DIS_LDSL_AT_RM_INC_PR,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0000nnnnmmmm0110","MOV.L Rm,@(R0,Rn)",SH2_MOVL_RM_AT_R0_RN,SH2_DIS_MOVL_RM_AT_R0_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0000nnnnmmmm1110","MOV.L @(R0,Rm),Rn",SH2_MOVL_AT_R0_RM_RN,SH2_DIS_MOVL_AT_R0_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0000000000001011","RTS",SH2_RTS,SH2_DIS_RTS,0,{0},{0},{0},{{""}}},
	{"0100nnnn00101000","SHLL16 Rn",SH2_SHLL16_RN,SH2_DIS_SHLL16_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0000nnnn00011010","STS MACL,Rn",SH2_STS_MACL_RN,SH2_DIS_STS_MACL_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0010nnnnmmmm1101","XTRCT Rm,Rn",SH2_XTRCT_RM_RN,SH2_DIS_XTRCT_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0110nnnnmmmm1001","SWAP.W Rm,Rn",SH2_SWAPW_RM_RN,SH2_DIS_SWAPW_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0010nnnnmmmm1110","MULU.W Rm,Rn",SH2_MULUW_RM_RN,SH2_DIS_MULUW_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0000nnnnmmmm1100","MOV.B @(R0,Rm),Rn",SH2_MOVB_AT_R0_RM_RN,SH2_DIS_MOVB_AT_R0_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0100nnnn00010101","CMP/PL Rn",SH2_CMPPL_RN,SH2_DIS_CMPPL_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0110nnnnmmmm0010","MOV.L @Rm,Rn",SH2_MOVL_AT_RM_RN,SH2_DIS_MOVL_AT_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"11001001iiiiiiii","AND #i,R0",SH2_AND_IMMEDIATE_R0,SH2_DIS_AND_IMMEDIATE_R0,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
	{"0100nnnn00010001","CMP/PZ Rn",SH2_CMPPZ_RN,SH2_DIS_CMPPZ_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0110nnnnmmmm1111","EXTS.W Rm,Rn",SH2_EXTSW_RM_RN,SH2_DIS_EXTSW_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0100nnnn00100010","STS.L PR,@-Rn",SH2_STSL_PR_AT_DEC_RN,SH2_DIS_STSL_PR_AT_DEC_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0100mmmm00001011","JSR @Rm",SH2_JSR_AT_RM,SH2_DIS_JSR_AT_RM,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0100nnnn00100100","ROTCL Rn",SH2_ROTCL_RN,SH2_DIS_ROTCL_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0100nnnn00100001","SHAR Rn",SH2_SHAR_RN,SH2_DIS_SHAR_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"1010dddddddddddd","BRA #i*2+PC",SH2_BRA,SH2_DIS_BRA,1,{0x0FFF},{0},{1},{{"rrrrrrrrrrrr"}}},
	{"0110nnnnmmmm0101","MOV.W @Rm+,Rn",SH2_MOVW_AT_RM_INC_RN,SH2_DIS_MOVW_AT_RM_INC_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0010nnnnmmmm1010","XOR Rm,Rn",SH2_XOR_RM_RN,SH2_DIS_XOR_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"11001010iiiiiiii","XOR #i,R0",SH2_XOR_IMMEDIATE_R0,SH2_DIS_XOR_IMMEDIATE_R0,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
	{"0000nnnnmmmm0100","MOV.B Rm,@(R0,Rn)",SH2_MOVB_RM_AT_R0_RN,SH2_DIS_MOVB_RM_AT_R0_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0010nnnnmmmm0100","MOV.B Rm,@-Rn",SH2_MOVB_RM_AT_DEC_RN,SH2_DIS_MOVB_RM_AT_DEC_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0010nnnnmmmm1001","AND Rm,Rn",SH2_AND_RM_RN,SH2_DIS_AND_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0000nnnnmmmm1101","MOV.W @(R0,Rm),Rn",SH2_MOVW_AT_R0_RM_RN,SH2_DIS_MOVW_AT_R0_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0110nnnnmmmm0100","MOV.B @Rm+,Rn",SH2_MOVB_AT_RM_INC_RN,SH2_DIS_MOVB_AT_RM_INC_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0110nnnnmmmm1000","SWAP.B Rm,Rn",SH2_SWAPB_RM_RN,SH2_DIS_SWAPB_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0100nnnn00001001","SHLR2 Rn",SH2_SHLR2_RN,SH2_DIS_SHLR2_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"10000100mmmmdddd","MOV.B @(DISP,Rm),R0",SH2_MOVB_AT_DISP_RM_R0,SH2_DIS_MOVB_AT_DISP_RM_R0,2,{0x00F0,0x000F},{4,0},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0110nnnnmmmm0000","MOV.B @Rm,Rn",SH2_MOVB_AT_RM_RN,SH2_DIS_MOVB_AT_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0110nnnnmmmm1100","EXTU.B Rm,Rn",SH2_EXTUB_RM_RN,SH2_DIS_EXTUB_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0011nnnnmmmm0111","CMP/GT Rm,Rn",SH2_CMPGT_RM_RN,SH2_DIS_CMPGT_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0100nnnn00000001","SHLR Rn",SH2_SHLR_RN,SH2_DIS_SHLR_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0100nnnn00011000","SHLL8 Rn",SH2_SHLL8_RN,SH2_DIS_SHLL8_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0100nnnn00000000","SHLL Rn",SH2_SHLL_RN,SH2_DIS_SHLL_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0000nnnnmmmm0101","MOV.W Rm,@(R0,Rn)",SH2_MOVW_RM_AT_R0_RN,SH2_DIS_MOVW_RM_AT_R0_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0100nnnn00001000","SHLL2 Rn",SH2_SHLL2_RN,SH2_DIS_SHLL2_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0010nnnnmmmm1011","OR Rm,Rn",SH2_OR_RM_RN,SH2_DIS_OR_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0110nnnnmmmm0011","MOV Rm,Rn",SH2_MOV_RM_RN,SH2_DIS_MOV_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0010nnnnmmmm1000","TST Rm,Rn",SH2_TST_RM_RN,SH2_DIS_TST_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"10001111dddddddd","BF/S #i*2+PC",SH2_BFS,SH2_DIS_BFS,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
	{"0100nnnn00010000","DT Rn",SH2_DT_RN,SH2_DIS_DT_RN,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"1001nnnndddddddd","MOV.W @(DISP,PC),Rn",SH2_MOVW_AT_DISP_PC_RN,SH2_DIS_MOVW_AT_DISP_PC_RN,2,{0x0F00,0x00FF},{8,0},{1,1},{{"rrrr"},{"rrrrrrrr"}}},
	{"0000000000001001","NOP",SH2_NOP,SH2_DIS_NOP,0,{0},{0},{0},{{""}}},
	{"0100mmmm00101011","JMP @Rm",SH2_JMP_AT_RM,SH2_DIS_JMP_AT_RM,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"11000010dddddddd","MOV.L R0,@(DISP,GBR)",SH2_MOVL_R0_AT_DISP_GBR,SH2_DIS_MOVL_R0_AT_DISP_GBR,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
	{"0100mmmm00101110","LDC Rm,VBR",SH2_LDC_RM_VBR,SH2_DIS_LDC_RM_VBR,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0101nnnnmmmmdddd","MOV.L @(DISP,Rm),Rn",SH2_MOVL_AT_DISP_RM_RN,SH2_DIS_MOVL_AT_DISP_RM_RN,3,{0x0F00,0x00F0,0x000F},{8,4,0},{1,1,1},{{"rrrr"},{"rrrr"},{"rrrr"}}},
	{"0011nnnnmmmm1000","SUB Rm,Rn",SH2_SUB_RM_RN,SH2_DIS_SUB_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0111nnnniiiiiiii","ADD #i,Rn",SH2_ADD_IMMEDIATE_RN,SH2_DIS_ADD_IMMEDIATE_RN,2,{0x0F00,0x00FF},{8,0},{1,1},{{"rrrr"},{"rrrrrrrr"}}},
	{"0010nnnnmmmm0010","MOV.L Rm,@Rn",SH2_MOVL_RM_AT_RN,SH2_DIS_MOVL_RM_AT_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0011nnnnmmmm1100","ADD Rm,Rn",SH2_ADD_RM_RN,SH2_DIS_ADD_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"11000001dddddddd","MOV.W R0,@(DISP,GBR)",SH2_MOVW_R0_AT_DISP_GBR,SH2_DIS_MOVW_R0_AT_DISP_GBR,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
	{"10001001dddddddd","BT #i*2+PC",SH2_BT,SH2_DIS_BT,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
	{"10001000iiiiiiii","CMP/EQ #i,R0",SH2_CMPEQ_IMMEDIATE_R0,SH2_DIS_CMPEQ_IMMEDIATE_R0,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
	{"0110nnnnmmmm1101","EXTU.W Rm,Rn",SH2_EXTUW_RM_RN,SH2_DIS_EXTUW_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0110nnnnmmmm0001","MOV.W @Rm,Rn",SH2_MOVW_AT_RM_RN,SH2_DIS_MOVW_AT_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0011nnnnmmmm0000","CMP/EQ Rm,Rn",SH2_CMPEQ_RM_RN,SH2_DIS_CMPEQ_RM_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0010nnnnmmmm0110","MOV.L Rm,@-Rn",SH2_MOVL_RM_AT_DEC_RN,SH2_DIS_MOVL_RM_AT_DEC_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0010nnnnmmmm0000","MOV.B Rm,@Rn",SH2_MOVB_RM_AT_RN,SH2_DIS_MOVB_RM_AT_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"10001011dddddddd","BF #i*2+PC",SH2_BF,SH2_DIS_BF,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
	{"11001000iiiiiiii","TST #i,R0",SH2_TST_IMMEDIATE_R0,SH2_DIS_TST_IMMEDIATE_R0,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
	{"11000100dddddddd","MOV.B @(DISP,GBR),R0",SH2_MOVB_AT_DISP_GBR_R0,SH2_DIS_MOVB_AT_DISP_GBR_R0,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
	{"0010nnnnmmmm0001","MOV.W Rm,@Rn",SH2_MOVW_RM_AT_RN,SH2_DIS_MOVW_RM_AT_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"1110nnnniiiiiiii","MOV #i,Rn",SH2_MOV_IMMEDIATE_RN,SH2_DIS_MOV_IMMEDIATE_RN,2,{0x0F00,0x00FF},{8,0},{1,1},{{"rrrr"},{"rrrrrrrr"}}},
	{"0100mmmm00011110","LDC Rm,GBR",SH2_LDC_RM_GBR,SH2_DIS_LDC_RM_GBR,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"0001nnnnmmmmdddd","MOV.L Rm,@(DISP,Rn)",SH2_MOVL_RM_AT_DISP_RN,SH2_DIS_MOVL_RM_AT_DISP_RN,3,{0x0F00,0x00F0,0x000F},{8,4,0},{1,1,1},{{"rrrr"},{"rrrr"},{"rrrr"}}},
	{"0110nnnnmmmm0110","MOV.L @Rm+,Rn",SH2_MOVL_AT_RM_INC_RN,SH2_DIS_MOVL_AT_RM_INC_RN,2,{0x0F00,0x00F0},{8,4},{1,1},{{"rrrr"},{"rrrr"}}},
	{"0100mmmm00001110","LDC Rm,SR",SH2_LDC_RM_SR,SH2_DIS_LDC_RM_SR,1,{0x0F00},{8},{1},{{"rrrr"}}},
	{"1101nnnndddddddd","MOV.L @(DISP,PC),Rn",SH2_MOVL_AT_DISP_PC_RN,SH2_DIS_MOVL_AT_DISP_PC_RN,2,{0x0F00,0x00FF},{8,0},{1,1},{{"rrrr"},{"rrrrrrrr"}}},
{"","",0,0,0,{0},{0},{0},{{""}}}
};

SH2_Function	SH2_JumpTable[65536];
SH2_Decode	SH2_DisTable[65536];
SH2_Ins		*SH2_Information[65536];

const char *SH2_byte_to_binary(U32 x)
{
    U32 z;
		U32 c;
    static char b[17] = {0};
	
    b[0]=0;
		c=0;
    for (z = 32768; z > 0; z >>= 1)
    {
			b[c]=((x & z) == z) ? '1' : '0';
			c++;
			b[c]=0;
    }
	
    return b;
}

U8 SH2_ValidateOpcode(int insNum,U16 opcode)
{
    U8 invalidMask=0;
    int a,b,c;
    char *mask;
    int operandNum=0;
    U8 lastBase=0;
	
    for (a=0;a<16;a++)
    {
		if (SH2_Instructions[insNum].baseTable[a]!=lastBase && SH2_Instructions[insNum].baseTable[a]!='0' && SH2_Instructions[insNum].baseTable[a]!='1')
		{
			U16 operand = (opcode & SH2_Instructions[insNum].operandMask[operandNum]);
			operand >>= SH2_Instructions[insNum].operandShift[operandNum];
			lastBase = SH2_Instructions[insNum].baseTable[a];
			
			for (b=0;b<SH2_Instructions[insNum].numValidMasks[operandNum];b++)
			{
				invalidMask = 0;
				mask = SH2_Instructions[insNum].validEffectiveAddress[operandNum][b];
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

void SH2_BuildTable()
{
	int a,b,c,d;
	int numInstructions=0;
	
	for (a=0;a<65536;a++)
	{
		SH2_JumpTable[a]=SH2_UNKNOWN;
		SH2_DisTable[a]=SH2_DIS_UNKNOWN;
		SH2_Information[a]=0;
	}
	
	a=0;
	while (SH2_Instructions[a].opcode)
	{
	    int modifiableBitCount=0;
		
	    /* precount modifiable bits */
	    for (b=0;b<16;b++)
	    {
			switch (SH2_Instructions[a].baseTable[b])
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
				switch (SH2_Instructions[a].baseTable[c])
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
					case 'n':
					case 'i':
					case 'c':
						opcode|=((b&(1<<d))>>d)<<(15-c);
						d++;
						needValidation=1;
						break;
				}
			}
			if (needValidation)
			{
				if (!SH2_ValidateOpcode(a,opcode))
				{
					validOpcode=0;
				}
			}
			if (validOpcode)
			{
/*				printf("Opcode Coding : %s : %04X %s\n", SH2_Instructions[a].opcodeName, opcode,SH2_byte_to_binary(opcode));		*/

				if (SH2_JumpTable[opcode]!=SH2_UNKNOWN)
				{
					printf("[ERR] Cpu Coding For Instruction Overlap\n");
					exit(-1);
				}
				SH2_JumpTable[opcode]=SH2_Instructions[a].opcode;
				SH2_DisTable[opcode]=SH2_Instructions[a].decode;
				SH2_Information[opcode]=&SH2_Instructions[a];
				numInstructions++;
			}
			b++;
			if (b>=modifiableBitCount)
				break;
	    }
		
	    a++;
	}

	printf("SH2 %d out of %d instructions\n",numInstructions,65536);
}

void SH2_DumpEmulatorState(SH2_State* cpu)
{
	printf("\n");
	printf("R00 =%08X\tR01 =%08X\tR02 =%08X\tR03 =%08X\n",cpu->R[0],cpu->R[1],cpu->R[2],cpu->R[3]);
	printf("R04 =%08X\tR05 =%08X\tR06 =%08X\tR07 =%08X\n",cpu->R[4],cpu->R[5],cpu->R[6],cpu->R[7]);
	printf("R08 =%08X\tR09 =%08X\tR10 =%08X\tR11 =%08X\n",cpu->R[8],cpu->R[9],cpu->R[10],cpu->R[11]);
	printf("R12 =%08X\tR13 =%08X\tR14 =%08X\tR15 =%08X\n",cpu->R[12],cpu->R[13],cpu->R[14],cpu->R[15]);
	printf("\n");
	printf("MACH=%08X\tMACL=%08X\tPR  =%08X\tPC  =%08X\n",cpu->MACH,cpu->MACL,cpu->PR,cpu->PC);
	printf("GBR =%08X\tVBR =%08X\tCCR =%02X\n",cpu->GBR,cpu->VBR,cpu->IOMemory[SH2_IO_CCR]);
	printf("\n");
	printf("          [   :  :  :  :  :  : M: Q:I3:I2:I1:I0:  :  : S: T ]\n");
	printf("SR = %04X [ %s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s ]\n", cpu->SR&0xFFFF, 
		cpu->SR & 0x8000 ? " 1" : " 0",
		cpu->SR & 0x4000 ? " 1" : " 0",
		cpu->SR & 0x2000 ? " 1" : " 0",
		cpu->SR & 0x1000 ? " 1" : " 0",
		cpu->SR & 0x0800 ? " 1" : " 0",
		cpu->SR & 0x0400 ? " 1" : " 0",
		cpu->SR & 0x0200 ? " 1" : " 0",
		cpu->SR & 0x0100 ? " 1" : " 0",
		cpu->SR & 0x0080 ? " 1" : " 0",
		cpu->SR & 0x0040 ? " 1" : " 0",
		cpu->SR & 0x0020 ? " 1" : " 0",
		cpu->SR & 0x0010 ? " 1" : " 0",
		cpu->SR & 0x0008 ? " 1" : " 0",
		cpu->SR & 0x0004 ? " 1" : " 0",
		cpu->SR & 0x0002 ? " 1" : " 0",
		cpu->SR & 0x0001 ? " 1" : " 0");
	printf("\n");
	printf("PipeLine : %d\t%d\t%d\t%d\t%d\n",cpu->pipeLine[0].stage,cpu->pipeLine[1].stage,cpu->pipeLine[2].stage,cpu->pipeLine[3].stage,cpu->pipeLine[4].stage);
	printf("PipeLine : %04X\t%04X\t%04X\t%04X\t%04X\n",cpu->pipeLine[0].opcode,cpu->pipeLine[1].opcode,cpu->pipeLine[2].opcode,cpu->pipeLine[3].opcode,cpu->pipeLine[4].opcode);
}

void SH2_ShowTrace(SH2_State* cpu)
{
	U16 opcode;
	U16 operands[8];
	U32	insCount;
	U32 fetchFrom = cpu->PC;
	U32 a,b;
	
	SH2_DumpEmulatorState(cpu);

	printf("In Pipe\n");
	for (a=0;a<SH2_PIPELINE_SIZE;a++)
	{
		if (cpu->pipeLine[a].stage>0)
		{
			fetchFrom=cpu->pipeLine[a].PC;
			insCount=SH2_DisTable[cpu->pipeLine[a].opcode](cpu,fetchFrom,cpu->pipeLine[a].operands[0],cpu->pipeLine[a].operands[1],cpu->pipeLine[a].operands[2],cpu->pipeLine[a].operands[3],cpu->pipeLine[a].operands[4],cpu->pipeLine[a].operands[5],cpu->pipeLine[a].operands[6],cpu->pipeLine[a].operands[7]);
		
			printf("%d (%d) %08X\t",cpu->pipeLine[a].stage,cpu->pipeLine[a].Discard,fetchFrom);

			for (b=0;b<(insCount+2)/2;b++)
			{
				printf("%04X ",SH2_Debug_Read_Word(cpu,fetchFrom+b*2));
			}

			printf("\t%s\n",SH2_mnemonicData);
		}
		else
		{
			printf("%d (%d) EMPTY\n",cpu->pipeLine[a].stage,cpu->pipeLine[a].Discard);
		}
	}

	printf("PC Disasm\n");
	if (cpu->pipeLine[0].stage==0)
	{
		fetchFrom=cpu->PC;
	}
	else
	{
		fetchFrom=cpu->pipeLine[0].PC;
	}

	for (a=0;a<2;a++)
	{
		opcode = SH2_Debug_Read_Word(cpu,fetchFrom);
		if (SH2_Information[opcode])
		{
			for (b=0;b<SH2_Information[opcode]->numOperands;b++)
			{
				operands[b] = (opcode & SH2_Information[opcode]->operandMask[b]) >> SH2_Information[opcode]->operandShift[b];
			}
		}
		
		insCount=SH2_DisTable[opcode](cpu,fetchFrom,operands[0],operands[1],operands[2],operands[3],operands[4],operands[5],operands[6],operands[7]);
		
		printf("%08X\t",fetchFrom);

		for (b=0;b<(insCount+2)/2;b++)
		{
			printf("%04X ",SH2_Debug_Read_Word(cpu,fetchFrom+b*2));
		}

		fetchFrom+=2+insCount;
		printf("\t%s\n",SH2_mnemonicData);
	}
}

U32 SH2_InstructionFetch(SH2_State* cpu,U32 slot)
{
	if (cpu->IFMask)		/* Do we have valid opcode for next instruction */
	{
		if (cpu->PC != cpu->PC2+2)
		{
			SOFT_BREAK
		}
		cpu->pipeLine[slot].opcode=cpu->IF & 0xFFFF;
		cpu->pipeLine[slot].PC = cpu->PC2;
		cpu->IFMask=0;
		return 1;
	}
	if (cpu->PC&0x00000003)
	{
		cpu->IF=SH2_Read_Word(cpu,cpu->PC,0);
		cpu->pipeLine[slot].opcode=cpu->IF &0xFFFF;
		cpu->IFMask=0;
		cpu->pipeLine[slot].PC = cpu->PC;
		if (cpu->pipeLine[slot].Discard==0)
		{
			cpu->PC+=2;
		}
		return 1;
	}
	else
	{
		cpu->IF=SH2_Read_Long(cpu,cpu->PC,0);
		cpu->pipeLine[slot].opcode=cpu->IF>>16;
		cpu->pipeLine[slot].PC = cpu->PC;
		cpu->PC2 = cpu->PC+2;
		if (cpu->pipeLine[slot].Discard==0)
		{
			cpu->IFMask=0xFFFF;
			cpu->PC+=4;
		}
		return 1;
	}

	return 0;				/* should return 0 if memory hazard prevented access */
}

U32 SH2_InstructionDecode(SH2_State* cpu,U32 slot)
{
	U32 a;

	if (cpu->pipeLine[slot].Discard)
	{
		cpu->pipeLine[slot].Discard=0;
		return 0;
	}
	if (cpu->pipeLine[slot].Delay)
	{
		cpu->pipeLine[slot].Delay--;
		return 1;
	}
	if (slot!=0)			/* Pipelining is broken at the moment, this forces single instruction at a time through pipe */
		return 1;
				
	if (SH2_Information[cpu->pipeLine[slot].opcode])
	{
		if (BPAddress==cpu->pipeLine[slot].PC)
		{
			DEB_PauseEmulation(cpu->pauseMode,"SH2 Breakpoint");
		}

		for (a=0;a<SH2_Information[cpu->pipeLine[slot].opcode]->numOperands;a++)
		{
			cpu->pipeLine[slot].operands[a] = (cpu->pipeLine[slot].opcode & SH2_Information[cpu->pipeLine[slot].opcode]->operandMask[a]) >> 
				SH2_Information[cpu->pipeLine[slot].opcode]->operandShift[a];
		}

		return SH2_JumpTable[cpu->pipeLine[slot].opcode](cpu,cpu->pipeLine[slot].stage,slot,cpu->pipeLine[slot].operands[0],cpu->pipeLine[slot].operands[1],cpu->pipeLine[slot].operands[2],
				cpu->pipeLine[slot].operands[3],cpu->pipeLine[slot].operands[4],cpu->pipeLine[slot].operands[5],cpu->pipeLine[slot].operands[6],cpu->pipeLine[slot].operands[7]);
	}

	DEB_PauseEmulation(cpu->pauseMode,"SH2 Unknown instruction");
	return 1;				/* returns 1 for now for handling of unknown instructions */
}

U32 SH2_InstructionExecute(SH2_State* cpu,U32 slot)
{
	return SH2_JumpTable[cpu->pipeLine[slot].opcode](cpu,cpu->pipeLine[slot].stage,slot,cpu->pipeLine[slot].operands[0],cpu->pipeLine[slot].operands[1],cpu->pipeLine[slot].operands[2],
				cpu->pipeLine[slot].operands[3],cpu->pipeLine[slot].operands[4],cpu->pipeLine[slot].operands[5],cpu->pipeLine[slot].operands[6],cpu->pipeLine[slot].operands[7]);
}

U32 SH2_DMAGetAddress(SH2_State* cpu, int baseRegister)
{
	U32 address;
	address =cpu->IOMemory[baseRegister+0]<<24;
	address|=cpu->IOMemory[baseRegister+1]<<16;
	address|=cpu->IOMemory[baseRegister+2]<<8;
	address|=cpu->IOMemory[baseRegister+3];

	address&=0x0FFFFFFF;			/* Not sure DMA can access cache etc.. so for now, make it cache through always */
	address|=0x20000000;
	
	return address;
}

void SH2_DMAStoreAddress(SH2_State* cpu, U32 address, int baseRegister, int adjust, int adjustment,int src0Mode)
{
	switch (adjust)
	{
	case 0:
		if (src0Mode && (adjustment==16))
			address+=adjustment;
		break;
	case 1:
		address+=adjustment;
		break;
	case 2:
		address-=adjustment;
	default:
		printf("Something gone very wrong\n");
		return;
	}
	cpu->IOMemory[baseRegister+0]=(address>>24)&0xFF;
	cpu->IOMemory[baseRegister+1]=(address>>16)&0xFF;
	cpu->IOMemory[baseRegister+2]=(address>>8)&0xFF;
	cpu->IOMemory[baseRegister+3]=address&0xFF;
}

void SH2_DMADoChannel(SH2_State* cpu,int channel,int regBase)
{
	U8 transferSize;
	U32 srcAddress;
	U32 dstAddress;
	U32 transferCnt;

	/* Channel 1 only for now */
	if (cpu->IOMemory[SH2_IO_CHCR0LL+regBase]&SH2_IO_CHCRLL_DE)
	{
		switch (cpu->dmaPhase[channel])
		{
		case 0:
			/* first call, choose dma size loop and go */
			transferSize =cpu->IOMemory[SH2_IO_CHCR0LH+regBase]&(SH2_IO_CHCRLH_TS1|SH2_IO_CHCRLH_TS0);
			if (transferSize == 0)
			{
				cpu->dmaPhase[channel]=1;
			}
			if (transferSize == SH2_IO_CHCRLH_TS0)
			{
				cpu->dmaPhase[channel]=3;
			}
			if (transferSize == SH2_IO_CHCRLH_TS1)
			{
				cpu->dmaPhase[channel]=5;
			}
			if (transferSize == (SH2_IO_CHCRLH_TS1|SH2_IO_CHCRLH_TS0))
			{
				cpu->dmaPhase[channel]=7;
			}
			break;
		case 1:				/* Read phase of byte transfer */

			if ((cpu->IOMemory[SH2_IO_CHCR0LH+regBase]&SH2_IO_CHCRLH_AR)==0)
			{
				if (cpu->dAckCnt==0)
					return;
			}
			srcAddress = SH2_DMAGetAddress(cpu,SH2_IO_SAR0HH+regBase);
			cpu->dmaTmp[channel]=SH2_Read_Byte(cpu,srcAddress,1);
			SH2_DMAStoreAddress(cpu,srcAddress,SH2_IO_SAR0HH+regBase,(cpu->IOMemory[SH2_IO_CHCR0LH+regBase]&(SH2_IO_CHCRLH_SM1|SH2_IO_CHCRLH_SM0))>>4,1,1);
			cpu->dmaPhase[channel]=2;
			break;
		case 2:				/* Write phase of byte transfer */

			dstAddress = SH2_DMAGetAddress(cpu,SH2_IO_DAR0HH+regBase);
			SH2_Write_Byte(cpu,dstAddress,cpu->dmaTmp[channel]);
			SH2_DMAStoreAddress(cpu,dstAddress,SH2_IO_DAR0HH+regBase,(cpu->IOMemory[SH2_IO_CHCR0LH+regBase]&(SH2_IO_CHCRLH_DM1|SH2_IO_CHCRLH_DM0))>>6,1,0);

			transferCnt=SH2_DMAGetAddress(cpu,SH2_IO_TCR0HH+regBase);
			transferCnt--;
			transferCnt&=0x00FFFFFF;
			SH2_DMAStoreAddress(cpu,transferCnt,SH2_IO_TCR0HH+regBase,0,0,0);

			if (transferCnt==0)
			{
				cpu->IOMemory[SH2_IO_CHCR0LL+regBase]|=SH2_IO_CHCRLL_TE;		/* Set transfer complete */
				cpu->IOMemory[SH2_IO_CHCR0LL+regBase]&=~SH2_IO_CHCRLL_DE;		/* Clear dma enable */
				cpu->dmaPhase[channel]=0;
				break;
			}
			cpu->dmaPhase[channel]=1;
			break;
		case 3:				/* Read phase of word transfer */

			if ((cpu->IOMemory[SH2_IO_CHCR0LH+regBase]&SH2_IO_CHCRLH_AR)==0)
			{
				if (cpu->dAckCnt==0)
					return;
			}

			srcAddress = SH2_DMAGetAddress(cpu,SH2_IO_SAR0HH+regBase);
			cpu->dmaTmp[channel]=SH2_Read_Word(cpu,srcAddress,1);
			SH2_DMAStoreAddress(cpu,srcAddress,SH2_IO_SAR0HH+regBase,(cpu->IOMemory[SH2_IO_CHCR0LH+regBase]&(SH2_IO_CHCRLH_SM1|SH2_IO_CHCRLH_SM0))>>4,2,1);
			cpu->dmaPhase[channel]=4;
			break;
		case 4:				/* Write phase of word transfer */

			dstAddress = SH2_DMAGetAddress(cpu,SH2_IO_DAR0HH+regBase);
			SH2_Write_Word(cpu,dstAddress,cpu->dmaTmp[channel]);
			SH2_DMAStoreAddress(cpu,dstAddress,SH2_IO_DAR0HH+regBase,(cpu->IOMemory[SH2_IO_CHCR0LH+regBase]&(SH2_IO_CHCRLH_DM1|SH2_IO_CHCRLH_DM0))>>6,2,0);

			transferCnt=SH2_DMAGetAddress(cpu,SH2_IO_TCR0HH+regBase);
			transferCnt--;
			transferCnt&=0x00FFFFFF;
			SH2_DMAStoreAddress(cpu,transferCnt,SH2_IO_TCR0HH+regBase,0,0,0);

			if (transferCnt==0)
			{
				cpu->IOMemory[SH2_IO_CHCR0LL+regBase]|=SH2_IO_CHCRLL_TE;		/* Set transfer complete */
				cpu->IOMemory[SH2_IO_CHCR0LL+regBase]&=~SH2_IO_CHCRLL_DE;		/* Clear dma enable */
				cpu->dmaPhase[channel]=0;
				break;
			}
			cpu->dmaPhase[channel]=3;
			break;
		case 5:				/* Read phase of long transfer */

			if ((cpu->IOMemory[SH2_IO_CHCR0LH+regBase]&SH2_IO_CHCRLH_AR)==0)
			{
				if (cpu->dAckCnt==0)
					return;
			}

			srcAddress = SH2_DMAGetAddress(cpu,SH2_IO_SAR0HH+regBase);
			cpu->dmaTmp[channel]=SH2_Read_Long(cpu,srcAddress,1);
			SH2_DMAStoreAddress(cpu,srcAddress,SH2_IO_SAR0HH+regBase,(cpu->IOMemory[SH2_IO_CHCR0LH+regBase]&(SH2_IO_CHCRLH_SM1|SH2_IO_CHCRLH_SM0))>>4,4,1);
			cpu->dmaPhase[channel]=6;
			break;
		case 6:				/* Write phase of long transfer */

			dstAddress = SH2_DMAGetAddress(cpu,SH2_IO_DAR0HH+regBase);
			SH2_Write_Long(cpu,dstAddress,cpu->dmaTmp[channel]);
			SH2_DMAStoreAddress(cpu,dstAddress,SH2_IO_DAR0HH+regBase,(cpu->IOMemory[SH2_IO_CHCR0LH+regBase]&(SH2_IO_CHCRLH_DM1|SH2_IO_CHCRLH_DM0))>>6,4,0);

			transferCnt=SH2_DMAGetAddress(cpu,SH2_IO_TCR0HH+regBase);
			transferCnt--;
			transferCnt&=0x00FFFFFF;
			SH2_DMAStoreAddress(cpu,transferCnt,SH2_IO_TCR0HH+regBase,0,0,0);

			if (transferCnt==0)
			{
				cpu->IOMemory[SH2_IO_CHCR0LL+regBase]|=SH2_IO_CHCRLL_TE;		/* Set transfer complete */
				cpu->IOMemory[SH2_IO_CHCR0LL+regBase]&=~SH2_IO_CHCRLL_DE;		/* Clear dma enable */
				cpu->dmaPhase[channel]=0;
				break;
			}
			cpu->dmaPhase[channel]=5;
			break;
		}
	}

}

void SH2_DMATick(SH2_State* cpu)
{
	SH2_DMADoChannel(cpu,0,0);
	SH2_DMADoChannel(cpu,1,0x10);
}

void SH2_Step(SH2_State* cpu)
{
	U32 a;
	U32 tStage;
	U32 lastSlot=SH2_PIPELINE_SIZE-1;
	U32 stages[10]={0,0,0,0,0,0,0,0,0,0};

	if (cpu->traceEnabled)
	{
		SH2_ShowTrace(cpu);
	}

	if (cpu->sleeping)
		return;

	SH2_DMATick(cpu);

/* Pipelined cpu - pipelining currently broken */

	a=0;
	while (a<=lastSlot)
	{
		if (stages[cpu->pipeLine[a].stage])
			break;																/* Pipeline stage already attempted this cycle */
		stages[cpu->pipeLine[a].stage]=1;

		tStage=cpu->pipeLine[a].stage;
		switch (cpu->pipeLine[a].stage)
		{
		case 0:																	/* Instruction Fetch */
			tStage = SH2_InstructionFetch(cpu,a);
			break;
		case 1:																	/* Instruction Decode */ 
			tStage = SH2_InstructionDecode(cpu,a);
			break;
		default:
			tStage=SH2_InstructionExecute(cpu,a);
			break;
		}
		
		if (tStage==cpu->pipeLine[a].stage)
		{
			break;															/* this is a simple way to handle pipe line hazard.. but it may lead to cycle counts being off? */
		}
		cpu->pipeLine[a].stage=tStage;
		if (tStage==0 && a!=(SH2_PIPELINE_SIZE-1))
		{
			memmove(&cpu->pipeLine[a],&cpu->pipeLine[a+1],(SH2_PIPELINE_SIZE-1-a)*sizeof(SH2_Slot));
			cpu->pipeLine[SH2_PIPELINE_SIZE-1].stage=0;
			cpu->pipeLine[SH2_PIPELINE_SIZE-1].Delay=0;
			cpu->pipeLine[SH2_PIPELINE_SIZE-1].Discard=0;
			lastSlot--;													/* mark last slot so we don't give it another slot update this go */
		}
		else
		{
			a++;
		}
	}

	if ((cpu->pipeLine[0].NoInterrupts==0) && (cpu->pipeLine[0].stage==1))
	{
		/* CMD INTERRUPT */
#if INTERRUPTS_ENABLED
		if ((cpu->interrupt==9) && (((cpu->SR&(SH2_SR_I0|SH2_SR_I1|SH2_SR_I2|SH2_SR_I3))>>4)<9))
		{
			cpu->R[15]-=4;
			SH2_Write_Long(cpu,cpu->R[15],cpu->SR);
			cpu->R[15]-=4;
			SH2_Write_Long(cpu,cpu->R[15],cpu->pipeLine[0].PC);
			cpu->IFMask=0;
			cpu->PC=SH2_Read_Long(cpu,cpu->VBR+(68*4),1);
			cpu->SR|=9<<4;
			cpu->interrupt=0;
			DEB_PauseEmulation(cpu->pauseMode,"SH2 CMD Interrupt");
		}
#endif
#if INTERRUPTS_ENABLED
		/* H INTERRUPT */
		if ((cpu->interrupt==11) && (((cpu->SR&(SH2_SR_I0|SH2_SR_I1|SH2_SR_I2|SH2_SR_I3))>>4)<11))
		{
			cpu->R[15]-=4;
			SH2_Write_Long(cpu,cpu->R[15],cpu->SR);
			cpu->R[15]-=4;
			SH2_Write_Long(cpu,cpu->R[15],cpu->pipeLine[0].PC);
			cpu->IFMask=0;
			cpu->PC=SH2_Read_Long(cpu,cpu->VBR+(69*4),1);
			cpu->SR|=11<<4;
			cpu->interrupt=0;
			DEB_PauseEmulation(cpu->pauseMode,"SH2 H Interrupt");
		}
#endif
#if INTERRUPTS_ENABLED
		/* V INTERRUPT */
		if ((cpu->interrupt==15) && (((cpu->SR&(SH2_SR_I0|SH2_SR_I1|SH2_SR_I2|SH2_SR_I3))>>4)<15))
		{
			cpu->R[15]-=4;
			SH2_Write_Long(cpu,cpu->R[15],cpu->SR);
			cpu->R[15]-=4;
			SH2_Write_Long(cpu,cpu->R[15],cpu->pipeLine[0].PC);
			cpu->IFMask=0;
			cpu->PC=SH2_Read_Long(cpu,cpu->VBR+(71*4),1);
			cpu->SR|=15<<4;
			cpu->interrupt=0;
			DEB_PauseEmulation(cpu->pauseMode,"SH2 V Interrupt");
		}
#endif
	}
}

SH2_State *SH2_CreateCPU(SH2_READ_BYTE readByteFunction,SH2_WRITE_BYTE writeByteFunction,int pauseMode)
{
	SH2_State* newState = (SH2_State*)malloc(sizeof(SH2_State));

	newState->readByte=readByteFunction;
	newState->writeByte=writeByteFunction;

	SH2_BuildTable();

	newState->pauseMode = pauseMode;
	return newState;
}

void SH2_Reset(SH2_State* cpu)
{
	U32 a;

	SH2_IO_Reset(cpu);																						/* must reset io registers first so cache is disabled */

	cpu->VBR=0;
	cpu->SR=SH2_SR_I0|SH2_SR_I1|SH2_SR_I2|SH2_SR_I3;
	cpu->PC=SH2_Read_Long(cpu,cpu->VBR+0,1);												/*reset vector address*/;
	cpu->R[15]=SH2_Read_Long(cpu,cpu->VBR+4,1);											/*stack vector address*/;

	cpu->IFMask=0;
	cpu->globalRegisterHazards=0;
	for (a=0;a<SH2_PIPELINE_SIZE;a++)
	{
		cpu->pipeLine[a].stage=0;
		cpu->pipeLine[a].Discard=0;
		cpu->pipeLine[a].Delay=0;
		cpu->pipeLine[a].NoInterrupts=0;
	}

	cpu->traceEnabled=0;

	cpu->sleeping=0;
	cpu->interrupt=0;
	cpu->dAckCnt=0;
}

void SH2_Interrupt(SH2_State* cpu,int level)
{
	cpu->interrupt=level;
}

void SH2_SignalDAck(SH2_State* cpu)
{
	cpu->dAckCnt++;
}
