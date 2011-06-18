/*
 * SH2_ops.c

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

 Reminder.. take a look at c instructions carry calculation may be wrong

*/

#include <stdio.h>

#include "config.h"
#include "mytypes.h"
#include "platform.h"

#include "sh2.h"
#include "sh2_memory.h"

U32 SH2_CheckRegisterHazard(SH2_State* cpu,U32 slot,U32* reg)
{
	if (slot>0)
	{
		if (cpu->pipeLine[slot-1].RegHazard==reg)
			return 1;
	}

	return 0;
}

/* Because of the pipeline lengths of Multiply - and the use of 2 destination registers this handles the hazzard checking for that */
U32 SH2_CheckGlobalHazard(SH2_State* cpu,U32 hazardMask)
{
	if (cpu->globalRegisterHazards & hazardMask)
	{
		return 1;
	}
	return 0;
}

U32 SH2_BRAF_RM(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		cpu->pipeLine[slot].NoInterrupts=1;
		cpu->pipeLine[slot+1].NoInterrupts=1;
		cpu->pipeLine[slot+1].Delay=1;
		return 2;
	case 2:									/* Execute */
		cpu->PC=cpu->pipeLine[slot].PC + 4 + cpu->R[op1];			/* DANGER OF PROBLEMS IF ROP1 modified in delay slot */
		cpu->IFMask=0;				/* flush cached pc if any */
		return 0;
	}
	return 0;
}

U32 SH2_NEGC_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->pipeLine[slot].EA=0 - cpu->R[op2];
		cpu->R[op1]=cpu->pipeLine[slot].EA;
		if (cpu->SR & SH2_SR_T)
		{
			cpu->R[op1]--;
		}
		if ((0<cpu->pipeLine[slot].EA) || (cpu->pipeLine[slot].EA<cpu->R[op1]))
		{
			cpu->SR|=SH2_SR_T;
		}
		else
		{
			cpu->SR&=~SH2_SR_T;
		}
		return 0;
	}
	return 0;
}

U32 SH2_LDCL_AT_RM_INC_GBR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op1];
		cpu->R[op1]+=4;				/* Need to determine where this store back is performed, here it avoids generating a hazard */
		cpu->pipeLine[slot].RegHazard=&cpu->GBR;
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=SH2_Read_Long(cpu,cpu->pipeLine[slot].EA,1);
		return 4;
	case 4:									/* Write Back */
		cpu->GBR=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_TSTB_IMMEDIATE_AT_R0_GBR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		cpu->pipeLine[slot+1].Delay=2;
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->GBR))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[0]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->GBR + cpu->R[0];
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=(U32)((S8)SH2_Read_Byte(cpu,cpu->pipeLine[slot].EA,1));
		return 4;
	case 4:									/* Ex */
		if (op1&cpu->pipeLine[slot].VAL)
		{
			cpu->SR&=~SH2_SR_T;
		}
		else
		{
			cpu->SR|=SH2_SR_T;
		}
		return 5;
	case 5:									/* Memory Access - Going to assume we do a write back for now */
		SH2_Write_Byte(cpu,cpu->pipeLine[slot].EA,cpu->pipeLine[slot].VAL);
		return 0;
	}
	return 0;
}

U32 SH2_RTE(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	case 1:									/* Instruction Decode */
		cpu->pipeLine[slot].NoInterrupts=1;
		cpu->pipeLine[slot+1].NoInterrupts=1;
		return 2;
	case 2:									/* Execute - Condition true */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[15]))
			return 2;
		cpu->IFMask=0;				/* flush cached pc if any */
		cpu->pipeLine[slot+1].Delay=3;
		return 3;
	case 3:
		cpu->PC=SH2_Read_Long(cpu,cpu->R[15],1);
		cpu->R[15]+=4;
		return 4;
	case 4:
		cpu->SR=SH2_Read_Long(cpu,cpu->R[15],1)&0x0FFF0FFF;
		cpu->R[15]+=4;
		return 0;
	}
	return 0;
}

U32 SH2_ORB_IMMEDIATE_AT_R0_GBR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		cpu->pipeLine[slot+1].Delay=2;
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->GBR))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[0]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->GBR + cpu->R[0];
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=(U32)((S8)SH2_Read_Byte(cpu,cpu->pipeLine[slot].EA,1));
		return 4;
	case 4:									/* Ex */
		cpu->pipeLine[slot].VAL|=op1;
		return 5;
	case 5:									/* Memory Access */
		SH2_Write_Byte(cpu,cpu->pipeLine[slot].EA,cpu->pipeLine[slot].VAL);
		return 0;
	}
	return 0;
}

U32 SH2_DMULUL_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U64	op164,op264,res64;

	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		if (SH2_CheckGlobalHazard(cpu,SH2_HAZARD_MACL|SH2_HAZARD_MACH))
			return 2;
		op164=(U32)cpu->R[op1];
		op264=(U32)cpu->R[op2];
		res64=op164*op264;
		cpu->pipeLine[slot].EA=res64&0xFFFFFFFF;
		cpu->pipeLine[slot].VAL=res64>>32;
		cpu->globalRegisterHazards|=SH2_HAZARD_MACL|SH2_HAZARD_MACH;
		return 3;
	case 3:									/* Memory Access */
		return 4;
	case 4:									/* Memory Access */
		return 5;
	case 5:									/* Multiplier Working */
		return 6;
	case 6:									/* Multiplier Working */
		return 7;
	case 7:									/* Multiplier Working */
		return 8;
	case 8:									/* Multiplier Working */
		cpu->MACL=cpu->pipeLine[slot].EA;
		cpu->MACH=cpu->pipeLine[slot].VAL;
		cpu->globalRegisterHazards&=~(SH2_HAZARD_MACL|SH2_HAZARD_MACH);
		return 0;
	}
	return 0;
}

U32 SH2_STSL_SR_AT_DEC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->R[op1]-=4;				/* Need to determine where this store back is performed, here it avoids generating a hazard */
		cpu->pipeLine[slot].EA=cpu->R[op1];
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		SH2_Write_Long(cpu,cpu->pipeLine[slot].EA,cpu->SR);		/* don't need to check for hazzard, since WB for previous instruction comes first */
		return 0;
	}
	return 0;
}

U32 SH2_LDCL_AT_RM_INC_SR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op1];
		cpu->R[op1]+=4;				/* Need to determine where this store back is performed, here it avoids generating a hazard */
		cpu->pipeLine[slot].RegHazard=&cpu->SR;
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=SH2_Read_Long(cpu,cpu->pipeLine[slot].EA,1);
		return 4;
	case 4:									/* Write Back */
		cpu->SR=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_ROTR_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].RegHazard=NULL;
		if (cpu->R[op1]&0x00000001)
		{
			cpu->SR|=SH2_SR_T;
			cpu->pipeLine[slot].EA=0x80000000;
		}
		else
		{
			cpu->SR&=~SH2_SR_T;
			cpu->pipeLine[slot].EA=0x00000000;
		}
		cpu->R[op1]>>=1;
		cpu->R[op1]&=0x7FFFFFFF;
		cpu->R[op1]|=cpu->pipeLine[slot].EA;
		return 0;
	}
	return 0;
}

U32 SH2_EXTSB_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->R[op1]=(S8)cpu->R[op2];
		return 0;
	}
	return 0;
}

U32 SH2_MOVT_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].RegHazard=NULL;
		if (cpu->SR&SH2_SR_T)
		{
			cpu->R[op1]=1;
		}
		else
		{
			cpu->R[op1]=0;
		}
		return 0;
	}
	return 0;
}

U32 SH2_SHLR8_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].RegHazard=NULL;
		cpu->R[op1]>>=8;
		cpu->R[op1]&=0x00FFFFFF;
		return 0;
	}
	return 0;
}

U32 SH2_LDS_RM_PR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->PR=cpu->R[op1];
		return 0;
	}
	return 0;
}

/* todo handle S bit being set */
U32 SH2_MACL_AT_RM_INC_AT_RN_INC(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	if (cpu->SR&SH2_SR_S)
	{
		DEB_PauseEmulation(cpu->pauseMode,"SH2 MACL Saturation unemulated");
		return 1;
	}

	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		if (SH2_CheckGlobalHazard(cpu,SH2_HAZARD_MACL|SH2_HAZARD_MACH))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op1];
		cpu->R[op1]+=4;				/* Need to determine where this store back is performed, here it avoids generating a hazard */
		cpu->pipeLine[slot].VAL=cpu->R[op2];
		cpu->R[op2]+=4;
		cpu->globalRegisterHazards|=SH2_HAZARD_MACL|SH2_HAZARD_MACH;
		return 3;
	case 3:									/* Memory Access */
		cpu->op164=(S32)SH2_Read_Long(cpu,cpu->pipeLine[slot].EA,1);
		return 4;
	case 4:									/* Memory Access */
		cpu->op264=(S32)SH2_Read_Long(cpu,cpu->pipeLine[slot].VAL,1);
		return 5;
	case 5:									/* Multiplier Working */
		cpu->res64=cpu->op164*cpu->op264;
		return 6;
	case 6:									/* Multiplier Working */
		return 7;
	case 7:									/* Multiplier Working */
		return 8;
	case 8:									/* Multiplier Working */
		cpu->op164=cpu->MACH;
		cpu->op164<<=32;
		cpu->op164|=cpu->MACL;
		cpu->res64+=cpu->op164;
		cpu->MACL=cpu->res64&0xFFFFFFFF;
		cpu->MACH=cpu->res64>>32;
		cpu->globalRegisterHazards&=~(SH2_HAZARD_MACL|SH2_HAZARD_MACH);
		return 0;
	}
	return 0;
}

U32 SH2_CLRMAC(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckGlobalHazard(cpu,SH2_HAZARD_MACL|SH2_HAZARD_MACH))
			return 2;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		cpu->MACH=0;
		cpu->MACL=0;
		return 0;
	}
	return 0;
}

U32 SH2_SHAL_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].RegHazard=NULL;
		if (cpu->R[op1]&0x80000000)
		{
			cpu->SR|=SH2_SR_T;
		}
		else
		{
			cpu->SR&=~SH2_SR_T;
		}
		cpu->R[op1]<<=1;
		cpu->R[op1]&=0xFFFFFFFE;
		return 0;
	}
	return 0;
}

U32 SH2_MOVW_AT_DISP_RM_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op1]+op2*2;
		cpu->pipeLine[slot].RegHazard=&cpu->R[0];
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=(S16)SH2_Read_Word(cpu,cpu->pipeLine[slot].EA,1);
		return 4;
	case 4:									/* Write Back */
		cpu->R[0]=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_DMULSL_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	S64	op164,op264,res64;

	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		if (SH2_CheckGlobalHazard(cpu,SH2_HAZARD_MACL|SH2_HAZARD_MACH))
			return 2;
		op164=(S32)cpu->R[op1];
		op264=(S32)cpu->R[op2];
		res64=op164*op264;
		cpu->pipeLine[slot].EA=res64&0xFFFFFFFF;
		cpu->pipeLine[slot].VAL=res64>>32;
		cpu->globalRegisterHazards|=SH2_HAZARD_MACL|SH2_HAZARD_MACH;
		return 3;
	case 3:									/* Memory Access */
		return 4;
	case 4:									/* Memory Access */
		return 5;
	case 5:									/* Multiplier Working */
		return 6;
	case 6:									/* Multiplier Working */
		return 7;
	case 7:									/* Multiplier Working */
		return 8;
	case 8:									/* Multiplier Working */
		cpu->MACL=cpu->pipeLine[slot].EA;
		cpu->MACH=cpu->pipeLine[slot].VAL;
		cpu->globalRegisterHazards&=~(SH2_HAZARD_MACL|SH2_HAZARD_MACH);
		return 0;
	}
	return 0;
}

U32 SH2_CMPHS_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		if (cpu->R[op1]>=cpu->R[op2])
		{
			cpu->SR|=SH2_SR_T;
		}
		else
		{
			cpu->SR&=~SH2_SR_T;
		}
		return 0;
	}
	return 0;
}

U32 SH2_STCL_GBR_AT_DEC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->R[op1]-=4;				/* Need to determine where this store back is performed, here it avoids generating a hazard */
		cpu->pipeLine[slot].EA=cpu->R[op1];
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		SH2_Write_Long(cpu,cpu->pipeLine[slot].EA,cpu->GBR);		/* don't need to check for hazzard, since WB for previous instruction comes first */
		return 0;
	}
	return 0;
}

U32 SH2_MOVW_R0_AT_DISP_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op1]+op2*2;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		SH2_Write_Word(cpu,cpu->pipeLine[slot].EA,cpu->R[0]);		/* don't need to check for hazzard, since WB for previous instruction comes first */
		return 0;
	}
	return 0;
}

U32 SH2_MOVW_RM_AT_DEC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->R[op1]-=2;				/* Need to determine where this store back is performed, here it avoids generating a hazard */
		cpu->pipeLine[slot].EA=cpu->R[op1];
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		SH2_Write_Word(cpu,cpu->pipeLine[slot].EA,cpu->R[op2]);		/* don't need to check for hazzard, since WB for previous instruction comes first */
		return 0;
	}
	return 0;
}

U32 SH2_CLRT(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(slot);
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
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute - Condition true */
		cpu->SR&=~SH2_SR_T;
		return 0;
	}
	return 0;
}

U32 SH2_STS_MACH_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckGlobalHazard(cpu,SH2_HAZARD_MACH))
			return 2;
		cpu->pipeLine[slot].EA=cpu->MACH;
		cpu->pipeLine[slot].RegHazard=&cpu->R[op1];
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=cpu->pipeLine[slot].EA;
		return 4;
	case 4:									/* Write Back */
		cpu->R[op1]=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_STS_PR_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->PR))
			return 2;
		cpu->R[op1]=cpu->PR;
		return 0;
	}
	return 0;
}

U32 SH2_STC_GBR_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->GBR))
			return 2;
		cpu->R[op1]=cpu->GBR;
		return 0;
	}
	return 0;
}

U32 SH2_STC_SR_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->SR))
			return 2;
		cpu->R[op1]=cpu->SR;
		return 0;
	}
	return 0;
}


U32 SH2_STC_VBR_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->VBR))
			return 2;
		cpu->R[op1]=cpu->VBR;
		return 0;
	}
	return 0;
}

/*  -- above not verified -- */

U32 SH2_SLEEP(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(slot);
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
	case 1:									/* Instruction Decode */
		cpu->sleeping=1;
		return 2;
	case 2:									/* Execute - Condition true */
		return 0;
	}
	return 0;
}

U32 SH2_MOVW_AT_DISP_GBR_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->GBR))
			return 2;
		cpu->pipeLine[slot].EA=cpu->GBR + op1*2;
		cpu->pipeLine[slot].RegHazard=&cpu->R[0];
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=(S16)SH2_Read_Word(cpu,cpu->pipeLine[slot].EA,1);
		return 4;
	case 4:									/* Write Back */
		cpu->R[0]=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_MULSW_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		if (SH2_CheckGlobalHazard(cpu,SH2_HAZARD_MACL))
			return 2;
		cpu->pipeLine[slot].EA=(U32)((S16)cpu->R[op2]) * (U32)((S16)cpu->R[op1]);
		cpu->globalRegisterHazards|=SH2_HAZARD_MACL;
		return 3;
	case 3:									/* Memory Access */
		return 4;
	case 4:									/* Multiplier Working */
		return 5;
	case 5:									/* Multiplier Working */
		cpu->MACL=cpu->pipeLine[slot].EA;
		cpu->globalRegisterHazards&=~SH2_HAZARD_MACL;
		return 0;
	}
	return 0;
}

U32 SH2_CMPHI_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		if (cpu->R[op1]>cpu->R[op2])
		{
			cpu->SR|=SH2_SR_T;
		}
		else
		{
			cpu->SR&=~SH2_SR_T;
		}
		return 0;
	}
	return 0;
}

U32 SH2_BTS(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		cpu->pipeLine[slot].NoInterrupts=1;
		cpu->pipeLine[slot+1].NoInterrupts=1;
		if ((cpu->SR & SH2_SR_T)==SH2_SR_T)
		{
			cpu->pipeLine[slot+1].Delay=1;
			return 2;
		}
		return 3;
	case 2:									/* Execute - Condition true */
		cpu->PC=cpu->pipeLine[slot].PC + 4 + ((U32)((S8)op1))*2;
		cpu->IFMask=0;				/* flush cached pc if any */
		return 0;
	case 3:									/* Execute - Condition false */
		return 0;
	}
	return 0;
}

U32 SH2_OR_IMMEDIATE_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[0]))
			return 2;
		cpu->R[0]|=op1;
		return 0;
	}
	return 0;
}

U32 SH2_SHLR16_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].RegHazard=NULL;
		cpu->R[op1]>>=16;
		cpu->R[op1]&=0x0000FFFF;
		return 0;
	}
	return 0;
}

U32 SH2_DIV0U(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(slot);
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
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute - Condition true */
		cpu->SR&=~(SH2_SR_M|SH2_SR_Q|SH2_SR_T);
		return 0;
	}
	return 0;
}

U32 SH2_MULL_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		if (SH2_CheckGlobalHazard(cpu,SH2_HAZARD_MACL))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op2] * cpu->R[op1];
		cpu->globalRegisterHazards|=SH2_HAZARD_MACL;
		return 3;
	case 3:									/* Memory Access */
		return 4;
	case 4:									/* Multiplier Working */
		return 5;
	case 5:									/* Multiplier Working */
		cpu->MACL=cpu->pipeLine[slot].EA;
		cpu->globalRegisterHazards&=~SH2_HAZARD_MACL;
		return 0;
	}
	return 0;
}

U32 SH2_MOVA_AT_DISP_PC_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[0]))
			return 2;
		cpu->R[0]=(cpu->pipeLine[slot].PC&0xFFFFFFFC) + 4 + (op1*4);
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_BSR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		cpu->pipeLine[slot].NoInterrupts=1;
		cpu->pipeLine[slot+1].NoInterrupts=1;
		cpu->pipeLine[slot+1].Delay=1;
		return 2;
	case 2:									/* Execute */
		if (op1&0x800)
		{
			op1|=0xF000;
		}
		cpu->PR=cpu->pipeLine[slot].PC + 4;
		cpu->PC=cpu->pipeLine[slot].PC + 4 + ((U32)((S16)op1))*2;
		cpu->IFMask=0;				/* flush cached pc if any */
		return 0;
	}
	return 0;
}

U32 SH2_NOT_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->R[op1]=~cpu->R[op2];
		return 0;
	}
	return 0;
}

U32 SH2_MOVB_R0_AT_DISP_GBR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->GBR))
			return 2;
		cpu->pipeLine[slot].EA=cpu->GBR + op1;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		SH2_Write_Byte(cpu,cpu->pipeLine[slot].EA,cpu->R[0]);			/* don't need to check for hazzard, since WB for previous instruction comes first */
		return 0;
	}
	return 0;
}

U32 SH2_MOVL_AT_DISP_GBR_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->GBR))
			return 2;
		cpu->pipeLine[slot].EA=cpu->GBR + op1*4;
		cpu->pipeLine[slot].RegHazard=&cpu->R[0];
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=SH2_Read_Long(cpu,cpu->pipeLine[slot].EA,1);
		return 4;
	case 4:									/* Write Back */
		cpu->R[0]=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_MOVB_R0_AT_DISP_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op1]+op2;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		SH2_Write_Byte(cpu,cpu->pipeLine[slot].EA,cpu->R[0]);		/* don't need to check for hazzard, since WB for previous instruction comes first */
		return 0;
	}
	return 0;
}

U32 SH2_CMPGE_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		if (((S32)cpu->R[op1])>=((S32)cpu->R[op2]))
		{
			cpu->SR|=SH2_SR_T;
		}
		else
		{
			cpu->SR&=~SH2_SR_T;
		}
		return 0;
	}
	return 0;
}

U32 SH2_NEG_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->R[op1]=0-cpu->R[op2];
		return 0;
	}
	return 0;
}

U32 SH2_ADDC_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op1] + cpu->R[op2];
		cpu->pipeLine[slot].VAL=cpu->R[op1];
		cpu->R[op1]=cpu->pipeLine[slot].EA;
		if (cpu->SR & SH2_SR_T)
		{
			cpu->R[op1]++;
		}
		if ((cpu->pipeLine[slot].VAL>cpu->pipeLine[slot].EA) || (cpu->pipeLine[slot].EA>cpu->R[op1]))
		{
			cpu->SR|=SH2_SR_T;
		}
		else
		{
			cpu->SR&=~SH2_SR_T;
		}
		return 0;
	}
	return 0;
}

U32 SH2_DIV1_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 Q;
	U32 tmp0;
	U32 tmp1;

	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;

		Q=(0x80000000 & cpu->R[op1])>>31;
		cpu->R[op1]<<=1;
		cpu->R[op1]|=(cpu->SR & SH2_SR_T);		/* T bit is in LSB so this works */

		if (cpu->SR & SH2_SR_Q)
		{
			tmp0=cpu->R[op1];
			if (cpu->SR & SH2_SR_M)
			{
				cpu->R[op1]-=cpu->R[op2];
				tmp1=(cpu->R[op1]>tmp0);
				if (Q)
				{
					Q=tmp1;
				}
				else
				{
					Q=(tmp1==0);
				}
			}
			else
			{
				cpu->R[op1]+=cpu->R[op2];
				tmp1=(cpu->R[op1]<tmp0);
				if (Q)
				{
					Q=(tmp1==0);
				}
				else
				{
					Q=tmp1;
				}
			}
		}
		else
		{
			tmp0=cpu->R[op1];
			if (cpu->SR & SH2_SR_M)
			{
				cpu->R[op1]+=cpu->R[op2];
				tmp1=(cpu->R[op1]<tmp0);
				if (Q)
				{
					Q=tmp1;
				}
				else
				{
					Q=(tmp1==0);
				}
			}
			else
			{
				cpu->R[op1]-=cpu->R[op2];
				tmp1=(cpu->R[op1]>tmp0);
				if (Q)
				{
					Q=(tmp1==0);
				}
				else
				{
					Q=tmp1;
				}
			}
		}

		if (Q)
		{
			cpu->SR|=SH2_SR_Q;
		}
		else
		{
			cpu->SR&=~SH2_SR_Q;
		}

		cpu->pipeLine[slot].VAL=((cpu->SR & SH2_SR_Q)==SH2_SR_Q);
		cpu->pipeLine[slot].EA=((cpu->SR & SH2_SR_M)==SH2_SR_M);
		
		if (cpu->pipeLine[slot].EA ^ cpu->pipeLine[slot].VAL)
		{
			cpu->SR&=~SH2_SR_T;
		}
		else
		{
			cpu->SR|=SH2_SR_T;
		}
		return 0;
	}
	return 0;
}

U32 SH2_SUBC_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op1] - cpu->R[op2];
		cpu->pipeLine[slot].VAL=cpu->R[op1];
		cpu->R[op1]=cpu->pipeLine[slot].EA;
		if (cpu->SR & SH2_SR_T)
		{
			cpu->R[op1]--;
		}
		if ((cpu->pipeLine[slot].VAL<cpu->pipeLine[slot].EA) || (cpu->pipeLine[slot].EA<cpu->R[op1]))
		{
			cpu->SR|=SH2_SR_T;
		}
		else
		{
			cpu->SR&=~SH2_SR_T;
		}
		return 0;
	}
	return 0;
}

U32 SH2_DIV0S_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;

		cpu->pipeLine[slot].VAL=cpu->R[op1]&0x80000000;
		cpu->pipeLine[slot].EA=cpu->R[op2]&0x80000000;

		if (cpu->pipeLine[slot].VAL)
		{
			cpu->SR|=SH2_SR_Q;
		}
		else
		{
			cpu->SR&=~SH2_SR_Q;
		}
		if (cpu->pipeLine[slot].EA)
		{
			cpu->SR|=SH2_SR_M;
		}
		else
		{
			cpu->SR&=~SH2_SR_M;
		}
		if (cpu->pipeLine[slot].EA ^ cpu->pipeLine[slot].VAL)
		{
			cpu->SR|=SH2_SR_T;
		}
		else
		{
			cpu->SR&=~SH2_SR_T;
		}
		return 0;
	}
	return 0;
}

U32 SH2_LDSL_AT_RM_INC_PR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op1];
		cpu->R[op1]+=4;				/* Need to determine where this store back is performed, here it avoids generating a hazard */
		cpu->pipeLine[slot].RegHazard=&cpu->PR;
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=SH2_Read_Long(cpu,cpu->pipeLine[slot].EA,1);
		return 4;
	case 4:									/* Write Back */
		cpu->PR=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_MOVL_RM_AT_R0_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[0]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op1]+cpu->R[0];
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		SH2_Write_Long(cpu,cpu->pipeLine[slot].EA,cpu->R[op2]);		/* don't need to check for hazzard, since WB for previous instruction comes first */
		return 0;
	}
	return 0;
}

U32 SH2_MOVL_AT_R0_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[0]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op2]+cpu->R[0];
		cpu->pipeLine[slot].RegHazard=&cpu->R[op1];
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=SH2_Read_Long(cpu,cpu->pipeLine[slot].EA,1);
		return 4;
	case 4:									/* Write Back */
		cpu->R[op1]=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_RTS(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	case 1:									/* Instruction Decode */
		cpu->pipeLine[slot].NoInterrupts=1;
		cpu->pipeLine[slot+1].NoInterrupts=1;
		return 2;
	case 2:									/* Execute - Condition true */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->PR))
			return 2;
		cpu->PC=cpu->PR;	/* docs say +4 but i have my doubts */
		cpu->IFMask=0;				/* flush cached pc if any */
		cpu->pipeLine[slot+1].Delay=1;
		return 0;
	}
	return 0;
}

U32 SH2_SHLL16_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].RegHazard=NULL;
		cpu->R[op1]<<=16;
		return 0;
	}
	return 0;
}

U32 SH2_STS_MACL_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckGlobalHazard(cpu,SH2_HAZARD_MACL))
			return 2;
		cpu->pipeLine[slot].EA=cpu->MACL;
		cpu->pipeLine[slot].RegHazard=&cpu->R[op1];
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=cpu->pipeLine[slot].EA;
		return 4;
	case 4:									/* Write Back */
		cpu->R[op1]=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_XTRCT_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op1]>>16;
		cpu->R[op1]=cpu->R[op2]<<16;
		cpu->R[op1]|=cpu->pipeLine[slot].EA;
		return 0;
	}
	return 0;
}

U32 SH2_SWAPW_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op2];
		cpu->R[op1]=(cpu->pipeLine[slot].EA&0xFFFF0000)>>16;
		cpu->R[op1]|=(cpu->pipeLine[slot].EA&0x0000FFFF)<<16;
		return 0;
	}
	return 0;
}

U32 SH2_MULUW_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		if (SH2_CheckGlobalHazard(cpu,SH2_HAZARD_MACL))
			return 2;
		cpu->pipeLine[slot].EA=(U32)((U16)cpu->R[op2]) * (U32)((U16)cpu->R[op1]);
		cpu->globalRegisterHazards|=SH2_HAZARD_MACL;
		return 3;
	case 3:									/* Memory Access */
		return 4;
	case 4:									/* Multiplier Working */
		return 5;
	case 5:									/* Multiplier Working */
		cpu->MACL=cpu->pipeLine[slot].EA;
		cpu->globalRegisterHazards&=~SH2_HAZARD_MACL;
		return 0;
	}
	return 0;
}


U32 SH2_MOVB_AT_R0_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[0]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op2]+cpu->R[0];
		cpu->pipeLine[slot].RegHazard=&cpu->R[op1];
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=(S8)SH2_Read_Byte(cpu,cpu->pipeLine[slot].EA,1);
		return 4;
	case 4:									/* Write Back */
		cpu->R[op1]=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_CMPPL_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (((S32)cpu->R[op1])>0)
		{
			cpu->SR|=SH2_SR_T;
		}
		else
		{
			cpu->SR&=~SH2_SR_T;
		}
		return 0;
	}
	return 0;
}

U32 SH2_MOVL_AT_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op2];
		cpu->pipeLine[slot].RegHazard=&cpu->R[op1];
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=SH2_Read_Long(cpu,cpu->pipeLine[slot].EA,1);
		return 4;
	case 4:									/* Write Back */
		cpu->R[op1]=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_AND_IMMEDIATE_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[0]))
			return 2;
		cpu->R[0]&=op1;
		return 0;
	}
	return 0;
}

U32 SH2_CMPPZ_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (((S32)cpu->R[op1])>=0)
		{
			cpu->SR|=SH2_SR_T;
		}
		else
		{
			cpu->SR&=~SH2_SR_T;
		}
		return 0;
	}
	return 0;
}

U32 SH2_EXTSW_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->R[op1]=(S16)cpu->R[op2];
		return 0;
	}
	return 0;
}

U32 SH2_STSL_PR_AT_DEC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->R[op1]-=4;				/* Need to determine where this store back is performed, here it avoids generating a hazard */
		cpu->pipeLine[slot].EA=cpu->R[op1];
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		SH2_Write_Long(cpu,cpu->pipeLine[slot].EA,cpu->PR);		/* don't need to check for hazzard, since WB for previous instruction comes first */
		return 0;
	}
	return 0;
}

U32 SH2_JSR_AT_RM(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		cpu->pipeLine[slot].NoInterrupts=1;
		cpu->pipeLine[slot+1].NoInterrupts=1;
		return 2;
	case 2:									/* Execute - Condition true */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->PR=cpu->pipeLine[slot].PC + 4;
		cpu->PC=cpu->R[op1];	/* docs say +4 but i have my doubts */
		cpu->IFMask=0;				/* flush cached pc if any */
		cpu->pipeLine[slot+1].Delay=1;
		return 0;
	}
	return 0;
}

U32 SH2_ROTCL_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].RegHazard=NULL;
		if (cpu->SR&SH2_SR_T)
		{
			cpu->pipeLine[slot].EA=0x00000001;
		}
		else
		{
			cpu->pipeLine[slot].EA=0x00000000;
		}
		if (cpu->R[op1]&0x80000000)
		{
			cpu->SR|=SH2_SR_T;
		}
		else
		{
			cpu->SR&=~SH2_SR_T;
		}
		cpu->R[op1]<<=1;
		cpu->R[op1]&=0xFFFFFFFE;
		cpu->R[op1]|=cpu->pipeLine[slot].EA;
		return 0;
	}
	return 0;
}

U32 SH2_SHAR_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].RegHazard=NULL;
		if (cpu->R[op1]&0x00000001)
		{
			cpu->SR|=SH2_SR_T;
		}
		else
		{
			cpu->SR&=~SH2_SR_T;
		}
		cpu->R[op1]>>=1;
		if (cpu->R[op1]&0x40000000)
		{
			cpu->R[op1]|=0x80000000;
		}
		return 0;
	}
	return 0;
}

U32 SH2_BRA(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		cpu->pipeLine[slot].NoInterrupts=1;
		cpu->pipeLine[slot+1].NoInterrupts=1;
		cpu->pipeLine[slot+1].Delay=1;
		return 2;
	case 2:									/* Execute */
		if (op1&0x800)
		{
			op1|=0xF000;
		}
		cpu->PC=cpu->pipeLine[slot].PC + 4 + ((U32)((S16)op1))*2;
		cpu->IFMask=0;				/* flush cached pc if any */
		return 0;
	}
	return 0;
}

U32 SH2_MOVW_AT_RM_INC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op2];
		cpu->R[op2]+=2;				/* Need to determine where this store back is performed, here it avoids generating a hazard */
		cpu->pipeLine[slot].RegHazard=&cpu->R[op1];
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=(S16)SH2_Read_Word(cpu,cpu->pipeLine[slot].EA,1);
		return 4;
	case 4:									/* Write Back */
		cpu->R[op1]=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_XOR_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->R[op1]^=cpu->R[op2];
		return 0;
	}
	return 0;
}

U32 SH2_XOR_IMMEDIATE_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[0]))
			return 2;
		cpu->R[0]^=op1;
		return 0;
	}
	return 0;
}

U32 SH2_MOVB_RM_AT_R0_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[0]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op1]+cpu->R[0];
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		SH2_Write_Byte(cpu,cpu->pipeLine[slot].EA,cpu->R[op2]);		/* don't need to check for hazzard, since WB for previous instruction comes first */
		return 0;
	}
	return 0;
}

U32 SH2_MOVB_RM_AT_DEC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->R[op1]-=1;				/* Need to determine where this store back is performed, here it avoids generating a hazard */
		cpu->pipeLine[slot].EA=cpu->R[op1];
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		SH2_Write_Byte(cpu,cpu->pipeLine[slot].EA,cpu->R[op2]);		/* don't need to check for hazzard, since WB for previous instruction comes first */
		return 0;
	}
	return 0;
}

U32 SH2_AND_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->R[op1]&=cpu->R[op2];
		return 0;
	}
	return 0;
}

U32 SH2_MOVW_AT_R0_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[0]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op2]+cpu->R[0];
		cpu->pipeLine[slot].RegHazard=&cpu->R[op1];
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=(S16)SH2_Read_Word(cpu,cpu->pipeLine[slot].EA,1);
		return 4;
	case 4:									/* Write Back */
		cpu->R[op1]=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_MOVB_AT_RM_INC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op2];
		cpu->R[op2]+=1;				/* Need to determine where this store back is performed, here it avoids generating a hazard */
		cpu->pipeLine[slot].RegHazard=&cpu->R[op1];
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=(S8)SH2_Read_Byte(cpu,cpu->pipeLine[slot].EA,1);
		return 4;
	case 4:									/* Write Back */
		cpu->R[op1]=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_SWAPB_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op2];
		cpu->R[op1]=cpu->pipeLine[slot].EA&0xFFFF0000;
		cpu->R[op1]|=(cpu->pipeLine[slot].EA&0x0000FF00)>>8;
		cpu->R[op1]|=(cpu->pipeLine[slot].EA&0x000000FF)<<8;
		return 0;
	}
	return 0;
}

U32 SH2_SHLR2_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].RegHazard=NULL;
		cpu->R[op1]>>=2;
		cpu->R[op1]&=0x3FFFFFFF;
		return 0;
	}
	return 0;
}

U32 SH2_MOVB_AT_DISP_RM_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op1]+op2;
		cpu->pipeLine[slot].RegHazard=&cpu->R[0];
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=(S8)SH2_Read_Byte(cpu,cpu->pipeLine[slot].EA,1);
		return 4;
	case 4:									/* Write Back */
		cpu->R[0]=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_MOVB_AT_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op2];
		cpu->pipeLine[slot].RegHazard=&cpu->R[op1];
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=(S8)SH2_Read_Byte(cpu,cpu->pipeLine[slot].EA,1);
		return 4;
	case 4:									/* Write Back */
		cpu->R[op1]=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_EXTUB_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->R[op1]=cpu->R[op2]&0x000000FF;
		return 0;
	}
	return 0;
}

U32 SH2_CMPGT_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		if (((S32)cpu->R[op1])>((S32)cpu->R[op2]))
		{
			cpu->SR|=SH2_SR_T;
		}
		else
		{
			cpu->SR&=~SH2_SR_T;
		}
		return 0;
	}
	return 0;
}

U32 SH2_SHLR_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].RegHazard=NULL;
		if (cpu->R[op1]&0x00000001)
		{
			cpu->SR|=SH2_SR_T;
		}
		else
		{
			cpu->SR&=~SH2_SR_T;
		}
		cpu->R[op1]>>=1;
		cpu->R[op1]&=0x7FFFFFFF;
		return 0;
	}
	return 0;
}

U32 SH2_SHLL8_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].RegHazard=NULL;
		cpu->R[op1]<<=8;
		return 0;
	}
	return 0;
}

U32 SH2_SHLL_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].RegHazard=NULL;
		if (cpu->R[op1]&0x80000000)
		{
			cpu->SR|=SH2_SR_T;
		}
		else
		{
			cpu->SR&=~SH2_SR_T;
		}
		cpu->R[op1]<<=1;
		return 0;
	}
	return 0;
}

U32 SH2_MOVW_RM_AT_R0_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[0]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op1]+cpu->R[0];
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		SH2_Write_Word(cpu,cpu->pipeLine[slot].EA,cpu->R[op2]);		/* don't need to check for hazzard, since WB for previous instruction comes first */
		return 0;
	}
	return 0;
}

U32 SH2_SHLL2_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].RegHazard=NULL;
		cpu->R[op1]<<=2;
		return 0;
	}
	return 0;
}

U32 SH2_OR_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->R[op1]|=cpu->R[op2];
		return 0;
	}
	return 0;
}

U32 SH2_MOV_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->R[op1]=cpu->R[op2];
		return 0;
	}
	return 0;
}

U32 SH2_TST_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		if (cpu->R[op2]&cpu->R[op1])
		{
			cpu->SR&=~SH2_SR_T;
		}
		else
		{
			cpu->SR|=SH2_SR_T;
		}
		return 0;
	}
	return 0;
}

U32 SH2_BFS(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		cpu->pipeLine[slot].NoInterrupts=1;
		cpu->pipeLine[slot+1].NoInterrupts=1;
		if ((cpu->SR & SH2_SR_T)==0)
		{
			cpu->pipeLine[slot+1].Delay=1;
			return 2;
		}
		return 3;
	case 2:									/* Execute - Condition true */
		cpu->PC=cpu->pipeLine[slot].PC + 4 + ((U32)((S8)op1))*2;
		cpu->IFMask=0;				/* flush cached pc if any */
		return 0;
	case 3:									/* Execute - Condition false */
		return 0;
	}
	return 0;
}

U32 SH2_DT_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].RegHazard=NULL;
		cpu->R[op1]--;
		if (cpu->R[op1]==0)
		{
			cpu->SR|=SH2_SR_T;
		}
		else
		{
			cpu->SR&=~SH2_SR_T;
		}
		return 0;
	}
	return 0;
}

U32 SH2_MOVW_AT_DISP_PC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		cpu->pipeLine[slot].EA=cpu->pipeLine[slot].PC + 4 + (op2*2);
		cpu->pipeLine[slot].RegHazard=&cpu->R[op1];
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=(S16)SH2_Read_Word(cpu,cpu->pipeLine[slot].EA,1);
		return 4;
	case 4:									/* Write Back */
		cpu->R[op1]=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_NOP(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(cpu);
	UNUSED_ARGUMENT(slot);
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
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute - Condition true */
		return 0;
	}
	return 0;
}

U32 SH2_JMP_AT_RM(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		cpu->pipeLine[slot].NoInterrupts=1;
		cpu->pipeLine[slot+1].NoInterrupts=1;
		return 2;
	case 2:									/* Execute - Condition true */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->PC=cpu->R[op1];	/* docs say +4 but i have my doubts */
		cpu->IFMask=0;				/* flush cached pc if any */
		cpu->pipeLine[slot+1].Delay=1;
		return 0;
	}
	return 0;
}

U32 SH2_MOVL_R0_AT_DISP_GBR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->GBR))
			return 2;
		cpu->pipeLine[slot].EA=cpu->GBR + op1*4;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		SH2_Write_Long(cpu,cpu->pipeLine[slot].EA,cpu->R[0]);			/* don't need to check for hazzard, since WB for previous instruction comes first */
		return 0;
	}
	return 0;
}

U32 SH2_LDC_RM_VBR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].RegHazard=NULL;
		cpu->VBR = cpu->R[op1];
		return 0;
	}
	return 0;
}

U32 SH2_MOVL_AT_DISP_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op2] + (op3*4);
		cpu->pipeLine[slot].RegHazard=&cpu->R[op1];
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=SH2_Read_Long(cpu,cpu->pipeLine[slot].EA,1);
		return 4;
	case 4:									/* Write Back */
		cpu->R[op1]=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_SUB_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->R[op1]-=cpu->R[op2];
		return 0;
	}
	return 0;
}

U32 SH2_ADD_IMMEDIATE_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->R[op1]+=(U32)((S8)op2);
		return 0;
	}
	return 0;
}

U32 SH2_MOVL_RM_AT_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op1];
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		SH2_Write_Long(cpu,cpu->pipeLine[slot].EA,cpu->R[op2]);		/* don't need to check for hazzard, since WB for previous instruction comes first */
		return 0;
	}
	return 0;
}

U32 SH2_ADD_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->R[op1]+=cpu->R[op2];
		return 0;
	}
	return 0;
}

U32 SH2_MOVW_R0_AT_DISP_GBR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->GBR))
			return 2;
		cpu->pipeLine[slot].EA=cpu->GBR + op1*2;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		SH2_Write_Word(cpu,cpu->pipeLine[slot].EA,cpu->R[0]);			/* don't need to check for hazzard, since WB for previous instruction comes first */
		return 0;
	}
	return 0;
}

U32 SH2_BT(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		if ((cpu->SR & SH2_SR_T)==SH2_SR_T)
		{
			cpu->pipeLine[slot+1].Discard=1;
			cpu->pipeLine[slot+2].Discard=1;
			return 2;
		}
		return 3;
	case 2:									/* Execute - Condition true */
		cpu->PC=cpu->pipeLine[slot].PC + 4 + ((U32)((S8)op1))*2;
		cpu->IFMask=0;				/* flush cached pc if any */
		return 0;
	case 3:									/* Execute - Condition false */
		return 0;
	}
	return 0;
}

U32 SH2_CMPEQ_IMMEDIATE_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[0]))
			return 2;
		if (cpu->R[0]==(U32)((S8)op1))
		{
			cpu->SR|=SH2_SR_T;
		}
		else
		{
			cpu->SR&=~SH2_SR_T;
		}
		return 0;
	}
	return 0;
}

U32 SH2_EXTUW_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->R[op1]=cpu->R[op2]&0x0000FFFF;
		return 0;
	}
	return 0;
}

U32 SH2_MOVW_AT_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op2];
		cpu->pipeLine[slot].RegHazard=&cpu->R[op1];
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=(S16)SH2_Read_Word(cpu,cpu->pipeLine[slot].EA,1);
		return 4;
	case 4:									/* Write Back */
		cpu->R[op1]=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_CMPEQ_RM_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		if (cpu->R[op1]==cpu->R[op2])
		{
			cpu->SR|=SH2_SR_T;
		}
		else
		{
			cpu->SR&=~SH2_SR_T;
		}
		return 0;
	}
	return 0;
}

U32 SH2_MOVL_RM_AT_DEC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->R[op1]-=4;				/* Need to determine where this store back is performed, here it avoids generating a hazard */
		cpu->pipeLine[slot].EA=cpu->R[op1];
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		SH2_Write_Long(cpu,cpu->pipeLine[slot].EA,cpu->R[op2]);		/* don't need to check for hazzard, since WB for previous instruction comes first */
		return 0;
	}
	return 0;
}

U32 SH2_MOVB_RM_AT_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op1];
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		SH2_Write_Byte(cpu,cpu->pipeLine[slot].EA,cpu->R[op2]);		/* don't need to check for hazzard, since WB for previous instruction comes first */
		return 0;
	}
	return 0;
}

U32 SH2_BF(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		if ((cpu->SR & SH2_SR_T)==0)
		{
			cpu->pipeLine[slot+1].Discard=1;
			cpu->pipeLine[slot+2].Discard=1;
			return 2;
		}
		return 3;
	case 2:									/* Execute - Condition true */
		cpu->PC=cpu->pipeLine[slot].PC + 4 + ((U32)((S8)op1))*2;
		cpu->IFMask=0;				/* flush cached pc if any */
		return 0;
	case 3:									/* Execute - Condition false */
		return 0;
	}
	return 0;
}

U32 SH2_TST_IMMEDIATE_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[0]))
			return 2;
		if (cpu->R[0]&op1)
		{
			cpu->SR&=~SH2_SR_T;
		}
		else
		{
			cpu->SR|=SH2_SR_T;
		}
		return 0;
	}
	return 0;
}

U32 SH2_MOVB_AT_DISP_GBR_R0(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->GBR))
			return 2;
		cpu->pipeLine[slot].EA=cpu->GBR + op1;
		cpu->pipeLine[slot].RegHazard=&cpu->R[0];
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=(U32)((S8)SH2_Read_Byte(cpu,cpu->pipeLine[slot].EA,1));
		return 4;
	case 4:									/* Write Back */
		cpu->R[0]=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_MOVW_RM_AT_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op1];
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		SH2_Write_Word(cpu,cpu->pipeLine[slot].EA,cpu->R[op2]);		/* don't need to check for hazzard, since WB for previous instruction comes first */
		return 0;
	}
	return 0;
}

U32 SH2_MOV_IMMEDIATE_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->R[op1]=(U32)((S8)op2);
		return 0;
	}
	return 0;
}

U32 SH2_LDC_RM_GBR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].RegHazard=NULL;
		cpu->GBR = cpu->R[op1];
		return 0;
	}
	return 0;
}

U32 SH2_MOVL_RM_AT_DISP_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op1] + (op3*4);
		cpu->pipeLine[slot].RegHazard=NULL;
		return 3;
	case 3:									/* Memory Access */
		SH2_Write_Long(cpu,cpu->pipeLine[slot].EA,cpu->R[op2]);		/* don't need to check for hazzard, since WB for previous instruction comes first */
		return 0;
	}
	return 0;
}


U32 SH2_MOVL_AT_RM_INC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op2]))
			return 2;
		cpu->pipeLine[slot].EA=cpu->R[op2];
		cpu->R[op2]+=4;				/* Need to determine where this store back is performed, here it avoids generating a hazard */
		cpu->pipeLine[slot].RegHazard=&cpu->R[op1];
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=SH2_Read_Long(cpu,cpu->pipeLine[slot].EA,1);
		return 4;
	case 4:									/* Write Back */
		cpu->R[op1]=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}


U32 SH2_LDC_RM_SR(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		if (SH2_CheckRegisterHazard(cpu,slot,&cpu->R[op1]))
			return 2;
		cpu->pipeLine[slot].RegHazard=NULL;
		cpu->SR = cpu->R[op1];
		return 0;
	}
	return 0;
}

U32 SH2_MOVL_AT_DISP_PC_RN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	switch (stage)
	{
	case 1:									/* Instruction Decode */
		return 2;
	case 2:									/* Execute */
		cpu->pipeLine[slot].EA=(cpu->pipeLine[slot].PC&0xFFFFFFFC) + 4 + (op2*4);
		cpu->pipeLine[slot].RegHazard=&cpu->R[op1];
		return 3;
	case 3:									/* Memory Access */
		cpu->pipeLine[slot].VAL=SH2_Read_Long(cpu,cpu->pipeLine[slot].EA,1);
		return 4;
	case 4:									/* Write Back */
		cpu->R[op1]=cpu->pipeLine[slot].VAL;
		cpu->pipeLine[slot].RegHazard=NULL;
		return 0;
	}
	return 0;
}

U32 SH2_UNKNOWN(SH2_State* cpu,U32 stage,U32 slot,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(slot);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	cpu->PC-=2;	/* account for prefetch */
	return 0;
}
