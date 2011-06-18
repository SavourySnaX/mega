/*
 * SH2_ops.h

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

#ifndef __SH2_OPS_H
#define __SH2_OPS_H

U32 SH2_UNKNOWN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);

U32 SH2_MOVL_AT_DISP_PC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_LDC_RM_SR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVL_AT_RM_INC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVL_RM_AT_DISP_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_LDC_RM_GBR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOV_IMMEDIATE_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVW_RM_AT_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVB_AT_DISP_GBR_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_TST_IMMEDIATE_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_BF(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVB_RM_AT_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVL_RM_AT_DEC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_CMPEQ_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVW_AT_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_EXTUW_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_CMPEQ_IMMEDIATE_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_BT(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVW_R0_AT_DISP_GBR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_ADD_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVL_RM_AT_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_ADD_IMMEDIATE_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_SUB_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVL_AT_DISP_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_LDC_RM_VBR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVL_R0_AT_DISP_GBR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_JMP_AT_RM(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_NOP(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVW_AT_DISP_PC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_DT_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_BFS(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_TST_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOV_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_OR_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_SHLL2_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVW_RM_AT_R0_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_SHLL_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_SHLL8_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_SHLR_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_CMPGT_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_EXTUB_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVB_AT_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVB_AT_DISP_RM_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_SHLR2_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_SWAPB_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVB_AT_RM_INC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVW_AT_R0_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_AND_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVB_RM_AT_DEC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVB_RM_AT_R0_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_XOR_IMMEDIATE_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_XOR_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVW_AT_RM_INC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_BRA(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_SHAR_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_ROTCL_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_JSR_AT_RM(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_STSL_PR_AT_DEC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_EXTSW_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_CMPPZ_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_AND_IMMEDIATE_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVL_AT_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_CMPPL_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVB_AT_R0_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MULUW_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_SWAPW_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_XTRCT_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_STS_MACL_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_SHLL16_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_RTS(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVL_AT_R0_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVL_RM_AT_R0_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_LDSL_AT_RM_INC_PR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_DIV0S_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_SUBC_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_DIV1_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_ADDC_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_NEG_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_CMPGE_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVB_R0_AT_DISP_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVL_AT_DISP_GBR_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVB_R0_AT_DISP_GBR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_NOT_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_BSR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVA_AT_DISP_PC_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MULL_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_DIV0U(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_SHLR16_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_OR_IMMEDIATE_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_BTS(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_CMPHI_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MULSW_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVW_AT_DISP_GBR_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_SLEEP(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_STC_VBR_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_STC_SR_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_STC_GBR_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_STS_PR_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_STS_MACH_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_CLRT(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVW_RM_AT_DEC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVW_R0_AT_DISP_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_STCL_GBR_AT_DEC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_CMPHS_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_DMULSL_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVW_AT_DISP_RM_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_SHAL_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_CLRMAC(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MACL_AT_RM_INC_AT_RN_INC(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_LDS_RM_PR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_SHLR8_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_MOVT_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_EXTSB_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_ROTR_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_LDCL_AT_RM_INC_SR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_STSL_SR_AT_DEC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_DMULUL_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_ORB_IMMEDIATE_AT_R0_GBR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_RTE(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_TSTB_IMMEDIATE_AT_R0_GBR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_LDCL_AT_RM_INC_GBR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_NEGC_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_BRAF_RM(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_ROTL_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_SETT(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);
U32 SH2_ROTCR_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8);

#endif/*__SH2_OPS_H*/
