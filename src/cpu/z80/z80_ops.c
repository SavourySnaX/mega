/*
 *  z80_ops.c

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

#include "z80_ops.h"

#include "z80.h"

void Z80_MEM_setByte(U16 address,U8 data);
U8 Z80_MEM_getByte(U16 address);
U8 Z80_IO_getByte(U16 address);
void Z80_IO_setByte(U16 address,U8 data);

int TestFlags(int cc)
{
	switch (cc)
	{
	case 0:
		return (Z80_regs.R[Z80_REG_F] & Z80_STATUS_Z)==0;
	case 1:
		return (Z80_regs.R[Z80_REG_F] & Z80_STATUS_Z)!=0;
	case 2:
		return (Z80_regs.R[Z80_REG_F] & Z80_STATUS_C)==0;
	case 3:
		return (Z80_regs.R[Z80_REG_F] & Z80_STATUS_C)!=0;
	case 4:
		return (Z80_regs.R[Z80_REG_F] & Z80_STATUS_PV)==0;
	case 5:
		return (Z80_regs.R[Z80_REG_F] & Z80_STATUS_PV)!=0;
	case 6:
		return (Z80_regs.R[Z80_REG_F] & Z80_STATUS_S)==0;
	case 7:
		return (Z80_regs.R[Z80_REG_F] & Z80_STATUS_S)!=0;
	}
	return 0;		/* Should not get here */
}

void Compute16Flags(U16 calculateFlags, U16 value)
{
	if (calculateFlags & Z80_STATUS_S)
	{
		if (value&0x8000)
		{
			Z80_regs.R[Z80_REG_F]|=Z80_STATUS_S;
		}
		else
		{
			Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_S;
		}
	}
	if (calculateFlags & Z80_STATUS_Z)
	{
		if (value)
		{
			Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_Z;
		}
		else
		{
			Z80_regs.R[Z80_REG_F]|=Z80_STATUS_Z;
		}
	}
}

/* TODO : N,C,H,V checks */
void ComputeFlags(U16 calculateFlags, U8 value, U8 clearFlags, U8 setFlags)
{
	Z80_regs.R[Z80_REG_F] |= setFlags;
	Z80_regs.R[Z80_REG_F] &= ~clearFlags;

	if (calculateFlags & Z80_STATUS_S)
	{
		if (value&0x80)
		{
			Z80_regs.R[Z80_REG_F]|=Z80_STATUS_S;
		}
		else
		{
			Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_S;
		}
	}
	if (calculateFlags & Z80_STATUS_Z)
	{
		if (value)
		{
			Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_Z;
		}
		else
		{
			Z80_regs.R[Z80_REG_F]|=Z80_STATUS_Z;
		}
	}
	if (calculateFlags & Z80_STATUS_X_B5)
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_X_B5;
		Z80_regs.R[Z80_REG_F]|=value & Z80_STATUS_X_B5;
	}
	if (calculateFlags & Z80_STATUS_X_B3)
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_X_B3;
		Z80_regs.R[Z80_REG_F]|=value & Z80_STATUS_X_B3;
	}
	if (calculateFlags & Z80_STATUS_P)
	{
		U8 parity = value & 0xFF;
		parity^=parity>>4;
		parity&=0xF;
		if ((0x6996 >> parity)&1)
		{
			Z80_regs.R[Z80_REG_F]|=Z80_STATUS_PV;
		}
		else
		{
			Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_PV;
		}
	}
}

void ComputeAddHV(U8 src,U8 dst,U8 res)
{
	U8 srcH = src & 0x08;
	U8 dstH = dst & 0x08;
	U8 resH = res & 0x08;
	U8 srcN = src & 0x80;
	U8 dstN = dst & 0x80;
	U8 resN = res & 0x80;

	if ((srcH & dstH) | ((~resH) & dstH) | (srcH & (~resH)))
	{
		Z80_regs.R[Z80_REG_F]|=(Z80_STATUS_H);
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~(Z80_STATUS_H);
	}

	if ((srcN & dstN & (~resN)) | ((~srcN) & (~dstN) & resN))
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_PV;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_PV;
	}
}

void ComputeSubHV(U8 src,U8 dst,U8 res)
{
	U8 srcH = src & 0x08;
	U8 dstH = dst & 0x08;
	U8 resH = res & 0x08;
	U8 srcN = src & 0x80;
	U8 dstN = dst & 0x80;
	U8 resN = res & 0x80;

	if ((srcH & (~dstH)) | (resH & (~dstH)) | (srcH & resH))
	{
		Z80_regs.R[Z80_REG_F]|=(Z80_STATUS_H);
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~(Z80_STATUS_H);
	}

	if (((~srcN) & dstN & (~resN)) | (srcN & (~dstN) & resN))
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_PV;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_PV;
	}
}

void ComputeAddHCV(U8 src,U8 dst,U8 res)
{
	U8 srcH = src & 0x08;
	U8 dstH = dst & 0x08;
	U8 resH = res & 0x08;
	U8 srcN = src & 0x80;
	U8 dstN = dst & 0x80;
	U8 resN = res & 0x80;

	if ((srcH & dstH) | ((~resH) & dstH) | (srcH & (~resH)))
	{
		Z80_regs.R[Z80_REG_F]|=(Z80_STATUS_H);
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~(Z80_STATUS_H);
	}
	
	if ((srcN & dstN) | ((~resN) & dstN) | (srcN & (~resN)))
	{
		Z80_regs.R[Z80_REG_F]|=(Z80_STATUS_C);
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~(Z80_STATUS_C);
	}

	if ((srcN & dstN & (~resN)) | ((~srcN) & (~dstN) & resN))
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_PV;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_PV;
	}
}

void ComputeSubHCV(U8 src,U8 dst,U8 res)
{
	U8 srcH = src & 0x08;
	U8 dstH = dst & 0x08;
	U8 resH = res & 0x08;
	U8 srcN = src & 0x80;
	U8 dstN = dst & 0x80;
	U8 resN = res & 0x80;

	if ((srcH & (~dstH)) | (resH & (~dstH)) | (srcH & resH))
	{
		Z80_regs.R[Z80_REG_F]|=(Z80_STATUS_H);
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~(Z80_STATUS_H);
	}
	
	if ((srcN & (~dstN)) | (resN & (~dstN)) | (srcN & resN))
	{
		Z80_regs.R[Z80_REG_F]|=(Z80_STATUS_C);
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~(Z80_STATUS_C);
	}

	if (((~srcN) & dstN & (~resN)) | (srcN & (~dstN) & resN))
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_PV;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_PV;
	}
}

void ComputeAdd16HC(U16 src,U16 dst,U16 res)
{
	U16 srcH = src & 0x0800;
	U16 dstH = dst & 0x0800;
	U16 resH = res & 0x0800;
	U16 srcN = src & 0x8000;
	U16 dstN = dst & 0x8000;
	U16 resN = res & 0x8000;

	if ((srcH & dstH) | ((~resH) & dstH) | (srcH & (~resH)))
	{
		Z80_regs.R[Z80_REG_F]|=(Z80_STATUS_H);
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~(Z80_STATUS_H);
	}
	
	if ((srcN & dstN) | ((~resN) & dstN) | (srcN & (~resN)))
	{
		Z80_regs.R[Z80_REG_F]|=(Z80_STATUS_C);
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~(Z80_STATUS_C);
	}
}

void ComputeAdd16HVC(U16 src,U16 dst,U16 res)
{
	U16 srcH = src & 0x0800;
	U16 dstH = dst & 0x0800;
	U16 resH = res & 0x0800;
	U16 srcN = src & 0x8000;
	U16 dstN = dst & 0x8000;
	U16 resN = res & 0x8000;

	if ((srcH & dstH) | ((~resH) & dstH) | (srcH & (~resH)))
	{
		Z80_regs.R[Z80_REG_F]|=(Z80_STATUS_H);
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~(Z80_STATUS_H);
	}
	
	if ((srcN & dstN) | ((~resN) & dstN) | (srcN & (~resN)))
	{
		Z80_regs.R[Z80_REG_F]|=(Z80_STATUS_C);
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~(Z80_STATUS_C);
	}
	
	if ((srcN & dstN & (~resN)) | ((~srcN) & (~dstN) & resN))
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_PV;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_PV;
	}
}

void ComputeSub16HVC(U16 src,U16 dst,U16 res)
{
	U16 srcH = src & 0x0800;
	U16 dstH = dst & 0x0800;
	U16 resH = res & 0x0800;
	U16 srcN = src & 0x8000;
	U16 dstN = dst & 0x8000;
	U16 resN = res & 0x8000;

	if ((srcH & (~dstH)) | (resH & (~dstH)) | (srcH & resH))
	{
		Z80_regs.R[Z80_REG_F]|=(Z80_STATUS_H);
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~(Z80_STATUS_H);
	}
	
	if ((srcN & (~dstN)) | (resN & (~dstN)) | (srcN & resN))
	{
		Z80_regs.R[Z80_REG_F]|=(Z80_STATUS_C);
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~(Z80_STATUS_C);
	}

	if (((~srcN) & dstN & (~resN)) | (srcN & (~dstN) & resN))
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_PV;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_PV;
	}
}

U16 GetRP(U16 op1)
{
	U16 rp;

	switch (op1)
	{
	case 0:
		rp=Z80_regs.R[Z80_REG_B]<<8;
		rp|=Z80_regs.R[Z80_REG_C]&0xFF;
		break;
	case 1:
		rp=Z80_regs.R[Z80_REG_D]<<8;
		rp|=Z80_regs.R[Z80_REG_E]&0xFF;
		break;
	case 2:
		if (Z80_regs.ixAdjust)
		{
			rp = Z80_regs.IX;
			break;
		}
		if (Z80_regs.iyAdjust)
		{
			rp = Z80_regs.IY;
			break;
		}
		rp=Z80_regs.R[Z80_REG_H]<<8;
		rp|=Z80_regs.R[Z80_REG_L]&0xFF;
		break;
	case 3:
		rp = Z80_regs.SP;
		break;
	default:
		DEB_PauseEmulation(DEB_Mode_Z80,"Something gone wrong");
		rp=0;
		break;
	}

	return rp;
}

U16 GetRPAF(U16 op1)
{
	U16 rp;

	switch (op1)
	{
	case 0:
		rp=Z80_regs.R[Z80_REG_B]<<8;
		rp|=Z80_regs.R[Z80_REG_C]&0xFF;
		break;
	case 1:
		rp=Z80_regs.R[Z80_REG_D]<<8;
		rp|=Z80_regs.R[Z80_REG_E]&0xFF;
		break;
	case 2:
		if (Z80_regs.ixAdjust)
		{
			rp = Z80_regs.IX;
			break;
		}
		if (Z80_regs.iyAdjust)
		{
			rp = Z80_regs.IY;
			break;
		}
		rp=Z80_regs.R[Z80_REG_H]<<8;
		rp|=Z80_regs.R[Z80_REG_L]&0xFF;
		break;
	case 3:
		rp=Z80_regs.R[Z80_REG_A]<<8;
		rp|=Z80_regs.R[Z80_REG_F]&0xFF;
		break;
	default:
		DEB_PauseEmulation(DEB_Mode_Z80,"Something gone wrong");
		rp=0;
		break;
	}

	return rp;
}

void SetRP(U16 op1,U8 nH,U8 nL)
{
	switch (op1)
	{
	case 0:
		Z80_regs.R[Z80_REG_B]=nH;
		Z80_regs.R[Z80_REG_C]=nL;
		break;
	case 1:
		Z80_regs.R[Z80_REG_D]=nH;
		Z80_regs.R[Z80_REG_E]=nL;
		break;
	case 2:
		if (Z80_regs.ixAdjust)
		{
			Z80_regs.IX = (nH<<8)|nL;
			break;
		}
		if (Z80_regs.iyAdjust)
		{
			Z80_regs.IY = (nH<<8)|nL;
			break;
		}
		Z80_regs.R[Z80_REG_H]=nH;
		Z80_regs.R[Z80_REG_L]=nL;
		break;
	case 3:
		Z80_regs.SP=(nH<<8) | nL;
		break;
	}
}

void SetRPAF(U16 op1,U8 nH,U8 nL)
{
	switch (op1)
	{
	case 0:
		Z80_regs.R[Z80_REG_B]=nH;
		Z80_regs.R[Z80_REG_C]=nL;
		break;
	case 1:
		Z80_regs.R[Z80_REG_D]=nH;
		Z80_regs.R[Z80_REG_E]=nL;
		break;
	case 2:
		if (Z80_regs.ixAdjust)
		{
			Z80_regs.IX = (nH<<8)|nL;
			break;
		}
		if (Z80_regs.iyAdjust)
		{
			Z80_regs.IY = (nH<<8)|nL;
			break;
		}
		Z80_regs.R[Z80_REG_H]=nH;
		Z80_regs.R[Z80_REG_L]=nL;
		break;
	case 3:
		Z80_regs.R[Z80_REG_A]=nH;
		Z80_regs.R[Z80_REG_F]=nL;
		break;
	}
}

void SetR(U16 op1,int IXIYPossible,U8 value)
{
	if (op1>7)
	{
		DEB_PauseEmulation(DEB_Mode_Z80,"Something gone wrong");
		return;
	}
	if (IXIYPossible && ((op1==4) || (op1==5)))
	{
		if (Z80_regs.ixAdjust)
		{
			if (op1==4)
			{
				Z80_regs.IX&=0x00FF;
				Z80_regs.IX|=(value<<8);
			}
			else
			{
				Z80_regs.IX&=0xFF00;
				Z80_regs.IX|=value;
			}
			return;
		}
		if (Z80_regs.iyAdjust)
		{
			if (op1==4)
			{
				Z80_regs.IY&=0x00FF;
				Z80_regs.IY|=(value<<8);
			}
			else
			{
				Z80_regs.IY&=0xFF00;
				Z80_regs.IY|=value;
			}
			return;
		}
	}
	if (op1!=6)											/* 6 means F (but i only use it when a value should not be written */
	{
		Z80_regs.R[op1]=value;
	}
}

U8 GetR(U16 op1,int IXIYPossible)
{
	if (op1>7)
	{
		DEB_PauseEmulation(DEB_Mode_Z80,"Something gone wrong");
		return 0;
	}

	if (IXIYPossible && ((op1==4) || (op1==5)))
	{
		if (Z80_regs.ixAdjust)
		{
			if (op1==4)
			{
				return Z80_regs.IX>>8;
			}
			else
			{
				return Z80_regs.IX&0xFF;
			}
		}
		if (Z80_regs.iyAdjust)
		{
			if (op1==4)
			{
				return Z80_regs.IY>>8;
			}
			else
			{
				return Z80_regs.IY&0xFF;
			}
		}
	}
	if (op1!=6)											/* 6 means F (but i only use it when 0 should be returned) */
	{
		return Z80_regs.R[op1];
	}
	return 0;
}

U16 Get_HL_(U16 dispAddress)
{
	S16 d;

	if (Z80_regs.ixAdjust)
	{
		d=(S8)Z80_MEM_getByte(dispAddress);
		return Z80_regs.IX + d;
	}
	if (Z80_regs.iyAdjust)
	{
		d=(S8)Z80_MEM_getByte(dispAddress);
		return Z80_regs.IY + d;
	}

	return (Z80_regs.R[Z80_REG_H]<<8) | Z80_regs.R[Z80_REG_L];
}

U16 GetHL()
{
	if (Z80_regs.ixAdjust)
	{
		return Z80_regs.IX;
	}
	if (Z80_regs.iyAdjust)
	{
		return Z80_regs.IY;
	}

	return (Z80_regs.R[Z80_REG_H]<<8) | Z80_regs.R[Z80_REG_L];
}

void SetHL(U16 res)
{
	if (Z80_regs.ixAdjust)
	{
		Z80_regs.IX=res;
		return;
	}
	if (Z80_regs.iyAdjust)
	{
		Z80_regs.IY=res;
		return;
	}

	Z80_regs.R[Z80_REG_H]=res>>8;
	Z80_regs.R[Z80_REG_L]=res&0xFF;
}

extern U32 Z80Cycles;


U32 Z80_DI(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80_regs.IFF1=0;
	Z80_regs.IFF2=0;

	Z80Cycles=4;

	return 0;
}

U32 Z80_LDRP(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 nH,nL;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	nL=Z80_MEM_getByte(Z80_regs.PC);
	nH=Z80_MEM_getByte(Z80_regs.PC+1);

	Z80_regs.PC+=2;

	SetRP(op1,nH,nL);	

	Z80Cycles=10;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}

	return 0;
}

U32 Z80_ED_IM(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	switch(op1)
	{
	case 0:
		Z80_regs.InterruptMode=0;
		break;
	case 2:
		Z80_regs.InterruptMode=1;
		break;
	case 3:
		Z80_regs.InterruptMode=2;
		break;
	}

	Z80Cycles=8;

	return 0;
}

U32 Z80_JP(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 nH,nL;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	nL=Z80_MEM_getByte(Z80_regs.PC);
	nH=Z80_MEM_getByte(Z80_regs.PC+1);

	Z80_regs.PC=(nH<<8) | nL;

	Z80Cycles=10;

	return 0;
}

U32 Z80_CALL(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 nH,nL;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	nL=Z80_MEM_getByte(Z80_regs.PC);
	nH=Z80_MEM_getByte(Z80_regs.PC+1);

	Z80_regs.PC+=2;
	
	Z80_regs.SP--;
	Z80_MEM_setByte(Z80_regs.SP,Z80_regs.PC>>8);
	Z80_regs.SP--;
	Z80_MEM_setByte(Z80_regs.SP,Z80_regs.PC&0xFF);
	
	Z80_regs.PC=(nH<<8) | nL;

	Z80Cycles=17;

	return 0;
}

U32 Z80_LDn(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 n;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	n=Z80_MEM_getByte(Z80_regs.PC);

	Z80_regs.PC+=1;

	SetR(op1,1,n);

	Z80Cycles=7;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}

	return 0;
}

U32 Z80_XOR(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80_regs.R[Z80_REG_A] ^= GetR(op1,1);

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,Z80_regs.R[Z80_REG_A],Z80_STATUS_C|Z80_STATUS_H|Z80_STATUS_N,0);

	Z80Cycles=4;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}

	return 0;
}

/* Flags are wrong here. but this is a kind of mental flag instruction anyway (see undocumented) */

U32 Z80_CB_BITm(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 Z;
	U16 MemAddress = Get_HL_(Z80_regs.PC-2);
	
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z = Z80_MEM_getByte(MemAddress);

	Z&=1<<op1;

	ComputeFlags(Z80_STATUS_Z,Z,Z80_STATUS_N,Z80_STATUS_H);
	if (Z80_regs.R[Z80_REG_F] & Z80_STATUS_Z)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_PV;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_PV;
	}
	if (op1==7)
	{
		if (Z!=0)
		{
			Z80_regs.R[Z80_REG_F]|=Z80_STATUS_S;
		}
		else
		{
			Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_S;
		}
	}
	if (op1==5)
	{
		if (Z!=0)
		{
			Z80_regs.R[Z80_REG_F]|=Z80_STATUS_X_B5;
		}
		else
		{
			Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_X_B5;
		}
	}
	if (op1==3)
	{
		if (Z!=0)
		{
			Z80_regs.R[Z80_REG_F]|=Z80_STATUS_X_B3;
		}
		else
		{
			Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_X_B3;
		}
	}
	Z80Cycles=12;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=8;
	}

	return 0;
}

U32 Z80_JPcc(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 nH,nL;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	nL=Z80_MEM_getByte(Z80_regs.PC);
	nH=Z80_MEM_getByte(Z80_regs.PC+1);

	if (TestFlags(op1))
	{
		Z80_regs.PC=(nH<<8) | nL;
	}
	else
	{
		Z80_regs.PC+=2;
	}

	Z80Cycles=10;

	return 0;
}

U32 Z80_LDm(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 memAddress = Get_HL_(Z80_regs.PC);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_MEM_setByte(memAddress,	GetR(op1,0));

	Z80Cycles=7;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=12;
		Z80_regs.PC+=1;
	}

	return 0;
}

U32 Z80_LD_nn_A(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 nHnL;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	nHnL=Z80_MEM_getByte(Z80_regs.PC);
	nHnL|=Z80_MEM_getByte(Z80_regs.PC+1)<<8;

	Z80_regs.PC+=2;

	Z80_MEM_setByte(nHnL,Z80_regs.R[Z80_REG_A]);

	Z80Cycles=13;

	return 0;
}

U32 Z80_RET(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80_regs.PC=Z80_MEM_getByte(Z80_regs.SP);
	Z80_regs.SP++;
	Z80_regs.PC|=Z80_MEM_getByte(Z80_regs.SP)<<8;
	Z80_regs.SP++;

	Z80Cycles=10;

	return 0;
}

U32 Z80_INC(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 src=1;
	U8 dst=GetR(op1,1);
	U8 res=dst+src;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	SetR(op1,1,res);
	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3,res,Z80_STATUS_N,0);
	ComputeAddHV(src,dst,res);

	Z80Cycles=4;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}
	return 0;
}

U32 Z80_LDA_nn(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 nHnL;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	nHnL=Z80_MEM_getByte(Z80_regs.PC);
	nHnL|=Z80_MEM_getByte(Z80_regs.PC+1)<<8;

	Z80_regs.PC+=2;

	Z80_regs.R[Z80_REG_A] = Z80_MEM_getByte(nHnL);

	Z80Cycles=13;

	return 0;
}

U32 Z80_OR(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80_regs.R[Z80_REG_A] |= GetR(op1,1);

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,Z80_regs.R[Z80_REG_A],Z80_STATUS_C|Z80_STATUS_H|Z80_STATUS_N,0);

	Z80Cycles=4;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}

	return 0;
}

U32 Z80_ED_LDIR(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 src = (Z80_regs.R[Z80_REG_H]<<8)|Z80_regs.R[Z80_REG_L];
	U16 dst = (Z80_regs.R[Z80_REG_D]<<8)|Z80_regs.R[Z80_REG_E];
	U16 cnt = (Z80_regs.R[Z80_REG_B]<<8)|Z80_regs.R[Z80_REG_C];

	U8 byte = Z80_MEM_getByte(src);
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_MEM_setByte(dst,byte);

	src++;
	dst++;
	cnt--;

	Z80_regs.R[Z80_REG_H] = src>>8;
	Z80_regs.R[Z80_REG_L] = src&0xFF;
	Z80_regs.R[Z80_REG_D] = dst>>8;
	Z80_regs.R[Z80_REG_E] = dst&0xFF;
	Z80_regs.R[Z80_REG_B] = cnt>>8;
	Z80_regs.R[Z80_REG_C] = cnt&0xFF;

	if (cnt!=0)
	{
		Z80Cycles=21;
		Z80_regs.PC-=2;
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_PV;
	}
	else
	{
		Z80Cycles=16;
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_PV;
	}

	if ((Z80_regs.R[Z80_REG_A]+byte)&0x02)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_X_B5;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_X_B5;
	}
	if ((Z80_regs.R[Z80_REG_A]+byte)&0x08)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_X_B3;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_X_B3;
	}
	ComputeFlags(0,Z80_regs.R[Z80_REG_A]+byte,Z80_STATUS_H|Z80_STATUS_N,0);

	return 0;
}

U32 Z80_LDrr(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	SetR(op1,1,GetR(op2,1));

	Z80Cycles=4;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}

	return 0;
}

U32 Z80_DEC(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 dst=GetR(op1,1);
	U8 src=1;
	U8 res=dst-src;
	
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	SetR(op1,1,res);

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3,res,0,Z80_STATUS_N);
	ComputeSubHV(src,dst,res);

	Z80Cycles=4;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}

	return 0;
}

U32 Z80_RLCA(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 carry = Z80_regs.R[Z80_REG_A]&0x80;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.R[Z80_REG_A]<<=1;

	if (carry)
	{
		Z80_regs.R[Z80_REG_A]|=0x01;
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_C;
	}
	else
	{
		Z80_regs.R[Z80_REG_A]&=~0x01;
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_C;
	}
	
	ComputeFlags(Z80_STATUS_X_B5|Z80_STATUS_X_B3,Z80_regs.R[Z80_REG_A],Z80_STATUS_H|Z80_STATUS_N,0);

	Z80Cycles=4;

	return 0;
}

U32 Z80_RRCA(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 carry = Z80_regs.R[Z80_REG_A]&0x01;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.R[Z80_REG_A]>>=1;

	if (carry)
	{
		Z80_regs.R[Z80_REG_A]|=0x80;
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_C;
	}
	else
	{
		Z80_regs.R[Z80_REG_A]&=~0x80;
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_C;
	}
	
	ComputeFlags(Z80_STATUS_X_B5|Z80_STATUS_X_B3,Z80_regs.R[Z80_REG_A],Z80_STATUS_H|Z80_STATUS_N,0);

	Z80Cycles=4;

	return 0;
}

U32 Z80_ORA_n(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 byte = Z80_MEM_getByte(Z80_regs.PC);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.PC++;

	Z80_regs.R[Z80_REG_A] |= byte;

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,Z80_regs.R[Z80_REG_A],Z80_STATUS_C|Z80_STATUS_H|Z80_STATUS_N,0);

	Z80Cycles=7;

	return 0;
}

U32 Z80_LD_HL(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 memAddress = Get_HL_(Z80_regs.PC);
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);

	SetR(op1,0,Z80_MEM_getByte(memAddress));

	Z80Cycles=7;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=12;
		Z80_regs.PC+=1;
	}

	return 0;
}

U32 Z80_ADDA_n(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 dst=Z80_regs.R[Z80_REG_A];
	U8 src=Z80_MEM_getByte(Z80_regs.PC);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.PC++;

	Z80_regs.R[Z80_REG_A]=dst+src;

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3,Z80_regs.R[Z80_REG_A],Z80_STATUS_N,0);
	ComputeAddHCV(src,dst,Z80_regs.R[Z80_REG_A]);

	Z80Cycles=7;

	return 0;
}

U32 Z80_ADCA_n(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 dst=Z80_regs.R[Z80_REG_A];
	U8 src=Z80_MEM_getByte(Z80_regs.PC);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.PC++;

	Z80_regs.R[Z80_REG_A]=dst+src;
	if (Z80_regs.R[Z80_REG_F]&Z80_STATUS_C)
		Z80_regs.R[Z80_REG_A]++;

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3,Z80_regs.R[Z80_REG_A],Z80_STATUS_N,0);
	ComputeAddHCV(src,dst,Z80_regs.R[Z80_REG_A]);

	Z80Cycles=7;

	return 0;
}

U32 Z80_INCRP(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 rp=GetRP(op1);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	rp++;

	SetRP(op1,rp>>8,rp&0xFF);

	Z80Cycles=6;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}

	return 0;
}

U32 Z80_DECRP(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 rp=GetRP(op1);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	rp--;

	SetRP(op1,rp>>8,rp&0xFF);

	Z80Cycles=6;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}

	return 0;
}

U32 Z80_NOP(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80Cycles=4;
	return 0;
}

U32 Z80_LDSP_HL(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 rp = GetHL();
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);

	Z80_regs.SP = rp;

	Z80Cycles=6;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}
	return 0;
}

U32 Z80_ED_LDI_A(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80_regs.IR&=0x00FF;
	Z80_regs.IR|=Z80_regs.R[Z80_REG_A]<<8;

	Z80Cycles=9;

	return 0;
}

U32 Z80_ED_LDR_A(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80_regs.IR&=0xFF00;
	Z80_regs.IR|=Z80_regs.R[Z80_REG_A];

	Z80Cycles=9;

	return 0;
}

U32 Z80_POPRP(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 nH,nL;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	nL=Z80_MEM_getByte(Z80_regs.SP);
	Z80_regs.SP++;
	nH=Z80_MEM_getByte(Z80_regs.SP);
	Z80_regs.SP++;

	SetRPAF(op1,nH,nL);

	Z80Cycles=10;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}

	return 0;
}

U32 Z80_EXAF(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	int a;
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	for (a=Z80_REG_F;a<=Z80_REG_A;a++)
	{
		U8 tmp;
		tmp = Z80_regs.R[a];
		Z80_regs.R[a]=Z80_regs.R_[a];
		Z80_regs.R_[a]=tmp;
	}
	Z80Cycles=4;

	return 0;
}

U32 Z80_EXX(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	int a;
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	for (a=Z80_REG_B;a<=Z80_REG_L;a++)
	{
		U8 tmp;
		tmp = Z80_regs.R[a];
		Z80_regs.R[a]=Z80_regs.R_[a];
		Z80_regs.R_[a]=tmp;
	}
	Z80Cycles=4;

	return 0;
}

U32 Z80_LDHL_n(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 byte;
	U16 memAddress = Get_HL_(Z80_regs.PC);

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
	{
		Z80_regs.PC++;
	}

	byte = Z80_MEM_getByte(Z80_regs.PC);
	Z80_regs.PC++;

	Z80_MEM_setByte(memAddress,byte);

	Z80Cycles=10;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=9;
	}
	return 0;
}

U32 Z80_JPHL(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80_regs.PC=GetHL();

	Z80Cycles=4;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}

	return 0;
}

U32 Z80_CP(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 dst = Z80_regs.R[Z80_REG_A];
	U8 src = GetR(op1,1);
	U8 tmp = dst-src;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	ComputeFlags(Z80_STATUS_X_B5|Z80_STATUS_X_B3,src,0,0);								/* bit 3 & 5 are based on source operand, not result */
	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z,tmp,0,Z80_STATUS_N);
	ComputeSubHCV(src,dst,tmp);

	Z80Cycles=4;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}

	return 0;
}

U32 Z80_RRA(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 carry = Z80_regs.R[Z80_REG_A]&0x01;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.R[Z80_REG_A]>>=1;
	
	if (Z80_regs.R[Z80_REG_F]&Z80_STATUS_C)
	{
		Z80_regs.R[Z80_REG_A]|=0x80;
	}
	else
	{
		Z80_regs.R[Z80_REG_A]&=~0x80;
	}

	if (carry)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_C;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_C;
	}
	
	ComputeFlags(Z80_STATUS_X_B5|Z80_STATUS_X_B3,Z80_regs.R[Z80_REG_A],Z80_STATUS_H|Z80_STATUS_N,0);

	Z80Cycles=4;

	return 0;
}

U32 Z80_RLA(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 carry = Z80_regs.R[Z80_REG_A]&0x80;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.R[Z80_REG_A]<<=1;
	
	if (Z80_regs.R[Z80_REG_F]&Z80_STATUS_C)
	{
		Z80_regs.R[Z80_REG_A]|=0x01;
	}
	else
	{
		Z80_regs.R[Z80_REG_A]&=~0x01;
	}

	if (carry)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_C;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_C;
	}
	
	ComputeFlags(Z80_STATUS_X_B5|Z80_STATUS_X_B3,Z80_regs.R[Z80_REG_A],Z80_STATUS_H|Z80_STATUS_N,0);

	Z80Cycles=4;

	return 0;
}

U32 Z80_ANDA_n(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 byte = Z80_MEM_getByte(Z80_regs.PC);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.PC++;

	Z80_regs.R[Z80_REG_A] &= byte;

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,Z80_regs.R[Z80_REG_A],Z80_STATUS_C|Z80_STATUS_N,Z80_STATUS_H);

	Z80Cycles=7;

	return 0;
}

U32 Z80_CB_SLA(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 carry;
	U8 res;
	U16 memAddress = Get_HL_(Z80_regs.PC-2);
	
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
	{
		SetR(op1,0,Z80_MEM_getByte(memAddress));
	}

	res = GetR(op1,0);
	carry = res&0x80;

	res<<=1;
	
	res&=~0x01;			/* should not be needed */

	if (carry)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_C;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_C;
	}
	
	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,res,Z80_STATUS_H|Z80_STATUS_N,0);

	SetR(op1,0,res);
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80_MEM_setByte(memAddress,res);
	}

	Z80Cycles=8;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=15;
	}

	return 0;
}

U32 Z80_ADDHL(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 dst=GetHL();
	U16 src;
	U16 res;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	src = GetRP(op1);

	res = dst+src;

	SetHL(res);

	ComputeFlags(Z80_STATUS_X_B3|Z80_STATUS_X_B5,res>>8,Z80_STATUS_N,0);
	ComputeAdd16HC(src,dst,res);

	Z80Cycles=11;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}

	return 0;
}

U32 Z80_DJNZ(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	S8 offs = Z80_MEM_getByte(Z80_regs.PC);
	
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.PC++;

	Z80_regs.R[Z80_REG_B]--;

	if (Z80_regs.R[Z80_REG_B])
	{
		Z80Cycles=13;
		Z80_regs.PC+=offs;
	}
	else
	{
		Z80Cycles=8;
	}

	return 0;
}

U32 Z80_CPHL(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 src = Z80_MEM_getByte(Get_HL_(Z80_regs.PC));
	U8 dst = Z80_regs.R[Z80_REG_A];
	U8 tmp = dst-src;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	ComputeFlags(Z80_STATUS_X_B5|Z80_STATUS_X_B3,src,0,0);								/* bit 3 & 5 are based on source operand, not result */
	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z,tmp,0,Z80_STATUS_N);
	ComputeSubHCV(src,dst,tmp);

	Z80Cycles=7;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=12;
		Z80_regs.PC+=1;
	}

	return 0;
}

U32 Z80_INCHL(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 memAddress = Get_HL_(Z80_regs.PC);
	U8 dst = Z80_MEM_getByte(memAddress);
	U8 src = 1;
	U8 res = dst+src;
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_MEM_setByte(memAddress,res);	

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3,res,Z80_STATUS_N,0);
	ComputeAddHV(src,dst,res);

	Z80Cycles=11;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=12;
		Z80_regs.PC+=1;
	}

	return 0;
}

U32 Z80_JR(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	S8 offs = (S8)Z80_MEM_getByte(Z80_regs.PC);
	
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.PC++;

	Z80_regs.PC+=offs;

	Z80Cycles=12;

	return 0;
}

U32 Z80_SUBA_n(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 dst=Z80_regs.R[Z80_REG_A];
	U8 src=Z80_MEM_getByte(Z80_regs.PC);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.PC++;

	Z80_regs.R[Z80_REG_A]=dst-src;

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3,Z80_regs.R[Z80_REG_A],0,Z80_STATUS_N);
	ComputeSubHCV(src,dst,Z80_regs.R[Z80_REG_A]);

	Z80Cycles=7;

	return 0;
}

U32 Z80_CP_n(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 dst=Z80_regs.R[Z80_REG_A];
	U8 src=Z80_MEM_getByte(Z80_regs.PC);
	U8 tmp = dst-src;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.PC++;

	ComputeFlags(Z80_STATUS_X_B5|Z80_STATUS_X_B3,src,0,0);								/* bit 3 & 5 are based on source operand, not result */
	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z,tmp,0,Z80_STATUS_N);
	ComputeSubHCV(src,dst,tmp);

	Z80Cycles=7;

	return 0;
}

U32 Z80_JRcc(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	S8 offs = (S8)Z80_MEM_getByte(Z80_regs.PC);
	
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.PC++;

	if (TestFlags(op1))
	{
		Z80Cycles=12;
		Z80_regs.PC+=offs;
	}
	else
	{
		Z80Cycles=7;
	}
	return 0;
}

U32 Z80_PUSHRP(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 val=GetRPAF(op1);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.SP--;
	Z80_MEM_setByte(Z80_regs.SP,val>>8);
	Z80_regs.SP--;
	Z80_MEM_setByte(Z80_regs.SP,val&0xFF);

	Z80Cycles=11;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}

	return 0;
}

U32 Z80_CB_BIT(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 memAddress = Get_HL_(Z80_regs.PC-2);
	U8 Z;
	
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
	{
		Z = Z80_MEM_getByte(memAddress);
	}
	else
	{
		Z = Z80_regs.R[op2];
	}

	Z&=1<<op1;

	ComputeFlags(Z80_STATUS_Z,Z,Z80_STATUS_N,Z80_STATUS_H);
	if (Z80_regs.R[Z80_REG_F] & Z80_STATUS_Z)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_PV;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_PV;
	}
	if (op1==7)
	{
		if (Z!=0)
		{
			Z80_regs.R[Z80_REG_F]|=Z80_STATUS_S;
		}
		else
		{
			Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_S;
		}
	}
	if (op1==5)
	{
		if (Z!=0)
		{
			Z80_regs.R[Z80_REG_F]|=Z80_STATUS_X_B5;
		}
		else
		{
			Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_X_B5;
		}
	}
	if (op1==3)
	{
		if (Z!=0)
		{
			Z80_regs.R[Z80_REG_F]|=Z80_STATUS_X_B3;
		}
		else
		{
			Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_X_B3;
		}
	}
	Z80Cycles=8;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=12;
	}
	return 0;
}

U32 Z80_LDA_DE(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80_regs.R[Z80_REG_A] = Z80_MEM_getByte((Z80_regs.R[Z80_REG_D]<<8) | Z80_regs.R[Z80_REG_E]);

	Z80Cycles=7;

	return 0;
}

U32 Z80_CB_SRL(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 carry;
	U8 res;
	U16 memAddress = Get_HL_(Z80_regs.PC-2);
	
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
	{
		SetR(op1,0,Z80_MEM_getByte(memAddress));
	}

	res = GetR(op1,0);
	carry = res&0x01;

	res>>=1;
	
	res&=~0x80;			/* should not be needed */

	if (carry)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_C;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_C;
	}
	
	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,res,Z80_STATUS_H|Z80_STATUS_N,0);

	SetR(op1,0,res);
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80_MEM_setByte(memAddress,res);
	}

	Z80Cycles=8;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=15;
	}

	return 0;
}

U32 Z80_EI(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80_regs.delayedInterruptEnable=2;		/* will enable AFTER next instruction cycle */

	Z80Cycles=4;

	return 0;
}

U32 Z80_ADDA_r(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 dst=Z80_regs.R[Z80_REG_A];
	U8 src=GetR(op1,1);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.R[Z80_REG_A]=dst+src;

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3,Z80_regs.R[Z80_REG_A],Z80_STATUS_N,0);
	ComputeAddHCV(src,dst,Z80_regs.R[Z80_REG_A]);

	Z80Cycles=4;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}

	return 0;
}

U32 Z80_RST(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80_regs.SP--;
	Z80_MEM_setByte(Z80_regs.SP,Z80_regs.PC>>8);
	Z80_regs.SP--;
	Z80_MEM_setByte(Z80_regs.SP,Z80_regs.PC&0xFF);

	Z80_regs.PC=op1<<3;

	Z80Cycles=11;

	return 0;
}

U32 Z80_LDDE_A(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80_MEM_setByte((Z80_regs.R[Z80_REG_D]<<8) | Z80_regs.R[Z80_REG_E],Z80_regs.R[Z80_REG_A]);

	Z80Cycles=7;

	return 0;
}

U32 Z80_OUTn_A(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 n;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	n=Z80_MEM_getByte(Z80_regs.PC);

	Z80_regs.PC+=1;

	Z80_IO_setByte((Z80_regs.R[Z80_REG_A]<<8)|n,Z80_regs.R[Z80_REG_A]);

	Z80Cycles=11;

	return 0;
}

U32 Z80_AND(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80_regs.R[Z80_REG_A] &= GetR(op1,1);

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,Z80_regs.R[Z80_REG_A],Z80_STATUS_C|Z80_STATUS_N,Z80_STATUS_H);

	Z80Cycles=4;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}

	return 0;
}

U32 Z80_ED_SBCHL(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 dst=(Z80_regs.R[Z80_REG_H]<<8)|Z80_regs.R[Z80_REG_L];
	U16 src;
	U16 res;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	switch (op1)
	{
	case 0:
		src=Z80_regs.R[Z80_REG_B]<<8;
		src|=Z80_regs.R[Z80_REG_C]&0xFF;
		break;
	case 1:
		src=Z80_regs.R[Z80_REG_D]<<8;
		src|=Z80_regs.R[Z80_REG_E]&0xFF;
		break;
	case 2:
		src=Z80_regs.R[Z80_REG_H]<<8;
		src|=Z80_regs.R[Z80_REG_L]&0xFF;
		break;
	case 3:
		src = Z80_regs.SP;
		break;
	default:
		DEB_PauseEmulation(DEB_Mode_Z80,"Something gone wrong");
		src=0;
		break;
	}

	res = dst-src;
	if (Z80_regs.R[Z80_REG_F]&Z80_STATUS_C)
	{
		res--;
	}
	
	Z80_regs.R[Z80_REG_H]=res>>8;
	Z80_regs.R[Z80_REG_L]=res&0xFF;

	Compute16Flags(Z80_STATUS_Z|Z80_STATUS_S,res);
	ComputeFlags(Z80_STATUS_X_B3|Z80_STATUS_X_B5,Z80_regs.R[Z80_REG_H],0,Z80_STATUS_N);
	ComputeSub16HVC(src,dst,res);

	Z80Cycles=15;

	return 0;
}

U32 Z80_DECHL(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 memAddress = Get_HL_(Z80_regs.PC);
	U8 dst = Z80_MEM_getByte(memAddress);
	U8 src = 1;
	U8 res = dst-src;
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_MEM_setByte(memAddress,res);	

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3,res,0,Z80_STATUS_N);
	ComputeSubHV(src,dst,res);

	Z80Cycles=11;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80_regs.PC++;
		Z80Cycles+=12;
	}

	return 0;
}

U32 Z80_ED_LDnn_dd(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 nHnL;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	nHnL=Z80_MEM_getByte(Z80_regs.PC);
	nHnL|=Z80_MEM_getByte(Z80_regs.PC+1)<<8;

	Z80_regs.PC+=2;

	switch (op1)
	{
	case 0:
		Z80_MEM_setByte(nHnL,Z80_regs.R[Z80_REG_C]);
		Z80_MEM_setByte(nHnL+1,Z80_regs.R[Z80_REG_B]);
		break;
	case 1:
		Z80_MEM_setByte(nHnL,Z80_regs.R[Z80_REG_E]);
		Z80_MEM_setByte(nHnL+1,Z80_regs.R[Z80_REG_D]);
		break;
	case 2:
		Z80_MEM_setByte(nHnL,Z80_regs.R[Z80_REG_L]);
		Z80_MEM_setByte(nHnL+1,Z80_regs.R[Z80_REG_H]);
		break;
	case 3:
		Z80_MEM_setByte(nHnL,Z80_regs.SP&0xFF);
		Z80_MEM_setByte(nHnL+1,Z80_regs.SP>>8);
		break;
	}

	Z80Cycles=20;

	return 0;
}

U32 Z80_LDnn_HL(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 rp = GetHL();
	U16 nHnL;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	nHnL=Z80_MEM_getByte(Z80_regs.PC);
	nHnL|=Z80_MEM_getByte(Z80_regs.PC+1)<<8;

	Z80_regs.PC+=2;

	Z80_MEM_setByte(nHnL,rp&0xFF);
	Z80_MEM_setByte(nHnL+1,rp>>8);

	Z80Cycles=16;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}

	return 0;
}

U32 Z80_EXDE_HL(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 tmp;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	tmp=Z80_regs.R[Z80_REG_D];
	Z80_regs.R[Z80_REG_D]=Z80_regs.R[Z80_REG_H];
	Z80_regs.R[Z80_REG_H]=tmp;
	tmp=Z80_regs.R[Z80_REG_E];
	Z80_regs.R[Z80_REG_E]=Z80_regs.R[Z80_REG_L];
	Z80_regs.R[Z80_REG_L]=tmp;

	Z80Cycles=4;

	return 0;
}

U32 Z80_ED_LDDR(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 src = (Z80_regs.R[Z80_REG_H]<<8)|Z80_regs.R[Z80_REG_L];
	U16 dst = (Z80_regs.R[Z80_REG_D]<<8)|Z80_regs.R[Z80_REG_E];
	U16 cnt = (Z80_regs.R[Z80_REG_B]<<8)|Z80_regs.R[Z80_REG_C];

	U8 byte = Z80_MEM_getByte(src);
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_MEM_setByte(dst,byte);

	src--;
	dst--;
	cnt--;

	Z80_regs.R[Z80_REG_H] = src>>8;
	Z80_regs.R[Z80_REG_L] = src&0xFF;
	Z80_regs.R[Z80_REG_D] = dst>>8;
	Z80_regs.R[Z80_REG_E] = dst&0xFF;
	Z80_regs.R[Z80_REG_B] = cnt>>8;
	Z80_regs.R[Z80_REG_C] = cnt&0xFF;

	if (cnt!=0)
	{
		Z80Cycles=21;
		Z80_regs.PC-=2;
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_PV;
	}
	else
	{
		Z80Cycles=16;
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_PV;
	}

	if ((Z80_regs.R[Z80_REG_A]+byte)&0x02)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_X_B5;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_X_B5;
	}
	if ((Z80_regs.R[Z80_REG_A]+byte)&0x08)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_X_B3;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_X_B3;
	}
	ComputeFlags(0,Z80_regs.R[Z80_REG_A]+byte,Z80_STATUS_H,Z80_STATUS_N);

	return 0;
}

U32 Z80_LDHL_nn(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 nHnL;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	nHnL=Z80_MEM_getByte(Z80_regs.PC);
	nHnL|=Z80_MEM_getByte(Z80_regs.PC+1)<<8;

	Z80_regs.PC+=2;

	SetHL((Z80_MEM_getByte(nHnL+1)<<8)|Z80_MEM_getByte(nHnL));

	Z80Cycles=16;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}

	return 0;
}

U32 Z80_CB_SETm(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 Z;
	U16 MemAddress = Get_HL_(Z80_regs.PC-2);
	
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z = Z80_MEM_getByte(MemAddress);
	Z|=1<<op1;
	Z80_MEM_setByte(MemAddress,Z);

	Z80Cycles=15;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=8;
	}

	return 0;
}

U32 Z80_CB_RESm(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 Z;
	U16 MemAddress = Get_HL_(Z80_regs.PC-2);
	
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z = Z80_MEM_getByte(MemAddress);
	Z&=~(1<<op1);
	Z80_MEM_setByte(MemAddress,Z);

	Z80Cycles=15;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=8;
	}

	return 0;
}

U32 Z80_SUBA_r(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 dst=Z80_regs.R[Z80_REG_A];
	U8 src=GetR(op1,1);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.R[Z80_REG_A]=dst-src;

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3,Z80_regs.R[Z80_REG_A],0,Z80_STATUS_N);
	ComputeSubHCV(src,dst,Z80_regs.R[Z80_REG_A]);

	Z80Cycles=4;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}

	return 0;
}

U32 Z80_RETcc(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	if (TestFlags(op1))
	{
		Z80_regs.PC=Z80_MEM_getByte(Z80_regs.SP);
		Z80_regs.SP++;
		Z80_regs.PC|=Z80_MEM_getByte(Z80_regs.SP)<<8;
		Z80_regs.SP++;

		Z80Cycles=11;
	}
	else
	{
		Z80Cycles=5;
	}

	return 0;
}

U32 Z80_SCF(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	ComputeFlags(Z80_STATUS_X_B3|Z80_STATUS_X_B5,Z80_regs.R[Z80_REG_A],Z80_STATUS_H|Z80_STATUS_N,Z80_STATUS_C);
	Z80Cycles=4;

	return 0;
}

U32 Z80_XORm(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 memAddress = Get_HL_(Z80_regs.PC);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.R[Z80_REG_A] ^= Z80_MEM_getByte(memAddress);

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,Z80_regs.R[Z80_REG_A],Z80_STATUS_C|Z80_STATUS_H|Z80_STATUS_N,0);

	Z80Cycles=7;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80_regs.PC++;
		Z80Cycles+=12;
	}

	return 0;
}

U32 Z80_CCF(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	if (Z80_regs.R[Z80_REG_F]&Z80_STATUS_C)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_H;
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_C;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_H;
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_C;
	}
	ComputeFlags(Z80_STATUS_X_B3|Z80_STATUS_X_B5,Z80_regs.R[Z80_REG_A],Z80_STATUS_N,0);
	Z80Cycles=4;

	return 0;
}

U32 Z80_ADDm(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 memAddress = Get_HL_(Z80_regs.PC);
	U8 dst=Z80_regs.R[Z80_REG_A];
	U8 src=Z80_MEM_getByte(memAddress);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.R[Z80_REG_A]=dst+src;

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3,Z80_regs.R[Z80_REG_A],Z80_STATUS_N,0);
	ComputeAddHCV(src,dst,Z80_regs.R[Z80_REG_A]);

	Z80Cycles=7;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80_regs.PC++;
		Z80Cycles+=12;
	}

	return 0;
}

U32 Z80_EXSP_HL(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 rp = GetHL();
	U8 nH,nL;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	nL=Z80_MEM_getByte(Z80_regs.SP);
	nH=Z80_MEM_getByte(Z80_regs.SP+1);

	Z80_MEM_setByte(Z80_regs.SP,rp&0xFF);
	Z80_MEM_setByte(Z80_regs.SP+1,rp>>8);

	SetHL((nH<<8)|nL);

	Z80Cycles=19;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}
	return 0;
}

U32 Z80_ED_LDdd_nn(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 nHnL;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	nHnL=Z80_MEM_getByte(Z80_regs.PC);
	nHnL|=Z80_MEM_getByte(Z80_regs.PC+1)<<8;

	Z80_regs.PC+=2;

	switch (op1)
	{
	case 0:
		Z80_regs.R[Z80_REG_C]=Z80_MEM_getByte(nHnL);
		Z80_regs.R[Z80_REG_B]=Z80_MEM_getByte(nHnL+1);
		break;
	case 1:
		Z80_regs.R[Z80_REG_E]=Z80_MEM_getByte(nHnL);
		Z80_regs.R[Z80_REG_D]=Z80_MEM_getByte(nHnL+1);
		break;
	case 2:
		Z80_regs.R[Z80_REG_L]=Z80_MEM_getByte(nHnL);
		Z80_regs.R[Z80_REG_H]=Z80_MEM_getByte(nHnL+1);
		break;
	case 3:
		Z80_regs.SP=Z80_MEM_getByte(nHnL);
		Z80_regs.SP|=Z80_MEM_getByte(nHnL+1)<<8;
		break;
	}

	Z80Cycles=20;

	return 0;
}

U32 Z80_ADCA_r(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 dst=Z80_regs.R[Z80_REG_A];
	U8 src=GetR(op1,1);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.R[Z80_REG_A]=dst+src;
	if (Z80_regs.R[Z80_REG_F]&Z80_STATUS_C)
		Z80_regs.R[Z80_REG_A]++;

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3,Z80_regs.R[Z80_REG_A],Z80_STATUS_N,0);
	ComputeAddHCV(src,dst,Z80_regs.R[Z80_REG_A]);

	Z80Cycles=4;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}

	return 0;
}

U32 Z80_CPL(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	Z80_regs.R[Z80_REG_A]=~Z80_regs.R[Z80_REG_A];

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	ComputeFlags(Z80_STATUS_X_B3|Z80_STATUS_X_B5,Z80_regs.R[Z80_REG_A],0,Z80_STATUS_N|Z80_STATUS_H);
	Z80Cycles=4;

	return 0;
}

U32 Z80_XORA_n(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 byte = Z80_MEM_getByte(Z80_regs.PC);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.PC++;

	Z80_regs.R[Z80_REG_A] ^= byte;

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,Z80_regs.R[Z80_REG_A],Z80_STATUS_C|Z80_STATUS_H|Z80_STATUS_N,0);

	Z80Cycles=7;

	return 0;
}

U32 Z80_HALT(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80_regs.stopped=1;

	Z80Cycles=4;

	return 0;
}

U32 Z80_ED_INr_C(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 byte = Z80_IO_getByte((Z80_regs.R[Z80_REG_B]<<8)|Z80_regs.R[Z80_REG_C]);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	SetR(op1,0,byte);

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,byte,Z80_STATUS_H|Z80_STATUS_N,0);

	Z80Cycles=12;

	return 0;
}

U32 Z80_CB_RLC(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 carry;
	U8 res;
	U16 memAddress = Get_HL_(Z80_regs.PC-2);
	
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
	{
		SetR(op1,0,Z80_MEM_getByte(memAddress));
	}

	res = GetR(op1,0);
	carry = res&0x80;

	res<<=1;
	
	if (carry)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_C;
		res|=0x01;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_C;
		res&=~0x01;
	}
	
	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,res,Z80_STATUS_H|Z80_STATUS_N,0);

	SetR(op1,0,res);
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80_MEM_setByte(memAddress,res);
	}

	Z80Cycles=8;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=15;
	}

	return 0;
}

U32 Z80_CALLcc(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 nH,nL;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	nL=Z80_MEM_getByte(Z80_regs.PC);
	nH=Z80_MEM_getByte(Z80_regs.PC+1);

	Z80_regs.PC+=2;

	if (TestFlags(op1))
	{
		Z80_regs.SP--;
		Z80_MEM_setByte(Z80_regs.SP,Z80_regs.PC>>8);
		Z80_regs.SP--;
		Z80_MEM_setByte(Z80_regs.SP,Z80_regs.PC&0xFF);
	
		Z80_regs.PC=(nH<<8) | nL;

		Z80Cycles=17;
	}
	else
	{
		Z80Cycles=10;
	}

	return 0;
}

U32 Z80_SBCA_r(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 dst=Z80_regs.R[Z80_REG_A];
	U8 src=GetR(op1,1);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.R[Z80_REG_A]=dst-src;
	if (Z80_regs.R[Z80_REG_F]&Z80_STATUS_C)
	{
		Z80_regs.R[Z80_REG_A]--;
	}

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3,Z80_regs.R[Z80_REG_A],0,Z80_STATUS_N);
	ComputeSubHCV(src,dst,Z80_regs.R[Z80_REG_A]);

	Z80Cycles=4;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=4;
	}

	return 0;
}

U32 Z80_CB_RES(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 Z;
	U16 memAddress = Get_HL_(Z80_regs.PC-2);
	
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
	{
		SetR(op2,0,Z80_MEM_getByte(memAddress));
	}

	Z = GetR(op2,0);
	Z&=~(1<<op1);
	SetR(op2,0,Z);
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80_MEM_setByte(memAddress,Z);
	}

	Z80Cycles=8;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=15;
	}

	return 0;
}

U32 Z80_CB_SET(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 Z;
	U16 memAddress = Get_HL_(Z80_regs.PC-2);
	
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
	{
		SetR(op2,0,Z80_MEM_getByte(memAddress));
	}

	Z = GetR(op2,0);
	Z|=1<<op1;
	SetR(op2,0,Z);
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80_MEM_setByte(memAddress,Z);
	}

	Z80Cycles=8;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=15;
	}

	return 0;
}

U32 Z80_SUBm(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 memAddress = Get_HL_(Z80_regs.PC);
	U8 dst=Z80_regs.R[Z80_REG_A];
	U8 src=Z80_MEM_getByte(memAddress);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.R[Z80_REG_A]=dst-src;

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3,Z80_regs.R[Z80_REG_A],0,Z80_STATUS_N);
	ComputeSubHCV(src,dst,Z80_regs.R[Z80_REG_A]);

	Z80Cycles=7;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80_regs.PC++;
		Z80Cycles+=12;
	}

	return 0;
}

U32 Z80_INA_n(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 portLo = Z80_MEM_getByte(Z80_regs.PC);
	U8 byte;
	
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.PC++;

	byte = Z80_IO_getByte((Z80_regs.R[Z80_REG_A]<<8)|portLo);

	Z80_regs.R[Z80_REG_A] = byte;

	Z80Cycles=11;

	return 0;
}

U32 Z80_ANDm(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 memAddress = Get_HL_(Z80_regs.PC);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.R[Z80_REG_A] &= Z80_MEM_getByte(memAddress);

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,Z80_regs.R[Z80_REG_A],Z80_STATUS_C|Z80_STATUS_N,Z80_STATUS_H);

	Z80Cycles=7;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80_regs.PC++;
		Z80Cycles+=12;
	}

	return 0;
}

U32 Z80_ORm(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 memAddress = Get_HL_(Z80_regs.PC);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.R[Z80_REG_A] |= Z80_MEM_getByte(memAddress);

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,Z80_regs.R[Z80_REG_A],Z80_STATUS_C|Z80_STATUS_H|Z80_STATUS_N,0);

	Z80Cycles=7;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80_regs.PC++;
		Z80Cycles+=12;
	}

	return 0;
}

U32 Z80_CB_RL(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 carry;
	U8 res;
	U16 memAddress = Get_HL_(Z80_regs.PC-2);
	
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
	{
		SetR(op1,0,Z80_MEM_getByte(memAddress));
	}

	res = GetR(op1,0);
	carry = res&0x80;

	res<<=1;
	
	if (Z80_regs.R[Z80_REG_F]&Z80_STATUS_C)
	{
		res|=0x01;
	}
	else
	{
		res&=~0x01;
	}

	if (carry)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_C;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_C;
	}
	
	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,res,Z80_STATUS_H|Z80_STATUS_N,0);

	SetR(op1,0,res);
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80_MEM_setByte(memAddress,res);
	}

	Z80Cycles=8;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=15;
	}

	return 0;
}

U32 Z80_ADCm(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 memAddress = Get_HL_(Z80_regs.PC);
	U8 dst=Z80_regs.R[Z80_REG_A];
	U8 src=Z80_MEM_getByte(memAddress);
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);

	Z80_regs.R[Z80_REG_A]=dst+src;
	if (Z80_regs.R[Z80_REG_F]&Z80_STATUS_C)
		Z80_regs.R[Z80_REG_A]++;

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3,Z80_regs.R[Z80_REG_A],Z80_STATUS_N,0);
	ComputeAddHCV(src,dst,Z80_regs.R[Z80_REG_A]);

	Z80Cycles=7;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80_regs.PC++;
		Z80Cycles+=12;
	}

	return 0;
}

U32 Z80_CB_RRC(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 carry;
	U8 res;
	U16 memAddress = Get_HL_(Z80_regs.PC-2);
	
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
	{
		SetR(op1,0,Z80_MEM_getByte(memAddress));
	}

	res = GetR(op1,0);
	carry = res&0x01;

	res>>=1;
	
	if (carry)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_C;
		res|=0x80;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_C;
		res&=~0x80;
	}
	
	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,res,Z80_STATUS_H|Z80_STATUS_N,0);

	SetR(op1,0,res);
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80_MEM_setByte(memAddress,res);
	}

	Z80Cycles=8;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=15;
	}

	return 0;
}

U32 Z80_CB_RR(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 carry;
	U8 res;
	U16 memAddress = Get_HL_(Z80_regs.PC-2);
	
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
	{
		SetR(op1,0,Z80_MEM_getByte(memAddress));
	}

	res = GetR(op1,0);
	carry = res&0x01;

	res>>=1;
	
	if (Z80_regs.R[Z80_REG_F]&Z80_STATUS_C)
	{
		res|=0x80;
	}
	else
	{
		res&=~0x80;
	}

	if (carry)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_C;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_C;
	}
	
	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,res,Z80_STATUS_H|Z80_STATUS_N,0);

	SetR(op1,0,res);
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80_MEM_setByte(memAddress,res);
	}

	Z80Cycles=8;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=15;
	}

	return 0;
}

U32 Z80_ED_ADCHL(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 dst=(Z80_regs.R[Z80_REG_H]<<8)|Z80_regs.R[Z80_REG_L];
	U16 src;
	U16 res;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	switch (op1)
	{
	case 0:
		src=Z80_regs.R[Z80_REG_B]<<8;
		src|=Z80_regs.R[Z80_REG_C]&0xFF;
		break;
	case 1:
		src=Z80_regs.R[Z80_REG_D]<<8;
		src|=Z80_regs.R[Z80_REG_E]&0xFF;
		break;
	case 2:
		src=Z80_regs.R[Z80_REG_H]<<8;
		src|=Z80_regs.R[Z80_REG_L]&0xFF;
		break;
	case 3:
		src = Z80_regs.SP;
		break;
	default:
		DEB_PauseEmulation(DEB_Mode_Z80,"Something gone wrong");
		src=0;
		break;
	}

	res = dst+src;
	if (Z80_regs.R[Z80_REG_F]&Z80_STATUS_C)
	{
		res++;
	}
	
	Z80_regs.R[Z80_REG_H]=res>>8;
	Z80_regs.R[Z80_REG_L]=res&0xFF;

	Compute16Flags(Z80_STATUS_Z|Z80_STATUS_S,res);
	ComputeFlags(Z80_STATUS_X_B3|Z80_STATUS_X_B5,Z80_regs.R[Z80_REG_H],Z80_STATUS_N,0);
	ComputeAdd16HVC(src,dst,res);

	Z80Cycles=15;

	return 0;
}

U32 Z80_CB_RLm(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 carry;
	U8 res;
	U16 memAddress = Get_HL_(Z80_regs.PC-2);
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	
	res = Z80_MEM_getByte(memAddress);
	carry = res&0x80;

	res<<=1;
	
	if (Z80_regs.R[Z80_REG_F]&Z80_STATUS_C)
	{
		res|=0x01;
	}
	else
	{
		res&=~0x01;
	}

	if (carry)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_C;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_C;
	}
	
	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,res,Z80_STATUS_H|Z80_STATUS_N,0);

	Z80_MEM_setByte(memAddress,res);

	Z80Cycles=8;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=15;
	}

	return 0;
}

U32 Z80_CB_SRA(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 carry;
	U8 res;
	U16 memAddress = Get_HL_(Z80_regs.PC-2);
	
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
	{
		SetR(op1,0,Z80_MEM_getByte(memAddress));
	}

	res = GetR(op1,0);
	carry = res&0x01;

	res>>=1;

	if (res&0x40)
	{
		res|=0x80;
	}
	else
	{
		res&=~0x80;
	}

	if (carry)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_C;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_C;
	}
	
	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,res,Z80_STATUS_H|Z80_STATUS_N,0);

	SetR(op1,0,res);
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80_MEM_setByte(memAddress,res);
	}

	Z80Cycles=8;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=15;
	}

	return 0;
}

U32 Z80_ED_NEG(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 src=Z80_regs.R[Z80_REG_A];
	U8 dst=0;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.R[Z80_REG_A]=dst-src;

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3,Z80_regs.R[Z80_REG_A],0,Z80_STATUS_N);
	ComputeSubHCV(src,dst,Z80_regs.R[Z80_REG_A]);

	Z80Cycles=8;

	return 0;
}

U32 Z80_LDBC_A(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80_MEM_setByte((Z80_regs.R[Z80_REG_B]<<8) | Z80_regs.R[Z80_REG_C],Z80_regs.R[Z80_REG_A]);

	Z80Cycles=7;

	return 0;
}

U32 Z80_ED_LDI(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 src = (Z80_regs.R[Z80_REG_H]<<8)|Z80_regs.R[Z80_REG_L];
	U16 dst = (Z80_regs.R[Z80_REG_D]<<8)|Z80_regs.R[Z80_REG_E];
	U16 cnt = (Z80_regs.R[Z80_REG_B]<<8)|Z80_regs.R[Z80_REG_C];

	U8 byte = Z80_MEM_getByte(src);
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_MEM_setByte(dst,byte);

	src++;
	dst++;
	cnt--;

	Z80_regs.R[Z80_REG_H] = src>>8;
	Z80_regs.R[Z80_REG_L] = src&0xFF;
	Z80_regs.R[Z80_REG_D] = dst>>8;
	Z80_regs.R[Z80_REG_E] = dst&0xFF;
	Z80_regs.R[Z80_REG_B] = cnt>>8;
	Z80_regs.R[Z80_REG_C] = cnt&0xFF;

	if (cnt!=0)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_PV;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_PV;
	}

	if ((Z80_regs.R[Z80_REG_A]+byte)&0x02)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_X_B5;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_X_B5;
	}
	if ((Z80_regs.R[Z80_REG_A]+byte)&0x08)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_X_B3;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_X_B3;
	}
	ComputeFlags(0,Z80_regs.R[Z80_REG_A]+byte,Z80_STATUS_H|Z80_STATUS_N,0);
	
	Z80Cycles=16;

	return 0;
}

U32 Z80_ED_RLD(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 hl = (Z80_regs.R[Z80_REG_H]<<8)|Z80_regs.R[Z80_REG_L];
	U8 accTmp = Z80_regs.R[Z80_REG_A];
	U8 hlTmp = Z80_MEM_getByte(hl);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	accTmp&=0xF0;
	accTmp|=hlTmp>>4;
	hlTmp<<=4;
	hlTmp|=Z80_regs.R[Z80_REG_A]&0x0F;

	Z80_regs.R[Z80_REG_A]=accTmp;
	Z80_MEM_setByte(hl,hlTmp);

	ComputeFlags(Z80_STATUS_Z|Z80_STATUS_S|Z80_STATUS_X_B3|Z80_STATUS_X_B5|Z80_STATUS_P,Z80_regs.R[Z80_REG_A],Z80_STATUS_H|Z80_STATUS_N,0);

	Z80Cycles=18;

	return 0;
}

U32 Z80_ED_LDA_R(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80_regs.R[Z80_REG_A]=Z80_regs.IR&0xFF;

	ComputeFlags(Z80_STATUS_Z|Z80_STATUS_S|Z80_STATUS_X_B3|Z80_STATUS_X_B5,Z80_regs.R[Z80_REG_A],Z80_STATUS_H|Z80_STATUS_N|Z80_STATUS_PV,0);
	if (Z80_regs.IFF2)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_PV;
	}
	Z80Cycles=9;

	return 0;
}

/* TODO verify flags */
U32 Z80_DAA(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 tmp=Z80_regs.R[Z80_REG_A];

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	if (Z80_regs.R[Z80_REG_F] & Z80_STATUS_N)
	{
		if ((Z80_regs.R[Z80_REG_F] & Z80_STATUS_H) || ((tmp&0xF)>9))
			tmp-=6;
		if ((Z80_regs.R[Z80_REG_F] & Z80_STATUS_C) || (tmp>0x99))
			tmp-=0x60;
	}
	else
	{
		if ((Z80_regs.R[Z80_REG_F] & Z80_STATUS_H) || ((tmp&0xF)>9))
			tmp+=6;
		if ((Z80_regs.R[Z80_REG_F] & Z80_STATUS_C) || (tmp>0x99))
			tmp+=0x60;
	}

	if (Z80_regs.R[Z80_REG_A]>0x99)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_C;
	}

	Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_H;
	Z80_regs.R[Z80_REG_F]|=(Z80_regs.R[Z80_REG_A]^tmp)&Z80_STATUS_H;

	Z80_regs.R[Z80_REG_A]=tmp;

	ComputeFlags(Z80_STATUS_Z|Z80_STATUS_S|Z80_STATUS_X_B3|Z80_STATUS_X_B5|Z80_STATUS_P,Z80_regs.R[Z80_REG_A],0,0);

	Z80Cycles=4;

	return 0;
}

U32 Z80_ED_CPIR(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 src = (Z80_regs.R[Z80_REG_H]<<8)|Z80_regs.R[Z80_REG_L];
	U16 cnt = (Z80_regs.R[Z80_REG_B]<<8)|Z80_regs.R[Z80_REG_C];

	U8 byte = Z80_MEM_getByte(src);
	U8 res = Z80_regs.R[Z80_REG_A] - byte;
	U8 n;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	ComputeSubHV(byte,Z80_regs.R[Z80_REG_A],res);
	ComputeFlags(Z80_STATUS_Z|Z80_STATUS_S,res,0,Z80_STATUS_N);

	if (Z80_regs.R[Z80_REG_F]&Z80_STATUS_H)
	{
		n=byte-1;
	}
	else
	{
		n=byte;
	}
	
	if ((n)&0x02)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_X_B5;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_X_B5;
	}
	if ((n)&0x08)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_X_B3;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_X_B3;
	}

	src++;
	cnt--;

	Z80_regs.R[Z80_REG_H] = src>>8;
	Z80_regs.R[Z80_REG_L] = src&0xFF;
	Z80_regs.R[Z80_REG_B] = cnt>>8;
	Z80_regs.R[Z80_REG_C] = cnt&0xFF;

	if (cnt!=0)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_PV;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_PV;
	}

	if (cnt!=0 && res!=0)
	{
		Z80Cycles=21;
		Z80_regs.PC-=2;
	}
	else
	{
		Z80Cycles=16;
	}

	return 0;
}

U32 Z80_ED_OUTC_r(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 byte = GetR(op1,0);
		
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_IO_setByte((Z80_regs.R[Z80_REG_B]<<8)|Z80_regs.R[Z80_REG_C],byte);

	Z80Cycles=12;

	return 0;
}

U32 Z80_LDA_BC(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80_regs.R[Z80_REG_A] = Z80_MEM_getByte((Z80_regs.R[Z80_REG_B]<<8) | Z80_regs.R[Z80_REG_C]);

	Z80Cycles=7;

	return 0;
}

U32 Z80_SBCA_n(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 dst=Z80_regs.R[Z80_REG_A];
	U8 src=Z80_MEM_getByte(Z80_regs.PC);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.PC++;

	Z80_regs.R[Z80_REG_A]=dst-src;
	if (Z80_regs.R[Z80_REG_F]&Z80_STATUS_C)
	{
		Z80_regs.R[Z80_REG_A]--;
	}

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3,Z80_regs.R[Z80_REG_A],0,Z80_STATUS_N);
	ComputeSubHCV(src,dst,Z80_regs.R[Z80_REG_A]);

	Z80Cycles=7;

	return 0;
}

U32 Z80_CB_RLCm(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 carry;
	U8 res;
	U16 memAddress = Get_HL_(Z80_regs.PC-2);
	
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	res = Z80_MEM_getByte(memAddress);
	carry = res&0x80;

	res<<=1;
	
	if (carry)
	{
		res|=0x01;
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_C;
	}
	else
	{
		res&=~0x01;
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_C;
	}
	
	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,res,Z80_STATUS_H|Z80_STATUS_N,0);

	Z80_MEM_setByte(memAddress,res);

	Z80Cycles=8;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=15;
	}

	return 0;
}

U32 Z80_CB_RRm(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 carry;
	U8 res;
	U16 memAddress = Get_HL_(Z80_regs.PC-2);
	
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	res = Z80_MEM_getByte(memAddress);
	carry = res&0x01;

	res>>=1;
	
	if (Z80_regs.R[Z80_REG_F]&Z80_STATUS_C)
	{
		res|=0x80;
	}
	else
	{
		res&=~0x80;
	}

	if (carry)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_C;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_C;
	}
	
	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,res,Z80_STATUS_H|Z80_STATUS_N,0);

	Z80_MEM_setByte(memAddress,res);

	Z80Cycles=8;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=15;
	}

	return 0;
}

U32 Z80_CB_SLAm(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 carry;
	U8 res;
	U16 memAddress = Get_HL_(Z80_regs.PC-2);
	
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	res = Z80_MEM_getByte(memAddress);
	carry = res&0x80;

	res<<=1;
	
	res&=~0x01;			/* should not be needed */

	if (carry)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_C;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_C;
	}
	
	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,res,Z80_STATUS_H|Z80_STATUS_N,0);

	Z80_MEM_setByte(memAddress,res);

	Z80Cycles=8;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=15;
	}

	return 0;
}

U32 Z80_CB_RRCm(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 carry;
	U8 res;
	U16 memAddress = Get_HL_(Z80_regs.PC-2);
	
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	res = Z80_MEM_getByte(memAddress);
	carry = res&0x01;

	res>>=1;
	
	if (carry)
	{
		res|=0x80;
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_C;
	}
	else
	{
		res&=~0x80;
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_C;
	}
	
	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,res,Z80_STATUS_H|Z80_STATUS_N,0);

	Z80_MEM_setByte(memAddress,res);

	Z80Cycles=8;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=15;
	}

	return 0;
}

U32 Z80_CB_SRLm(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U8 carry;
	U8 res;
	U16 memAddress = Get_HL_(Z80_regs.PC-2);
	
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	res = Z80_MEM_getByte(memAddress);
	carry = res&0x01;

	res>>=1;
	
	res&=~0x80;			/* should not be needed */

	if (carry)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_C;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_C;
	}
	
	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3|Z80_STATUS_P,res,Z80_STATUS_H|Z80_STATUS_N,0);

	Z80_MEM_setByte(memAddress,res);

	Z80Cycles=8;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80Cycles+=15;
	}

	return 0;
}

U32 Z80_ED_RETI(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80_regs.PC=Z80_MEM_getByte(Z80_regs.SP);
	Z80_regs.SP++;
	Z80_regs.PC|=Z80_MEM_getByte(Z80_regs.SP)<<8;
	Z80_regs.SP++;

	Z80_regs.IFF1=Z80_regs.IFF2;

	Z80Cycles=14;

	return 0;
}

U32 Z80_ED_OUTD(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 madFlags;
	U8 n;
	U16 src = (Z80_regs.R[Z80_REG_H]<<8)|Z80_regs.R[Z80_REG_L];

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	n=Z80_MEM_getByte(src);

	Z80_regs.R[Z80_REG_B]--;

	if (n&0x80)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_N;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_N;
	}

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3,Z80_regs.R[Z80_REG_B],0,0);

	Z80_IO_setByte((Z80_regs.R[Z80_REG_B]<<8)|Z80_regs.R[Z80_REG_C],n);

	src--;
	Z80_regs.R[Z80_REG_H]=src>>8;
	Z80_regs.R[Z80_REG_L]=src&0xFF;

	madFlags=n;
	madFlags+=Z80_regs.R[Z80_REG_L];

	if (madFlags>255)
	{
		ComputeFlags(Z80_STATUS_P,(madFlags&0x07)^Z80_regs.R[Z80_REG_B],0,Z80_STATUS_C|Z80_STATUS_H);
	}
	else
	{
		ComputeFlags(Z80_STATUS_P,(madFlags&0x07)^Z80_regs.R[Z80_REG_B],Z80_STATUS_C|Z80_STATUS_H,0);
	}

	Z80Cycles=16;

	return 0;
}

U32 Z80_ED_LDA_I(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80_regs.R[Z80_REG_A]=Z80_regs.IR>>8;

	ComputeFlags(Z80_STATUS_Z|Z80_STATUS_S|Z80_STATUS_X_B3|Z80_STATUS_X_B5,Z80_regs.R[Z80_REG_A],Z80_STATUS_H|Z80_STATUS_N|Z80_STATUS_PV,0);
	if (Z80_regs.IFF2)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_PV;
	}
	Z80Cycles=9;

	return 0;
}

U32 Z80_ED_LDD(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 src = (Z80_regs.R[Z80_REG_H]<<8)|Z80_regs.R[Z80_REG_L];
	U16 dst = (Z80_regs.R[Z80_REG_D]<<8)|Z80_regs.R[Z80_REG_E];
	U16 cnt = (Z80_regs.R[Z80_REG_B]<<8)|Z80_regs.R[Z80_REG_C];

	U8 byte = Z80_MEM_getByte(src);
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_MEM_setByte(dst,byte);

	src--;
	dst--;
	cnt--;

	Z80_regs.R[Z80_REG_H] = src>>8;
	Z80_regs.R[Z80_REG_L] = src&0xFF;
	Z80_regs.R[Z80_REG_D] = dst>>8;
	Z80_regs.R[Z80_REG_E] = dst&0xFF;
	Z80_regs.R[Z80_REG_B] = cnt>>8;
	Z80_regs.R[Z80_REG_C] = cnt&0xFF;

	if (cnt!=0)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_PV;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_PV;
	}

	if ((Z80_regs.R[Z80_REG_A]+byte)&0x02)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_X_B5;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_X_B5;
	}
	if ((Z80_regs.R[Z80_REG_A]+byte)&0x08)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_X_B3;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_X_B3;
	}
	ComputeFlags(0,Z80_regs.R[Z80_REG_A]+byte,Z80_STATUS_H|Z80_STATUS_N,0);
	
	Z80Cycles=16;

	return 0;
}

U32 Z80_ED_OTDR(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	Z80_ED_OUTD(stage,op1,op2,op3,op4,op5,op6,op7,op8);			/* saves some duplication */

	if (Z80_regs.R[Z80_REG_B]!=0)
	{
		Z80Cycles=21;
		Z80_regs.PC-=2;
	}
	else
	{
		Z80Cycles=16;
	}

	return 0;
}

U32 Z80_ED_INI(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 madFlags;
	U8 n;
	U16 dst = (Z80_regs.R[Z80_REG_H]<<8)|Z80_regs.R[Z80_REG_L];

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	n=Z80_IO_getByte((Z80_regs.R[Z80_REG_B]<<8)|Z80_regs.R[Z80_REG_C]);

	Z80_MEM_setByte(dst,n);

	Z80_regs.R[Z80_REG_B]--;

	if (n&0x80)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_N;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_N;
	}

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3,Z80_regs.R[Z80_REG_B],0,0);

	dst++;
	Z80_regs.R[Z80_REG_H]=dst>>8;
	Z80_regs.R[Z80_REG_L]=dst&0xFF;

	madFlags=n;
	madFlags+=(Z80_regs.R[Z80_REG_C]+1)&0xFF;

	if (madFlags>255)
	{
		ComputeFlags(Z80_STATUS_P,(madFlags&0x07)^Z80_regs.R[Z80_REG_B],0,Z80_STATUS_C|Z80_STATUS_H);
	}
	else
	{
		ComputeFlags(Z80_STATUS_P,(madFlags&0x07)^Z80_regs.R[Z80_REG_B],Z80_STATUS_C|Z80_STATUS_H,0);
	}

	Z80Cycles=16;

	return 0;
}

U32 Z80_ED_RRD(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 hl = (Z80_regs.R[Z80_REG_H]<<8)|Z80_regs.R[Z80_REG_L];
	U8 accTmp = Z80_regs.R[Z80_REG_A];
	U8 hlTmp = Z80_MEM_getByte(hl);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	accTmp&=0xF0;
	accTmp|=hlTmp&0x0F;
	hlTmp>>=4;
	hlTmp|=Z80_regs.R[Z80_REG_A]&0x0F;

	Z80_regs.R[Z80_REG_A]=accTmp;
	Z80_MEM_setByte(hl,hlTmp);

	ComputeFlags(Z80_STATUS_Z|Z80_STATUS_S|Z80_STATUS_X_B3|Z80_STATUS_X_B5|Z80_STATUS_P,Z80_regs.R[Z80_REG_A],Z80_STATUS_H|Z80_STATUS_N,0);

	Z80Cycles=18;

	return 0;
}

U32 Z80_SBCm(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 memAddress = Get_HL_(Z80_regs.PC);
	U8 dst=Z80_regs.R[Z80_REG_A];
	U8 src=Z80_MEM_getByte(memAddress);

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	Z80_regs.R[Z80_REG_A]=dst-src;
	if (Z80_regs.R[Z80_REG_F]&Z80_STATUS_C)
		Z80_regs.R[Z80_REG_A]--;

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3,Z80_regs.R[Z80_REG_A],0,Z80_STATUS_N);
	ComputeSubHCV(src,dst,Z80_regs.R[Z80_REG_A]);

	Z80Cycles=7;
	if (Z80_regs.ixAdjust||Z80_regs.iyAdjust)
	{
		Z80_regs.PC++;
		Z80Cycles+=12;
	}

	return 0;
}

U32 Z80_ED_OUTI(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 madFlags;
	U8 n;
	U16 src = (Z80_regs.R[Z80_REG_H]<<8)|Z80_regs.R[Z80_REG_L];

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	n=Z80_MEM_getByte(src);

	Z80_regs.R[Z80_REG_B]--;

	if (n&0x80)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_N;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_N;
	}

	ComputeFlags(Z80_STATUS_S|Z80_STATUS_Z|Z80_STATUS_X_B5|Z80_STATUS_X_B3,Z80_regs.R[Z80_REG_B],0,0);

	Z80_IO_setByte((Z80_regs.R[Z80_REG_B]<<8)|Z80_regs.R[Z80_REG_C],n);

	src++;
	Z80_regs.R[Z80_REG_H]=src>>8;
	Z80_regs.R[Z80_REG_L]=src&0xFF;

	madFlags=n;
	madFlags+=Z80_regs.R[Z80_REG_L];

	if (madFlags>255)
	{
		ComputeFlags(Z80_STATUS_P,(madFlags&0x07)^Z80_regs.R[Z80_REG_B],0,Z80_STATUS_C|Z80_STATUS_H);
	}
	else
	{
		ComputeFlags(Z80_STATUS_P,(madFlags&0x07)^Z80_regs.R[Z80_REG_B],Z80_STATUS_C|Z80_STATUS_H,0);
	}

	Z80Cycles=16;

	return 0;
}

U32 Z80_ED_OTIR(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	Z80_ED_OUTI(stage,op1,op2,op3,op4,op5,op6,op7,op8);			/* saves some duplication */

	if (Z80_regs.R[Z80_REG_B]!=0)
	{
		Z80Cycles=21;
		Z80_regs.PC-=2;
	}
	else
	{
		Z80Cycles=16;
	}

	return 0;
}

U32 Z80_ED_CPI(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
{
	U16 src = (Z80_regs.R[Z80_REG_H]<<8)|Z80_regs.R[Z80_REG_L];
	U16 cnt = (Z80_regs.R[Z80_REG_B]<<8)|Z80_regs.R[Z80_REG_C];

	U8 byte = Z80_MEM_getByte(src);
	U8 res = Z80_regs.R[Z80_REG_A] - byte;
	U8 n;

	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(op1);
	UNUSED_ARGUMENT(op2);
	UNUSED_ARGUMENT(op3);
	UNUSED_ARGUMENT(op4);
	UNUSED_ARGUMENT(op5);
	UNUSED_ARGUMENT(op6);
	UNUSED_ARGUMENT(op7);
	UNUSED_ARGUMENT(op8);
	ComputeSubHV(byte,Z80_regs.R[Z80_REG_A],res);
	ComputeFlags(Z80_STATUS_Z|Z80_STATUS_S,res,0,Z80_STATUS_N);

	if (Z80_regs.R[Z80_REG_F]&Z80_STATUS_H)
	{
		n=byte-1;
	}
	else
	{
		n=byte;
	}
	
	if ((n)&0x02)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_X_B5;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_X_B5;
	}
	if ((n)&0x08)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_X_B3;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_X_B3;
	}

	src++;
	cnt--;

	Z80_regs.R[Z80_REG_H] = src>>8;
	Z80_regs.R[Z80_REG_L] = src&0xFF;
	Z80_regs.R[Z80_REG_B] = cnt>>8;
	Z80_regs.R[Z80_REG_C] = cnt&0xFF;

	if (cnt!=0)
	{
		Z80_regs.R[Z80_REG_F]|=Z80_STATUS_PV;
	}
	else
	{
		Z80_regs.R[Z80_REG_F]&=~Z80_STATUS_PV;
	}

	Z80Cycles=16;

	return 0;
}

U32 Z80_ED_RETN(U32 stage,U16 op1,U16 op2,U16 op3,U16 op4,U16 op5,U16 op6,U16 op7,U16 op8)
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
	Z80_regs.PC=Z80_MEM_getByte(Z80_regs.SP);
	Z80_regs.SP++;
	Z80_regs.PC|=Z80_MEM_getByte(Z80_regs.SP)<<8;
	Z80_regs.SP++;

	Z80_regs.IFF1=Z80_regs.IFF2;

	Z80Cycles=14;

	return 0;
}


