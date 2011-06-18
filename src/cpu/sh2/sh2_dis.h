/*
 * SH2_dis.h

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

#ifndef __SH2_DIS_H
#define __SH2_DIS_H

extern char SH2_mnemonicData[256];

U16 SH2_DIS_UNKNOWN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);

U16 SH2_DIS_MOVL_AT_DISP_PC_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_LDC_RM_SR(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVL_AT_RM_INC_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVL_RM_AT_DISP_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_LDC_RM_GBR(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOV_IMMEDIATE_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVW_RM_AT_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVB_AT_DISP_GBR_R0(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_TST_IMMEDIATE_R0(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_BF(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVB_RM_AT_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVL_RM_AT_DEC_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_CMPEQ_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVW_AT_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_EXTUW_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_CMPEQ_IMMEDIATE_R0(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_BT(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVW_R0_AT_DISP_GBR(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_ADD_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVL_RM_AT_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_ADD_IMMEDIATE_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_SUB_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVL_AT_DISP_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_LDC_RM_VBR(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVL_R0_AT_DISP_GBR(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_JMP_AT_RM(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_NOP(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVW_AT_DISP_PC_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_DT_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_BFS(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_TST_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOV_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_OR_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_SHLL2_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVW_RM_AT_R0_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_SHLL_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_SHLL8_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_SHLR_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_CMPGT_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_EXTUB_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVB_AT_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVB_AT_DISP_RM_R0(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_SHLR2_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_SWAPB_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVB_AT_RM_INC_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVW_AT_R0_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_AND_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVB_RM_AT_DEC_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVB_RM_AT_R0_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_XOR_IMMEDIATE_R0(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_XOR_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVW_AT_RM_INC_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_BRA(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_SHAR_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_ROTCL_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_JSR_AT_RM(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_STSL_PR_AT_DEC_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_EXTSW_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_CMPPZ_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_AND_IMMEDIATE_R0(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVL_AT_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_CMPPL_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVB_AT_R0_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MULUW_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_SWAPW_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_XTRCT_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_STS_MACL_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_SHLL16_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_RTS(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVL_AT_R0_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVL_RM_AT_R0_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_LDSL_AT_RM_INC_PR(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_DIV0S_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_SUBC_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_DIV1_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_ADDC_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_NEG_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_CMPGE_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVB_R0_AT_DISP_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVL_AT_DISP_GBR_R0(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVB_R0_AT_DISP_GBR(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_NOT_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_BSR(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVA_AT_DISP_PC_R0(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MULL_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_DIV0U(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_SHLR16_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_OR_IMMEDIATE_R0(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_BTS(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_CMPHI_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MULSW_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVW_AT_DISP_GBR_R0(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_SLEEP(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_STC_VBR_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_STC_SR_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_STC_GBR_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_STS_PR_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_STS_MACH_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_CLRT(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVW_RM_AT_DEC_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVW_R0_AT_DISP_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_STCL_GBR_AT_DEC_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_CMPHS_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_DMULSL_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVW_AT_DISP_RM_R0(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_SHAL_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_CLRMAC(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MACL_AT_RM_INC_AT_RN_INC(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_LDS_RM_PR(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_SHLR8_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_MOVT_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_EXTSB_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_ROTR_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_LDCL_AT_RM_INC_SR(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_STSL_SR_AT_DEC_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_DMULUL_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_ORB_IMMEDIATE_AT_R0_GBR(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_RTE(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_TSTB_IMMEDIATE_AT_R0_GBR(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_LDCL_AT_RM_INC_GBR(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_NEGC_RM_RN(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U16 SH2_DIS_BRAF_RM(SH2_State* cpu,U32 adr,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);

#endif/*__SH2_DIS_H*/
