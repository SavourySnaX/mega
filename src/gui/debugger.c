/*
 *  debugger.c

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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "config.h"

#include "platform.h"

#include "cpu.h"
#include "cpu_dis.h"
#include "z80.h"
#include "z80_dis.h"
#include "mytypes.h"
#include "debugger.h"
#include "memory.h"

#include "font.h"

extern U32 lineNo;
extern U32 colNo;

extern U8 *cRam;
extern U8 *vRam;
extern U8 *vsRam;

extern U8 VDP_Registers[0x20];
void doPixel(int x,int y,U8 colHi,U8 colLo);
void doPixel32(int x,int y,U32 colour);

extern U8 CRAM[0x200];

#if ENABLE_DEBUGGER

#define MAX_BPS	20
U32 bpAddresses[MAX_BPS]={0x30c};
U32 Z80_bpAddresses[MAX_BPS]={0x66};
int numBps=0;
int Z80_numBps=0;

int copNum=0;

void doPixelClipped(int x,int y,U8 colHi,U8 colLo);

void DrawChar(U32 x,U32 y, char c,int cMask1,int cMask2)
{
	int a,b;
	unsigned char *fontChar=&FontData[c*6*8];
	
	x*=8;
	y*=8;
	
	for (a=0;a<8;a++)
	{
		for (b=0;b<6;b++)
		{
			doPixel(x+b+1,y+a,(*fontChar) * cMask1,(*fontChar) * cMask2);
			fontChar++;
		}
	}
}

void PrintAt(int cMask1,int cMask2,U32 x,U32 y,const char *msg,...)
{
	static char tStringBuffer[32768];
	char *pMsg=tStringBuffer;
	va_list args;

	va_start (args, msg);
	vsprintf (tStringBuffer,msg, args);
	va_end (args);

	while (*pMsg)
	{
		DrawChar(x,y,*pMsg,cMask1,cMask2);
		x++;
		pMsg++;
	}
}

void DisplayWindow32(U32 x,U32 y, U32 w, U32 h, U32 colour)
{
	U32 xx,yy;
	x*=8;
	y*=8;
	w*=8;
	h*=8;
	w+=x;
	h+=y;
	for (yy=y;yy<h;yy++)
	{
		for (xx=x;xx<w;xx++)
		{
			doPixel32(xx,yy,colour);
		}
	}
}

void DisplayWindow(U32 x,U32 y, U32 w, U32 h, U8 AR, U8 GB)
{
	U32 xx,yy;
	x*=8;
	y*=8;
	w*=8;
	h*=8;
	w+=x;
	h+=y;
	for (yy=y;yy<h;yy++)
	{
		for (xx=x;xx<w;xx++)
		{
			doPixel(xx,yy,AR,GB);
		}
	}
}

extern U32 bankAddress;

void ShowZ80State(int offs)
{
	UNUSED_ARGUMENT(offs);
	DisplayWindow(0,0,66,11,0,0);
	PrintAt(0x0F,0xFF,1,1," A=%02X  B=%02X  C=%02x  D=%02x",Z80_regs.R[Z80_REG_A],Z80_regs.R[Z80_REG_B],Z80_regs.R[Z80_REG_C],Z80_regs.R[Z80_REG_D]);
	PrintAt(0x0F,0xFF,1,2," E=%02X  F=%02X  H=%02x  L=%02x",Z80_regs.R[Z80_REG_E],Z80_regs.R[Z80_REG_F],Z80_regs.R[Z80_REG_H],Z80_regs.R[Z80_REG_L]);
	PrintAt(0x0F,0xFF,1,3,"A'=%02X B'=%02X C'=%02x D'=%02x",Z80_regs.R_[Z80_REG_A],Z80_regs.R_[Z80_REG_B],Z80_regs.R_[Z80_REG_C],Z80_regs.R_[Z80_REG_D]);
	PrintAt(0x0F,0xFF,1,4,"E'=%02X F'=%02X H'=%02x L'=%02x",Z80_regs.R_[Z80_REG_E],Z80_regs.R_[Z80_REG_F],Z80_regs.R_[Z80_REG_H],Z80_regs.R_[Z80_REG_L]);
  PrintAt(0x0F,0xFF,1,5,"AF=%02X%02X BC=%02X%02X DE=%02X%02X HL=%02X%02X IR=%04X",Z80_regs.R[Z80_REG_A],Z80_regs.R[Z80_REG_F],
		Z80_regs.R[Z80_REG_B],Z80_regs.R[Z80_REG_C],Z80_regs.R[Z80_REG_D],Z80_regs.R[Z80_REG_E],Z80_regs.R[Z80_REG_H],Z80_regs.R[Z80_REG_L],Z80_regs.IR);
	PrintAt(0x0F,0xFF,1,6,"IX=%04X IY=%04X SP=%04X IFF1=%01X IFF2=%01X\n",Z80_regs.IX,Z80_regs.IY,Z80_regs.SP,Z80_regs.IFF1,Z80_regs.IFF2);
	PrintAt(0x0F,0xFF,1,8,"[  S: Z:B5: H:B3:PV: N: C ]  Interrupt Mode = %d",Z80_regs.InterruptMode);
	PrintAt(0x0F,0xFF,1,9,"[ %s:%s:%s:%s:%s:%s:%s:%s ]  Bank Address = %08X",				/* Bank address is not part of z80, but convenient to show here for now */
		   Z80_regs.R[Z80_REG_F] & 0x80 ? " 1" : " 0",
		   Z80_regs.R[Z80_REG_F] & 0x40 ? " 1" : " 0",
		   Z80_regs.R[Z80_REG_F] & 0x20 ? " 1" : " 0",
		   Z80_regs.R[Z80_REG_F] & 0x10 ? " 1" : " 0",
		   Z80_regs.R[Z80_REG_F] & 0x08 ? " 1" : " 0",
		   Z80_regs.R[Z80_REG_F] & 0x04 ? " 1" : " 0",
		   Z80_regs.R[Z80_REG_F] & 0x02 ? " 1" : " 0",
		   Z80_regs.R[Z80_REG_F] & 0x01 ? " 1" : " 0",bankAddress);
}


void ShowCPUState(int offs)
{
	DisplayWindow(0,0+offs,66,10,0,0);
    PrintAt(0x0F,0xFF,1,1+offs," D0=%08X  D1=%08X  D2=%08x  D3=%08x",cpu_regs.D[0],cpu_regs.D[1],cpu_regs.D[2],cpu_regs.D[3]);
    PrintAt(0x0F,0xFF,1,2+offs," D4=%08X  D5=%08X  D6=%08x  D7=%08x",cpu_regs.D[4],cpu_regs.D[5],cpu_regs.D[6],cpu_regs.D[7]);
    PrintAt(0x0F,0xFF,1,3+offs," A0=%08X  A1=%08X  A2=%08x  A3=%08x",cpu_regs.A[0],cpu_regs.A[1],cpu_regs.A[2],cpu_regs.A[3]);
    PrintAt(0x0F,0xFF,1,4+offs," A4=%08X  A5=%08X  A6=%08x  A7=%08x",cpu_regs.A[4],cpu_regs.A[5],cpu_regs.A[6],cpu_regs.A[7]);
    PrintAt(0x0F,0xFF,1,5+offs,"USP=%08X ISP=%08x\n",cpu_regs.USP,cpu_regs.ISP);
    PrintAt(0x0F,0xFF,1,7+offs,"          [ T1:T0: S: M:  :I2:I1:I0:  :  :  : X: N: Z: V: C ]");
    PrintAt(0x0F,0xFF,1,8+offs," SR=%04X  [ %s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s ]", cpu_regs.SR, 
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
}

#if ENABLE_32X_MODE

#include "sh2.h"
#include "sh2_memory.h"

extern SH2_State* master;
extern SH2_State* slave;

void ShowSH2State(SH2_State* cpu)
{
	DisplayWindow(0,1+0,66,10,0,0);
	PrintAt(0x0F,0xFF,1,1+1,"R00 =%08X\tR01 =%08X\tR02 =%08X\tR03 =%08X\n",cpu->R[0],cpu->R[1],cpu->R[2],cpu->R[3]);
	PrintAt(0x0F,0xFF,1,1+2,"R04 =%08X\tR05 =%08X\tR06 =%08X\tR07 =%08X\n",cpu->R[4],cpu->R[5],cpu->R[6],cpu->R[7]);
	PrintAt(0x0F,0xFF,1,1+3,"R08 =%08X\tR09 =%08X\tR10 =%08X\tR11 =%08X\n",cpu->R[8],cpu->R[9],cpu->R[10],cpu->R[11]);
	PrintAt(0x0F,0xFF,1,1+4,"R12 =%08X\tR13 =%08X\tR14 =%08X\tR15 =%08X\n",cpu->R[12],cpu->R[13],cpu->R[14],cpu->R[15]);
	PrintAt(0x0F,0xFF,1,1+5,"\n");
	PrintAt(0x0F,0xFF,1,1+6,"MACH=%08X\tMACL=%08X\tPR  =%08X\tPC  =%08X\n",cpu->MACH,cpu->MACL,cpu->PR,cpu->PC);
	PrintAt(0x0F,0xFF,1,1+7,"GBR =%08X\tVBR =%08X\n",cpu->GBR,cpu->VBR);
	PrintAt(0x0F,0xFF,1,1+9,"          [   :  :  :  :  :  : M: Q:I3:I2:I1:I0:  :  : S: T ]\n");
	PrintAt(0x0F,0xFF,1,1+10,"SR = %04X [ %s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s ]\n", cpu->SR&0xFFFF, 
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
	PrintAt(0x0F,0xFF,1,1+12,"PipeLine : %d\t%d\t%d\t%d\t%d\n",cpu->pipeLine[0].stage,cpu->pipeLine[1].stage,cpu->pipeLine[2].stage,cpu->pipeLine[3].stage,cpu->pipeLine[4].stage);
	PrintAt(0x0F,0xFF,1,1+13,"PipeLine : %04X\t%04X\t%04X\t%04X\t%04X\n",cpu->pipeLine[0].opcode,cpu->pipeLine[1].opcode,cpu->pipeLine[2].opcode,cpu->pipeLine[3].opcode,cpu->pipeLine[4].opcode);
}

extern SH2_Ins		*SH2_Information[65536];
extern SH2_Decode	SH2_DisTable[65536];

U32 SH2_GetOpcodeLength(SH2_State* cpu,U32 address)
{
	U16	opcode;
	U16	operands[8];
	U32	insCount;
	S32	a;

	opcode = SH2_Debug_Read_Word(cpu,address);

	if (SH2_Information[opcode])
	{
		for (a=0;a<SH2_Information[opcode]->numOperands;a++)
		{
			operands[a] = (opcode & SH2_Information[opcode]->operandMask[a]) >> 
									SH2_Information[opcode]->operandShift[a];
		}
	}
			
	insCount=SH2_DisTable[opcode](cpu,address+1,operands[0],operands[1],operands[2],
						operands[3],operands[4],operands[5],operands[6],operands[7]);

	return insCount;
}

extern char SH2_mnemonicData[256];

U32 SH2_DissasembleAddress(U32 x,U32 y,U32 address,SH2_State* cpu,int cursor)
{
	U32	insCount;
/*	S32	a;*/
	U32 b;
	int cMask1=0x0F,cMask2=0xFF;
	
	insCount=SH2_GetOpcodeLength(cpu,address);		/* note this also does the disassemble */
/*
	for (a=0;a<Z80_numBps;a++)
	{
		if (address == Z80_bpAddresses[a])
		{
			cMask1=0x0F;
			cMask2=0x00;
			break;
		}
	}
	*/
	if (cursor)
	{
		PrintAt(cMask1,cMask2,x,y,"%08X >",address);
	}
	else
	{
		PrintAt(cMask1,cMask2,x,y,"%08X ",address);
	}
	
	for (b=0;b<insCount+1;b++)
	{
		PrintAt(cMask1,cMask2,x+10+b*5,y,"%04X ",SH2_Debug_Read_Word(cpu,address+b*2));
	}
			
	PrintAt(cMask1,cMask2,x+30,y,"%s",SH2_mnemonicData);
	
	return insCount+1;
}

void DisplaySH2Dis(SH2_State* cpu)
{
	int a;
	U32 address = cpu->pipeLine[0].PC;

	if (cpu->pipeLine[0].stage==0)
	{
		address=cpu->PC;
	}

	DisplayWindow(0,19+0,70,12,0,0);
	for (a=0;a<10;a++)
	{
		address+=2*SH2_DissasembleAddress(1,20+a+0,address,cpu,0);
	}
}


#endif

U32 GetOpcodeLength(U32 address)
{
	U16	opcode;
	U16	operands[8];
	U32	insCount;
	S32	a;

	opcode = MEM_getWord(address);

	if (CPU_Information[opcode])
	{
		for (a=0;a<CPU_Information[opcode]->numOperands;a++)
		{
			operands[a] = (opcode & CPU_Information[opcode]->operandMask[a]) >> 
									CPU_Information[opcode]->operandShift[a];
		}
	}
			
	insCount=CPU_DisTable[opcode](address+2,operands[0],operands[1],operands[2],
						operands[3],operands[4],operands[5],operands[6],operands[7]);

	return insCount;
}

U8 Z80_MEM_getByte(U16 address);

U32 Z80_GetOpcodeLength(U32 address)
{
	U8	opcode;
	U16	operands[8];
	U32	insCount;
	S32	a;

	opcode = Z80_MEM_getByte(address);

	if (Z80_Information[opcode])
	{
		for (a=0;a<Z80_Information[opcode]->numOperands;a++)
		{
			operands[a] = (opcode & Z80_Information[opcode]->operandMask[a]) >> 
									Z80_Information[opcode]->operandShift[a];
		}
	}
			
	insCount=Z80_DisTable[opcode](address+1,operands[0],operands[1],operands[2],
						operands[3],operands[4],operands[5],operands[6],operands[7]);

	return insCount;
}


U32 Z80_DissasembleAddress(U32 x,U32 y,U32 address,int cursor)
{
	U32	insCount;
	S32	a;
	U32 b;
	int cMask1=0x0F,cMask2=0xFF;
	
	insCount=Z80_GetOpcodeLength(address);		/* note this also does the disassemble */

	for (a=0;a<Z80_numBps;a++)
	{
		if (address == Z80_bpAddresses[a])
		{
			cMask1=0x0F;
			cMask2=0x00;
			break;
		}
	}

	if (cursor)
	{
		PrintAt(cMask1,cMask2,x,y,"%08X >",address);
	}
	else
	{
		PrintAt(cMask1,cMask2,x,y,"%08X ",address);
	}
	
	for (b=0;b<insCount+1;b++)
	{
		PrintAt(cMask1,cMask2,x+10+b*5,y,"%02X ",Z80_MEM_getByte(address+b+0));
	}
			
	PrintAt(cMask1,cMask2,x+30,y,"%s",mnemonicData);
	
	return insCount+1;
}


U32 DissasembleAddress(U32 x,U32 y,U32 address,int cursor)
{
	U32	insCount;
	S32	a;
	U32 b;
	int cMask1=0x0F,cMask2=0xFF;
	
	insCount=GetOpcodeLength(address);		/* note this also does the dissasemble */

	for (a=0;a<numBps;a++)
	{
		if (address == bpAddresses[a])
		{
			cMask1=0x0F;
			cMask2=0x00;
			break;
		}
	}

	if (cursor)
	{
	PrintAt(cMask1,cMask2,x,y,"%08X >",address);
	}
	else
	{
	PrintAt(cMask1,cMask2,x,y,"%08X ",address);
	}
	
	for (b=0;b<(insCount+2)/2;b++)
	{
		PrintAt(cMask1,cMask2,x+10+b*5,y,"%02X%02X ",MEM_getByte(address+b*2+0),MEM_getByte(address+b*2+1));
	}
			
	PrintAt(cMask1,cMask2,x+30,y,"%s",mnemonicData);
	
	return insCount+2;
}

extern int g_newScreenNotify;

int g_pause=0;
U32 stageCheck=0;


int CheckKey(int key);
void ClearKey(int key);

void DisplayHelp()
{
	DisplayWindow(84,0,30,20,0,0);

	PrintAt(0x0F,0xFF,85,1,"T - Step Instruction");
	PrintAt(0x0F,0xFF,85,2,"H - Step hardware cycle");
	PrintAt(0x0F,0xFF,85,3,"G - Toggle Run in debugger");
	PrintAt(0x0F,0xFF,85,4,"<space> - Toggle Breakpoint");
	PrintAt(0x0F,0xFF,85,5,"<up/dn> - Move cursor");
	PrintAt(0x0F,0xFF,85,6,"M - Switch to cpu debug");
	PrintAt(0x0F,0xFF,85,7,"C - Show active copper");
	PrintAt(0x0F,0xFF,85,8,"P - Show cpu history");
}

void VDP_GetRegisterContents(U16 offset,char *buffer);
void IO_GetRegisterContents(U16 offset,char *buffer);

/* Debug Function */
void DrawTile(int xx,int yy,U32 address,int pal,U32 flipH,U32 flipV)
{
	int x,y;
	int colour;

	address&=0xFFFF;

	for (y=0;y<8;y++)
	{
		for (x=0;x<8;x++)
		{
			if ((x&1)==0)
			{
				colour=(vRam[address+(y*4) + (x/2)]&0xF0)>>4;
			}
			else
			{
				colour=vRam[address+(y*4) + (x/2)]&0x0F;
			}
			
			if (colour!=0)
			{
				U8 r=colour;
				U8 gb=colour|(colour<<4);

				if (pal!=-1)
				{
					gb=cRam[pal*2*16+colour*2+0]&0x0E;
					r=cRam[pal*2*16+colour*2+1]&0x0E;
					gb|=cRam[pal*2*16+colour*2+1]&0xE0;
				}
				if (flipH && flipV)
				{
					doPixel((7-x)+xx,(7-y)+yy,r,gb);
				}
				if (!flipH && flipV)
				{
					doPixel(x+xx,(7-y)+yy,r,gb);
				}
				if (flipH && !flipV)
				{
					doPixel((7-x)+xx,y+yy,r,gb);
				}
				if (!flipH && !flipV)
				{
					doPixel(x+xx,y+yy,r,gb);
				}
			}
		}
	}
}

void SMS_DrawTile(int xx,int yy,U32 address,int pal,U32 flipH,U32 flipV)
{
	int x,y,p;
	int colour;
	U8 plane[4];
	U8 planeMask;
	U8 planeShift;

	address&=0x3FFF;

	for (y=0;y<8;y++)
	{
		for (p=0;p<4;p++)
		{
			plane[p]=vRam[address+y*4+p];
		}

		planeMask=0x80;
		planeShift=7;
		for (x=0;x<8;x++)
		{
			colour=((plane[3]&planeMask)>>planeShift)<<3;
			colour|=((plane[2]&planeMask)>>planeShift)<<2;
			colour|=((plane[1]&planeMask)>>planeShift)<<1;
			colour|=((plane[0]&planeMask)>>planeShift)<<0;

			planeMask>>=1;
			planeShift--;

			if (colour!=0)		/* not sure about transparency yet on sms */
			{
				U8 r=colour;
				U8 gb=colour|(colour<<4);

				if (pal!=-1)
				{
					r=(cRam[pal*16+colour]&0x03)<<2;
					gb=(cRam[pal*16+colour]&0x0C)<<4;
					gb|=(cRam[pal*16+colour]&0x30)>>2;
				}
				if (flipH && flipV)
				{
					doPixel((7-x)+xx,(7-y)+yy,r,gb);
				}
				if (!flipH && flipV)
				{
					doPixel(x+xx,(7-y)+yy,r,gb);
				}
				if (flipH && !flipV)
				{
					doPixel((7-x)+xx,y+yy,r,gb);
				}
				if (!flipH && !flipV)
				{
					doPixel(x+xx,y+yy,r,gb);
				}
			}
		}
	}
}

/* Slow clipping tile draw for sprites (until i do it properly!) */
void DrawTileClipped(int xx,int yy,U32 address,int pal,U32 flipH,U32 flipV)
{
	int x,y;
	int colour;

	address&=0xFFFF;

	for (y=0;y<8;y++)
	{
		for (x=0;x<8;x++)
		{
			if ((x&1)==0)
			{
				colour=(vRam[address+(y*4) + (x/2)]&0xF0)>>4;
			}
			else
			{
				colour=vRam[address+(y*4) + (x/2)]&0x0F;
			}
			
			if (colour!=0)
			{
				U8 r=colour;
				U8 gb=colour|(colour<<4);

				if (pal!=-1)
				{
					gb=cRam[pal*2*16+colour*2+0]&0x0E;
					r=cRam[pal*2*16+colour*2+1]&0x0E;
					gb|=cRam[pal*2*16+colour*2+1]&0xE0;

				}
				if (flipH && flipV)
				{
					doPixelClipped((7-x)+xx,(7-y)+yy,r,gb);
				}
				if (!flipH && flipV)
				{
					doPixelClipped(x+xx,(7-y)+yy,r,gb);
				}
				if (flipH && !flipV)
				{
					doPixelClipped((7-x)+xx,y+yy,r,gb);
				}
				if (!flipH && !flipV)
				{
					doPixelClipped(x+xx,y+yy,r,gb);
				}
			}
		}
	}
}

void DisplayApproximationOfScreen(int xx,int yy,int winNumber)
{
	int x,y;
	int displaySizeX = (VDP_Registers[12]&0x01) ? 40 : 32;
	int displaySizeY = (VDP_Registers[1]&0x08) ? 30 : 28;			/* not quite true.. ntsc can never be 30! */
	int windowSizeX,windowSizeY;
	S32 scrollAmount=((vsRam[2]&0x0F)<<8)|(vsRam[3]);
	U32 address=(VDP_Registers[0x04])&0x07;
	U32 baseAddress;
	address<<=13;

	if (winNumber==0)
	{
		scrollAmount=((vsRam[0]&0x0F)<<8)|(vsRam[1]);
		address=(VDP_Registers[0x02])&0x38;
		address<<=10;
	}
	baseAddress=address;

	switch (VDP_Registers[16] & 0x30)
	{
	default:
	case 0x20:			/* Marked Prohibited */
	case 0x00:
		windowSizeY=32;
		break;
	case 0x10:
		windowSizeY=64;
		break;
	case 0x30:
		windowSizeY=128;
		break;
	}
	switch (VDP_Registers[16] & 0x03)
	{
	default:
	case 2:			/* Marked Prohibited */
	case 0:
		windowSizeX=32;
		break;
	case 1:
		windowSizeX=64;
		break;
	case 3:
		windowSizeX=128;
		break;
	}

	if (scrollAmount&0x0800)
		scrollAmount|=0xFFFFF000;

	address-=baseAddress;
	address+=(windowSizeX*2)*(scrollAmount/8);
	address&=(windowSizeY*windowSizeX*2-1);
	address+=baseAddress;
	/* Dump screen representation? */



	for (y=0;y<displaySizeY;y++)
	{
		for (x=0;x<displaySizeX;x++)
		{
			U16 tile = (vRam[address+0]<<8)|vRam[address+1];

			U16 tileAddress = tile & 0x07FF;			/* pccv hnnn nnnn nnnn */

			int pal = (tile & 0x6000)>>13;

			tileAddress <<= 5;

			DrawTile(xx+x*8,y*8+yy,tileAddress,pal,tile&0x0800,tile&0x1000);

			address+=2;
		}

		address+=(windowSizeX*2)-(displaySizeX*2);
	}
}

void SMS_DisplayApproximationOfScreen(int xx,int yy)
{
	int x,y;
	int displaySizeX = 32;
	int displaySizeY = 24;
	U32 address=(VDP_Registers[0x02])&0x0E;

	address<<=10;
	address&=0x3FFF;

	for (y=0;y<displaySizeY;y++)
	{
		for (x=0;x<displaySizeX;x++)
		{
			U16 tile = (vRam[address+1]<<8)|vRam[address+0];

			U16 tileAddress = tile & 0x01FF;			/* ---p cvhn nnnn nnnn */

			int pal = (tile & 0x0800)>>11;

			tileAddress <<= 5;

			SMS_DrawTile(xx+x*8,y*8+yy,tileAddress,pal,tile&0x0200,tile&0x0400);

			address+=2;
		}
	}
}

int tileOffset=0;
int palIndex=-1;

void DisplaySomeTiles()
{
	int x,y;
	int address=0+32*tileOffset;
	/* Dump a few tiles */

	for (y=0;y<20;y++)
	{
		for (x=0;x<40;x++)
		{
			DrawTile(70*8+x*8,y*8+40*8,address,palIndex,0,0);
			address+=32;
		}
	}
}

void SMS_DisplaySomeTiles()
{
	int x,y;
	int address=0+32*tileOffset;
	/* Dump a few tiles */

	for (y=0;y<20;y++)
	{
		for (x=0;x<40;x++)
		{
			SMS_DrawTile(70*8+x*8,y*8+40*8,address,palIndex,0,0);
			address+=32;
		}
	}
}

void DisplayPalette()
{
	int x,y;
	for (y=0;y<4;y++)
	{
		for (x=0;x<16;x++)
		{
			U8 r=0;
			U8 gb=0;

			gb|=cRam[((y*16+x)*2)+0]&0x0E;
			r=cRam[((y*16+x)*2)+1]&0x0E;
			gb|=cRam[((y*16+x)*2)+1]&0xE0;
			DisplayWindow(30+x,0x14+32+(y*2),1,1,r,gb);
		}
	}
#if ENABLE_32X_MODE
	for (y=0;y<16;y++)
	{
		for (x=0;x<16;x++)
		{
			U32 colour;
			U8 bg=0;
			U8 gr=0;
			bg=CRAM[(x+y*16)*2+0];
			gr=CRAM[(x+y*16)*2+1];

			colour=0;
			colour|=(gr&0x1F)<<(3+8+8);
			colour|=(bg&0x7C)<<(1);
			colour|=(bg&0x03)<<(6+8);
			colour|=(gr&0xE0)<<(8-2);

			DisplayWindow32(40+x,0x14+32+(y*2),1,1,colour);
		}
	}
#endif
}

void SMS_DisplayPalette()
{
	int x,y;
	for (y=0;y<2;y++)
	{
		for (x=0;x<16;x++)
		{
			U8 r=0;
			U8 gb=0;

			r=(cRam[y*16+x]&0x03)<<2;
			gb|=(cRam[y*16+x]&0x0C)<<4;
			gb|=(cRam[y*16+x]&0x30)>>2;
			DisplayWindow(30+x,0x14+32+(y*2),1,1,r,gb);
		}
	}
}

void DisplaySprites()
{
	int x,y;
	U16 baseSpriteAddress = (VDP_Registers[5]&0x7F)<<9;
	U16 totSprites = 80; /*HACK*/
	U16 spriteAddress=baseSpriteAddress;
/* Sprite Data 


    Index + 0  :   ------yy yyyyyyyy
 Index + 2  :   ----hhvv
 Index + 3  :   -lllllll
 Index + 4  :   pccvhnnn nnnnnnnn
 Index + 6  :   ------xx xxxxxxxx

 y = Vertical coordinate of sprite
 h = Horizontal size in cells (00b=1 cell, 11b=4 cells)
 v = Vertical size in cells (00b=1 cell, 11b=4 cells)
 l = Link field
 p = Priority
 c = Color palette
 v = Vertical flip
 h = Horizontal flip
 n = Sprite pattern start index
 x = Horizontal coordinate of sprite

*/

	while (--totSprites)
	{
		U16 yPos = ( (vRam[spriteAddress+0]<<8) | (vRam[spriteAddress+1]) ) & 0x03FF;
		U16 size = vRam[spriteAddress+2] & 0x0F;
		U16 link = vRam[spriteAddress+3] & 0x7F;
		U16 ctrl = ( (vRam[spriteAddress+4]<<8) | (vRam[spriteAddress+5]) ) & 0xFFFF;
		U16 xPos = ( (vRam[spriteAddress+6]<<8) | (vRam[spriteAddress+7]) ) & 0x03FF;
		int vSize = size&0x03;
		int hSize = (size&0x0C)>>2;

		/* Need to draw correct number of sprites for one line etc.*/

		for (x=0;x<=hSize;x++)
		{
			for (y=0;y<=vSize;y++)
			{
				if ((ctrl&0x0800) && (ctrl&0x1000))		/* H & V flip */
				{
					DrawTileClipped(xPos+((hSize)-x)*8,yPos+((vSize)-y)*8,((ctrl&0x07FF)<<5)+y*32 +x*(vSize+1)*32,(ctrl&0x6000)>>13,ctrl&0x0800,ctrl&0x1000);
				}
				if (((ctrl&0x0800)==0) && (ctrl&0x1000))/* V flip */
				{
					DrawTileClipped(xPos+x*8,yPos+((vSize)-y)*8,((ctrl&0x07FF)<<5)+y*32 +x*(vSize+1)*32,(ctrl&0x6000)>>13,ctrl&0x0800,ctrl&0x1000);
				}
				if ((ctrl&0x0800) && ((ctrl&0x1000)==0))/* H flip */
				{
					DrawTileClipped(xPos+((hSize)-x)*8,yPos+y*8,((ctrl&0x07FF)<<5)+y*32 +x*(vSize+1)*32,(ctrl&0x6000)>>13,ctrl&0x0800,ctrl&0x1000);
				}
				if (((ctrl&0x0800)==0) && ((ctrl&0x1000)==0))		/* no flip */
				{
					DrawTileClipped(xPos+x*8,yPos+y*8,((ctrl&0x07FF)<<5)+y*32 +x*(vSize+1)*32,(ctrl&0x6000)>>13,ctrl&0x0800,ctrl&0x1000);
				}

			}
		}

		if (link == 0)
			break;

		spriteAddress=baseSpriteAddress+link*8;
		spriteAddress&=0xFFFF;
	}
}

#if ENABLE_32X_MODE
extern U8 SH2_68K_COMM_REGISTER[16];
extern U16 FIFO_BUFFER[8];

void DisplayComm()
{
	int y;

	for (y=0;y<8;y++)
	{
		PrintAt(0x0F,0xFF,60+y*4,0x12+39,"%04X",FIFO_BUFFER[y]);
	}
	for (y=0;y<16;y++)
	{
		PrintAt(0x0F,0xFF,60+y*2,0x12+40,"%02X",SH2_68K_COMM_REGISTER[y]);
	}
}
#endif
void DisplayCustomRegs()
{
	int y;
	static char buffer[256]={0};

	DisplayWindow(0,31,14*8+1,512/16+2,0,0);
	
	for (y=0;y<0x20;y++)
	{
		VDP_GetRegisterContents(y,buffer);
		PrintAt(0x0F,0xFF,1, y+32, buffer);
	}

	for (y=0;y<0x20;y+=2)
	{
		IO_GetRegisterContents(y,buffer);
		PrintAt(0x0F,0xFF,30,y/2+32,buffer);
	}

	sprintf(buffer,"Line %03d, Col %03d",lineNo,colNo);
	PrintAt(0x0F,0xFF,30,0x12+32,buffer);

	DisplayPalette();
	DisplaySomeTiles();
	DisplayApproximationOfScreen(70*8,16*8,0);
}

extern U16	PSG_ToneNoise[4];
extern U16	PSG_ToneCounter[4];
extern U16	PSG_ToneOut[4];
extern U8		PSG_Vol[4];
extern U16	PSG_NoiseShiftRegister;

#if SMS_MODE
extern U8 SMS_MemPageRegister;
extern U32 SMS_Slot0_Bank;
extern U32 SMS_Slot1_Bank;
extern U32 SMS_Slot2_Bank;
extern U8 SMS_Ram_Control;

void SMS_DisplayCustomRegs()
{
	int y;
	static char buffer[256]={0};

	DisplayWindow(0,31,14*8+1,512/16+2,0,0);
	
	for (y=0;y<0x10;y++)
	{
		VDP_GetRegisterContents(y,buffer);
		PrintAt(0x0F,0xFF,1, y+32, buffer);
	}

	SMS_DisplayPalette();
	
	SMS_DisplaySomeTiles();
	
	SMS_DisplayApproximationOfScreen(70*8,16*8);

	for (y=0;y<4;y++)
	{
		PrintAt(0x0F,0xFF,30,y+32,"TN %04X",PSG_ToneNoise[y]);
		PrintAt(0x0F,0xFF,40,y+32,"TC %04X",PSG_ToneCounter[y]);
		PrintAt(0x0F,0xFF,50,y+32,"TO %04X",PSG_ToneOut[y]);
		PrintAt(0x0F,0xFF,60,y+32,"V  %02X",PSG_Vol[y]);
	}
	PrintAt(0x0F,0xFF,30,4+32,"Noise %04X",PSG_NoiseShiftRegister);

	PrintAt(0x0F,0xFF,30,6+32,"MemSelect %02X",SMS_MemPageRegister);
	PrintAt(0x0F,0xFF,30,7+32,"Page 0 %08X",SMS_Slot0_Bank);
	PrintAt(0x0F,0xFF,30,8+32,"Page 1 %08X",SMS_Slot1_Bank);
	PrintAt(0x0F,0xFF,30,9+32,"Page 2 %08X",SMS_Slot2_Bank);
	PrintAt(0x0F,0xFF,30,10+32,"Ram Control %02X",SMS_Ram_Control);
	
/*
	for (y=0;y<0x20;y+=2)
	{
		IO_GetRegisterContents(y,buffer);
		PrintAt(0x0F,0xFF,30,y/2+32,buffer);
	}

	sprintf(buffer,"Line %03d, Col %03d",lineNo,colNo);
	PrintAt(0x0F,0xFF,30,0x12+32,buffer);

	DisplayPalette();
*/
}
#endif

int dbMode=3;

U32 lastPC;
#define PCCACHESIZE	1000

U32	pcCache[PCCACHESIZE];
U32	cachePos=0;

void DecodePCHistory(int offs)
{
	int a;
	DisplayWindow(0,0,14*8+1,31+34,0,0);

	for (a=0;a<32+31;a++)
	{
		if (a+offs < PCCACHESIZE)
		{
			PrintAt(0x0F,0xFF,1,1+a,"%d : PC History : %08X\n",a+offs,pcCache[a+offs]);
		}
	}
}

int cpOffs = 0;
int cpuOffs = 0;
int Z80Offs = 0;
int hisOffs = 0;
			
int bpAt=0xFFFFFFFF;

void Z80_BreakpointModify(U32 address)
{
	int a;
	int b;

	for (a=0;a<Z80_numBps;a++)
	{
		if (address==Z80_bpAddresses[a])
		{
			/* Remove breakpoint */
			Z80_numBps--;

			for (b=a;b<Z80_numBps;b++)
			{
				Z80_bpAddresses[b]=Z80_bpAddresses[b+1];
			}
			return;
		}
	}

	Z80_bpAddresses[Z80_numBps]=address;
	Z80_numBps++;
}

void BreakpointModify(U32 address)
{
	int a;
	int b;

	for (a=0;a<numBps;a++)
	{
		if (address==bpAddresses[a])
		{
			/* Remove breakpoint */
			numBps--;

			for (b=a;b<numBps;b++)
			{
				bpAddresses[b]=bpAddresses[b+1];
			}
			return;
		}
	}

	bpAddresses[numBps]=address;
	numBps++;
}

int spriteOffset=0;

void DecodeSpriteTable()
{
	U16 baseSpriteAddress = (VDP_Registers[5]&0x7F)<<9;
	U16 spriteAddress;
	U16 totSpritesScreen = (VDP_Registers[12]&0x01) ? 80 : 64;
	U16 curLink =0;
	int line=0;
	int actualOffset=spriteOffset;

	DisplayWindow(0,0,14*8+1,31+34,0,0);

	spriteAddress=baseSpriteAddress;
	while (1)	
	{
		U16 yPos = ( (vRam[spriteAddress+0]<<8) | (vRam[spriteAddress+1]) ) & 0x03FF;
		U16 size = vRam[spriteAddress+2] & 0x0F;
		U16 link = vRam[spriteAddress+3] & 0x7F;
		U16 xPos = ( (vRam[spriteAddress+6]<<8) | (vRam[spriteAddress+7]) ) & 0x03FF;
		int vSize = size&0x03;
		int hSize = (size&0x0C)>>2;

		if (actualOffset==0)
		{
			PrintAt(0x0F,0xFF,1,1+line,"%d->%d : %d,%d [%d,%d]",curLink,link,xPos,yPos,(hSize+1)*8,(vSize+1)*8);

			line++;
			if (line>=64)
				break;
		}
		else
		{
			actualOffset--;
		}

		if (link == 0)
			break;

		curLink=link;
		if (curLink>=totSpritesScreen)
			break;

		spriteAddress=baseSpriteAddress+link*8;
		spriteAddress&=0xFFFF;
	}
}

void DisplayZ80Dis(int offs)
{
	int a;
	U16 address = Z80_regs.lastInstruction;

	UNUSED_ARGUMENT(offs);

		
	if (Z80_regs.stage==0)
	{
		address=Z80_regs.PC;
	}

	DisplayWindow(0,19,70,12,0,0);
	for (a=0;a<10;a++)
	{
		if (bpAt==a)
		{
			Z80_BreakpointModify(address);
			bpAt=0xFFFFFFFF;
		}

		address+=Z80_DissasembleAddress(1,20+a,address,Z80Offs==a);
	}
}

extern U8 YM2612_Regs[2][256];

void ShowYM2612()
{
	int a,b;

	DisplayWindow(60,0,60,70,0,0);

	for (b=0;b<2;b++)
	{
		for (a=0;a<32;a++)
		{
			PrintAt(0x0F,0xFF,60+b*26+ 0,a,"%02X",YM2612_Regs[b][a*8+0]);
			PrintAt(0x0F,0xFF,60+b*26+ 3,a,"%02X",YM2612_Regs[b][a*8+1]);
			PrintAt(0x0F,0xFF,60+b*26+ 6,a,"%02X",YM2612_Regs[b][a*8+2]);
			PrintAt(0x0F,0xFF,60+b*26+ 9,a,"%02X",YM2612_Regs[b][a*8+3]);
			PrintAt(0x0F,0xFF,60+b*26+12,a,"%02X",YM2612_Regs[b][a*8+4]);
			PrintAt(0x0F,0xFF,60+b*26+15,a,"%02X",YM2612_Regs[b][a*8+5]);
			PrintAt(0x0F,0xFF,60+b*26+18,a,"%02X",YM2612_Regs[b][a*8+6]);
			PrintAt(0x0F,0xFF,60+b*26+21,a,"%02X",YM2612_Regs[b][a*8+7]);
		}
	}
}

void Display68000Dis(int offs)
{
	int a;
	U32 address = cpu_regs.lastInstruction;

	if (cpu_regs.stage==0)
	{
		address=cpu_regs.PC;
	}

	DisplayWindow(0,19+offs,70,12,0,0);
	for (a=0;a<10;a++)
	{
		if (bpAt==a)
		{
			BreakpointModify(address);
			bpAt=0xFFFFFFFF;
		}

		address+=DissasembleAddress(1,20+a+offs,address,cpuOffs==a);
	}
}

void DisplayDebugger()
{
	int y;

	if (g_pause || (stageCheck==3) || (stageCheck==6))
	{
		if (dbMode==0)
		{
			ShowCPUState(0);
		
			DisplayHelp();
		
			DisplayCustomRegs();
#if ENABLE_32X_MODE
			DisplayComm();
#endif	
			Display68000Dis(0);
		}
		if (dbMode==1)
		{
			DecodePCHistory(hisOffs);
		}
		if (dbMode==2)
		{
			DecodeSpriteTable();
		}
		if (dbMode==3)
		{
			ShowZ80State(0);
		
			DisplayZ80Dis(0);

#if !SMS_MODE
			ShowCPUState(32);
			
			Display68000Dis(32);

			ShowYM2612();

			for (y=0;y<4;y++)
			{
				PrintAt(0x0F,0xFF,60,y+32,"TN %04X",PSG_ToneNoise[y]);
				PrintAt(0x0F,0xFF,70,y+32,"TC %04X",PSG_ToneCounter[y]);
				PrintAt(0x0F,0xFF,80,y+32,"TO %04X",PSG_ToneOut[y]);
				PrintAt(0x0F,0xFF,90,y+32,"V  %02X",PSG_Vol[y]);
			}
			PrintAt(0x0F,0xFF,60,4+32,"Noise %04X",PSG_NoiseShiftRegister);

#else
			SMS_DisplayCustomRegs();
#endif
#if ENABLE_32X_MODE
			DisplayComm();
#endif
		}
#if ENABLE_32X_MODE
		if (dbMode==DEB_Mode_SH2_Master)
		{
			ShowSH2State(master);

			DisplaySH2Dis(master);
			
			DisplayComm();
		}
		if (dbMode==DEB_Mode_SH2_Slave)
		{
			ShowSH2State(slave);

			DisplaySH2Dis(slave);
			
			DisplayComm();
		}
#endif
		
		g_newScreenNotify=1;
	}
}

extern U32 Z80Cycles;

#define ENABLE_PC_HISTORY		0
int UpdateDebugger()
{
	int a;

	if (cpu_regs.stage==0)
	{
#if ENABLE_PC_HISTORY
		lastPC=cpu_regs.PC;

		if (cachePos<PCCACHESIZE)
		{
			if (pcCache[cachePos]!=lastPC)
			{
				pcCache[cachePos++]=lastPC;
			}
		}
		else
		{
			if (pcCache[PCCACHESIZE-1]!=lastPC)
			{
				memmove(pcCache,pcCache+1,(PCCACHESIZE-1)*sizeof(U32));

				pcCache[PCCACHESIZE-1]=lastPC;
			}
		}
#endif

		for (a=0;a<numBps;a++)
		{
			if ((bpAddresses[a]==cpu_regs.PC))
			{
				if (!g_pause)
				{
					dbMode=0;
				}
				g_pause=1;
			}
		}
	}
	if (Z80_regs.stage==0)
	{
		for (a=0;a<Z80_numBps;a++)
		{
			if ((Z80_bpAddresses[a]==Z80_regs.PC))
			{
				if (!g_pause)
				{
					dbMode=3;
				}
				g_pause=1;
			}
		}
	}

	if (CheckKey(GLFW_KEY_PAGEUP))
		g_pause=!g_pause;

	if (CheckKey(GLFW_KEY_KP_0))
	{
		/* Save state */
		FILE *save=fopen("mega_save.dmp","wb");

		MEM_SaveState(save);
		CPU_SaveState(save);

		fclose(save);
	}
	if (CheckKey(GLFW_KEY_KP_MULTIPLY))
	{
		/* Load state */

		FILE *load=fopen("mega_save.dmp","rb");

		MEM_LoadState(load);
		CPU_LoadState(load);

		fclose(load);
	}

	if (stageCheck)
	{
		if (stageCheck==1)
		{
			if (cpu_regs.stage==0)
			{
				g_pause=1;		/* single step cpu */
				stageCheck=0;
			}
		}
		if (stageCheck==2)
		{
			g_pause=1;			/* single step hardware */
			stageCheck=0;
		}
		if (stageCheck==4)
		{
			if ((Z80_regs.stage==0) && (Z80Cycles==0))
			{
				g_pause=1;
				stageCheck=0;
			}
		}
		if (stageCheck==5)
		{
			g_pause=1;
			stageCheck=0;
		}

		if (stageCheck==(DEB_Mode_SH2_Master+1))
		{
			g_pause=1;
			stageCheck=0;
		}
	}

	if (g_pause || (stageCheck==3) || (stageCheck==6) || (stageCheck==9))
	{
		if (dbMode==DEB_Mode_SH2_Master || dbMode==DEB_Mode_SH2_Slave)
		{
			if (CheckKey('T'))
			{
				stageCheck=DEB_Mode_SH2_Master+1;
				g_pause=0;
			}
		}
		if (dbMode==3)
		{
			/* While paused - enable debugger keys */
			if (CheckKey('T'))
			{
				/* cpu step into instruction */
				stageCheck=4;

				g_pause=0;
			}
			if (CheckKey('H'))
			{
				stageCheck=5;
				g_pause=0;
			}
			if (CheckKey('G'))
			{
				if (stageCheck==6)
				{
					stageCheck=0;
					g_pause=1;
				}
				else
				{
					stageCheck=6;
					g_pause=0;
				}
			}
			if (CheckKey(' '))
				bpAt=Z80Offs;
			if (CheckKey(GLFW_KEY_UP) && Z80Offs>0)
				Z80Offs--;
			if (CheckKey(GLFW_KEY_DOWN) && Z80Offs<9)
				Z80Offs++;
		}

		if (CheckKey('9'))
			dbMode=DEB_Mode_SH2_Master;
		if (CheckKey('0'))
			dbMode=DEB_Mode_SH2_Slave;

		if (CheckKey('P'))
			dbMode=1;
		if (dbMode==1)
		{
			if (CheckKey(GLFW_KEY_UP))
			{
				hisOffs-=32;
				if (hisOffs<0)
					hisOffs=0;
			}
			if (CheckKey(GLFW_KEY_DOWN))
			{
				hisOffs+=32;
				if (hisOffs>=PCCACHESIZE)
					hisOffs=PCCACHESIZE-1;
			}
		}
		if (CheckKey('C'))
			Z80_Reset();

		if (CheckKey('X'))
			dbMode=2;
		if (dbMode==2)
		{
			if (CheckKey('W'))
			{
				spriteOffset--;
				if (spriteOffset<=0)
					spriteOffset=0;
			}
			if (CheckKey('S'))
			{
				spriteOffset++;
			}
		}
		if (CheckKey('N'))
			dbMode=3;
		if (CheckKey('M'))
			dbMode=0;
		if (dbMode==0)
		{
			/* While paused - enable debugger keys */
			if (CheckKey('T'))
			{
				/* cpu step into instruction */
				stageCheck=1;

				g_pause=0;
			}
			if (CheckKey('H'))
			{
				stageCheck=2;
				g_pause=0;
			}
			if (CheckKey('G'))
			{
				if (stageCheck==3)
				{
					stageCheck=0;
					g_pause=1;
				}
				else
				{
					stageCheck=3;
					g_pause=0;
				}
			}
			if (CheckKey('Q'))
			{
				palIndex++;
				if (palIndex>3)
					palIndex=-1;
			}
			if (CheckKey('A'))
			{
				tileOffset-=20*32;
			}
			if (CheckKey('Z'))
			{
				tileOffset+=20*32;
			}
			if (CheckKey(GLFW_KEY_UP) && cpuOffs>0)
				cpuOffs--;
			if (CheckKey(GLFW_KEY_DOWN) && cpuOffs<9)
				cpuOffs++;
			if (CheckKey(' '))
				bpAt=cpuOffs;
		}
		ClearKey(' ');
		ClearKey(GLFW_KEY_LEFT);
		ClearKey(GLFW_KEY_UP);
		ClearKey(GLFW_KEY_DOWN);
		ClearKey('P');
		ClearKey('C');
		ClearKey('N');
		ClearKey('M');
		ClearKey('T');
		ClearKey('H');
		ClearKey('G');
		ClearKey('W');
		ClearKey('S');
		ClearKey('X');
		ClearKey('A');
		ClearKey('Z');
		ClearKey('Q');
		ClearKey('9');
		ClearKey('0');
	}
	ClearKey(GLFW_KEY_KP_0);
	ClearKey(GLFW_KEY_KP_MULTIPLY);
	ClearKey(GLFW_KEY_PAGEUP);

	return g_pause;
}

void DEB_PauseEmulation(int pauseMode,char *reason)
{
	dbMode=pauseMode;
	g_pause=1;
	printf("Invoking debugger due to %s\n",reason);
}
#endif

/*----------------------------*/

/* VIDEO STUFF */

extern U8 videoMemory[LINE_LENGTH*HEIGHT*sizeof(U32)];

void DisplayFillBGColourHiLoLine(U8 colHi,U8 colLo,U32 *pixelPos,int w)
{
	int a;
	U32 colour;
	U8 r = (colHi&0x0F)<<4;
	U8 g = (colLo&0xF0);
	U8 b = (colLo&0x0F)<<4;

	colour = (r<<16) | (g<<8) | (b<<0);

	for (a=0;a<w;a++)
	{
		*pixelPos++=colour;
	}
}

extern U8 *cRam;

void DisplayFillBGColourLine(U32 *pixelPos,int w)
{
	U8 pal = (VDP_Registers[7]&0x30)>>4;
	U8 colour = VDP_Registers[7]&0x0F;

	U8 r=colour;
	U8 gb=colour|(colour<<4);

	gb=cRam[pal*2*16+colour*2+0]&0x0E;
	r=cRam[pal*2*16+colour*2+1]&0x0E;
	gb|=cRam[pal*2*16+colour*2+1]&0xE0;

	DisplayFillBGColourHiLoLine(r,gb,pixelPos,w);
}

void SMS_DisplayFillBGColourLine(U32 *pixelPos,int w)
{
	U8 pal = 1;
	U8 colour = VDP_Registers[7]&0x0F;

	U8 r=0;
	U8 gb=0;

	r=(cRam[pal*16+colour]&0x03)<<2;
	gb|=(cRam[pal*16+colour]&0x0C)<<4;
	gb|=(cRam[pal*16+colour]&0x30)>>2;

	DisplayFillBGColourHiLoLine(r,gb,pixelPos,w);
}

int ComputeColourXY(int tx,int ty,U32 address,U32 flipH,U32 flipV)
{
	int colour;
	int odd;

	if (flipV)
	{
		address+=(7-ty)*4;
	}
	else
	{
		address+=ty*4;
	}
	if (flipH)
	{
		address+=(7-tx)/2;
		odd=(7-tx)&1;
	}
	else
	{
		address+=tx/2;
		odd=tx&1;
	}
	address&=0xFFFF;

	if (odd==0)
	{
		colour=(vRam[address]&0xF0)>>4;
	}
	else
	{
		colour=vRam[address]&0x0F;
	}
	
	return colour;
}

U8	zBuffer[40*8];

void doPixelZ(int x,int y,U8 colHi,U8 colLo,U8 zValue)
{
	if (zBuffer[x-128]<zValue)
	{
		zBuffer[x-128]=zValue;
		doPixel(x,y,colHi,colLo);
	}
}

/*void doPixelZ(int x,int y,U8 colHi,U8 colLo,U8 zValue,U32 *pixelPos,U8 *zPos)
{
	if (*zPos<zValue)
	{
		*zPos=zValue;
		doPixel(x,y,colHi,colLo);
	}
}*/

/* ty should be 0-7; */
/* tx should be 0-7; */
void DrawTileXY(int tx,int ty,U32 address,int pal,U32 flipH,U32 flipV,U8 zValue,U32 *pixelPos,U8 *zPos)
{
	if (*zPos<zValue)
	{
		int colour = ComputeColourXY(tx,ty,address,flipH,flipV);

		if (colour!=0)
		{
			U8 r=colour;
			U8 g=colour;
			U8 b=colour;
			U32 realColour;

			*zPos=zValue;

			r=(cRam[pal*2*16+colour*2+1]&0x0E)<<4;
			g=(cRam[pal*2*16+colour*2+0]&0x0E)<<4;
			b=(cRam[pal*2*16+colour*2+1]&0xE0);

			realColour = (r<<16)|(b<<8)|g;

			*pixelPos=realColour;
		}
	}
}

void DrawScreenRow(int ty,int winNumber,U8 zValue,U32 *pixelPos,U8 *zPos)
{
	int tx,ox;
	int displaySizeX = (VDP_Registers[12]&0x01) ? 40 : 32;
	int windowSizeX,windowSizeY;
	S32 scrollAmountY=((vsRam[2]&0x0F)<<8)|(vsRam[3]);
	U32 scrollHTable = (VDP_Registers[13]&0x3F)<<10;
	S32 scrollAmountX;
	U32 address=(VDP_Registers[0x04])&0x07;
	U32 baseAddress;
	address<<=13;
	if ((VDP_Registers[11]&0x03)==0x03)			/* do pixel scroll */
	{
		scrollAmountX = ((vRam[scrollHTable+2 + ty*4]&0x07)<<8)|vRam[scrollHTable+3+ty*4];
	}
	else
	{
		if ((VDP_Registers[11]&0x03)==0x02)			/* do cell scroll */
		{
			scrollAmountX = ((vRam[scrollHTable+2 + (ty/8)*32]&0x07)<<8)|vRam[scrollHTable+3+(ty/8)*32];
		}
		else
		{
			scrollAmountX = ((vRam[scrollHTable+2]&0x07)<<8)|vRam[scrollHTable+3];
		}
	}
	if (winNumber==0)
	{
		if ((VDP_Registers[11]&0x03) == 0x03)			/* do pixel scroll */
		{
			scrollAmountX = ((vRam[scrollHTable+0+ty*4]&0x07)<<8)|vRam[scrollHTable+1+ty*4];
		}
		else
		{
			if ((VDP_Registers[11]&0x03)==0x02)			/* do cell scroll */
			{
				scrollAmountX = ((vRam[scrollHTable+0+(ty/8)*32]&0x07)<<8)|vRam[scrollHTable+1+(ty/8)*32];
			}
			else
			{
				scrollAmountX = ((vRam[scrollHTable+0]&0x07)<<8)|vRam[scrollHTable+1];
			}
		}
		scrollAmountY=((vsRam[0]&0x0F)<<8)|(vsRam[1]);
		address=(VDP_Registers[0x02])&0x38;
		address<<=10;
		if (VDP_Registers[0x12]&0x9F)
		{
			/* window enabled */
			U8 windowLine = (VDP_Registers[0x12]&0x1F)<<3;
			U8 useWindow=0;

			if (VDP_Registers[0x12]&0x80)
			{
				if (ty>=windowLine)
				{
					useWindow=1;
				}
			}
			else
			{
				if (ty<windowLine)
				{
					useWindow=1;
				}
			}

			if (useWindow)
			{
				scrollAmountY=0;
				scrollAmountX=0;
				address=(VDP_Registers[0x03])&0x3E;		/* todo 40 cell clamp to 3C */
				address<<=10;
			}
		}
	}
	baseAddress=address;

	switch (VDP_Registers[16] & 0x30)
	{
	default:
	case 0x20:			/* Marked Prohibited */
	case 0x00:
		windowSizeY=32;
		break;
	case 0x10:
		windowSizeY=64;
		break;
	case 0x30:
		windowSizeY=128;
		break;
	}
	switch (VDP_Registers[16] & 0x03)
	{
	default:
	case 2:			/* Marked Prohibited */
	case 0:
		windowSizeX=32;
		break;
	case 1:
		windowSizeX=64;
		break;
	case 3:
		windowSizeX=128;
		break;
	}

	if (scrollAmountY&0x0800)
		scrollAmountY|=0xFFFFF000;
	if (scrollAmountX&0x0400)
		scrollAmountX|=0xFFFFF800;

	ty+=scrollAmountY;
	ty&=(windowSizeY*8-1);

	for (ox=0;ox<displaySizeX*8;ox++)			/* base address is vertical adjusted..  */
	{
		U16 tile;
		U16 tileAddress;
		int pal;
		
		tx=ox - scrollAmountX;
		tx&=(windowSizeX*8-1);
		address = baseAddress+(tx/8)*2 + (ty/8)*windowSizeX*2;
		address&=0xFFFF;
		tile = (vRam[address+0]<<8)|vRam[address+1];
		tileAddress = tile & 0x07FF;			/* pccv hnnn nnnn nnnn */

		pal = (tile & 0x6000)>>13;

		tileAddress <<= 5;

		if (tile&0x8000)
		{
			DrawTileXY((tx&7),(ty&7),tileAddress,pal,tile&0x0800,tile&0x1000,zValue<<4,pixelPos,zPos);
		}
		else
		{
			DrawTileXY((tx&7),(ty&7),tileAddress,pal,tile&0x0800,tile&0x1000,zValue,pixelPos,zPos);
		}

		pixelPos++;
		zPos++;
	}
}

/* we need at least : pcc0XXXX			(p priority - cc colour palette - XXXX colour from tile) */
U8	spriteTempBuffer[40*8];		/* 1 Full width line		(VDP must compute this or similar buffer a line ahead?) */

void ProcessSpritePixel(int x,int y,U8 attributes,int ScreenY)
{
	if (y==ScreenY)
	{
		if (x>=128 && x<=128+40*8)
		{
			if (spriteTempBuffer[x-128]==0)
			{
				spriteTempBuffer[x-128]=attributes;
			}
		}
	}
}

/* Slow - but easy to test quickly */
void ProcessSpriteTile(int xx,int yy,U32 address,int pal,U32 flipH,U32 flipV,int ScreenY,U8 highNibbleOfSpriteInfo)
{
	int x,y;
	int colour;

	UNUSED_ARGUMENT(pal);

	address&=0xFFFF;

	for (y=0;y<8;y++)
	{
		for (x=0;x<8;x++)
		{
			if ((x&1)==0)
			{
				colour=(vRam[address+(y*4) + (x/2)]&0xF0)>>4;
			}
			else
			{
				colour=vRam[address+(y*4) + (x/2)]&0x0F;
			}
			
			if (colour!=0)
			{
				colour|=highNibbleOfSpriteInfo;
				if (flipH && flipV)
				{
					ProcessSpritePixel((7-x)+xx,(7-y)+yy,colour,ScreenY);
				}
				if (!flipH && flipV)
				{
					ProcessSpritePixel(x+xx,(7-y)+yy,colour,ScreenY);
				}
				if (flipH && !flipV)
				{
					ProcessSpritePixel((7-x)+xx,y+yy,colour,ScreenY);
				}
				if (!flipH && !flipV)
				{
					ProcessSpritePixel(x+xx,y+yy,colour,ScreenY);
				}
			}
		}
	}
}

void ComputeSpritesForNextLine(int nextLine)
{
	int x,y;
	U16 baseSpriteAddress = (VDP_Registers[5]&0x7F)<<9;
	int displaySizeX = (VDP_Registers[12]&0x01) ? 40 : 32;
	U16 spriteAddress;
	U16 totSpritesScreen = (VDP_Registers[12]&0x01) ? 80 : 64;
	U16 totSpritesScan = (VDP_Registers[12]&0x01) ? 20 : 16;
	U16 totPixelsScan = (VDP_Registers[12]&0x01) ? 20*16 : 16*16;
	U16 curLink =0;

	int	bailOut=80;			/* bug in sprite stuff - infinite loop this will clear after 80  */

	memset(spriteTempBuffer,0,displaySizeX*8);			/* clear processing buffer down */

/* Sprite Data 


    Index + 0  :   ------yy yyyyyyyy
 Index + 2  :   ----hhvv
 Index + 3  :   -lllllll
 Index + 4  :   pccvhnnn nnnnnnnn
 Index + 6  :   ------xx xxxxxxxx

 y = Vertical coordinate of sprite
 h = Horizontal size in cells (00b=1 cell, 11b=4 cells)
 v = Vertical size in cells (00b=1 cell, 11b=4 cells)
 l = Link field
 p = Priority
 c = Color palette
 v = Vertical flip
 h = Horizontal flip
 n = Sprite pattern start index
 x = Horizontal coordinate of sprite


NB : sprite with x coord=0 masks off lower sprites for scanlines this sprite would occupy

*/
	spriteAddress=baseSpriteAddress;
	while (totSpritesScan && bailOut)
	{
		U16 yPos = ( (vRam[spriteAddress+0]<<8) | (vRam[spriteAddress+1]) ) & 0x03FF;
		U16 size = vRam[spriteAddress+2] & 0x0F;
		U16 link = vRam[spriteAddress+3] & 0x7F;
		U16 ctrl = ( (vRam[spriteAddress+4]<<8) | (vRam[spriteAddress+5]) ) & 0xFFFF;
		U16 xPos = ( (vRam[spriteAddress+6]<<8) | (vRam[spriteAddress+7]) ) & 0x03FF;
		int vSize = size&0x03;
		int hSize = (size&0x0C)>>2;
		U8 attributeHighNibble = (ctrl&0xE000)>>8;

		bailOut--;

		if ((nextLine>=yPos) && (nextLine<=(yPos+((8*(1+vSize))-1))))			/* not interested if not current line */
		{
			if (xPos==0 && curLink!=0)			/* Special masking mode.. check if curLine is within sprite Y range, if it is */
			{						/*stop processing list - sprite 0 does not count */
				break;
			}
			/* Need to draw correct number of sprites for one line etc. */

			totSpritesScan--;

			for (x=0;x<=hSize;x++)
			{
				for (y=0;y<=vSize;y++)
				{
					if ((ctrl&0x0800) && (ctrl&0x1000))		/* H & V flip */
					{
						ProcessSpriteTile(xPos+((hSize)-x)*8,yPos+((vSize)-y)*8,((ctrl&0x07FF)<<5)+y*32 +x*(vSize+1)*32,(ctrl&0x6000)>>13,ctrl&0x0800,ctrl&0x1000,nextLine,attributeHighNibble);
					}
					if (((ctrl&0x0800)==0) && (ctrl&0x1000))	/* V flip */
					{
						ProcessSpriteTile(xPos+x*8,yPos+((vSize)-y)*8,((ctrl&0x07FF)<<5)+y*32 +x*(vSize+1)*32,(ctrl&0x6000)>>13,ctrl&0x0800,ctrl&0x1000,nextLine,attributeHighNibble);
					}
					if ((ctrl&0x0800) && ((ctrl&0x1000)==0))	/* H flip */
					{
						ProcessSpriteTile(xPos+((hSize)-x)*8,yPos+y*8,((ctrl&0x07FF)<<5)+y*32 +x*(vSize+1)*32,(ctrl&0x6000)>>13,ctrl&0x0800,ctrl&0x1000,nextLine,attributeHighNibble);
					}
					if (((ctrl&0x0800)==0) && ((ctrl&0x1000)==0))	/* no flip */
					{
						ProcessSpriteTile(xPos+x*8,yPos+y*8,((ctrl&0x07FF)<<5)+y*32 +x*(vSize+1)*32,(ctrl&0x6000)>>13,ctrl&0x0800,ctrl&0x1000,nextLine,attributeHighNibble);
					}
				}
				totPixelsScan-=8;
				if (!totPixelsScan)
				{
					break;
				}
				if (x==2)
				{
					if (!totSpritesScan)
					{
						break;
					}
					totSpritesScan--;
				}
			}
		}

		if (link == 0)
			break;

		curLink=link;
		if (curLink>=totSpritesScreen)
			break;

		spriteAddress=baseSpriteAddress+link*8;
		spriteAddress&=0xFFFF;
	}
}

void DrawSpritesForLine(int curLine,U8 zValue)
{
	int x;
	int displaySizeX = (VDP_Registers[12]&0x01) ? 40 : 32;

	for (x=0;x<displaySizeX*8;x++)
	{
		U8 spriteInfo = spriteTempBuffer[x];

		if (spriteInfo!=0)
		{
			int pal = (spriteInfo&0x60)>>5;
			int colour = spriteInfo&0x0F;
			U8 r=colour;
			U8 gb=colour|(colour<<4);

			gb=cRam[pal*2*16+colour*2+0]&0x0E;
			r=cRam[pal*2*16+colour*2+1]&0x0E;
			gb|=cRam[pal*2*16+colour*2+1]&0xE0;

			if (spriteInfo&0x80)
			{
				doPixelZ(128+x,curLine,r,gb,zValue<<4);
			}
			else
			{
				doPixelZ(128+x,curLine,r,gb,zValue);
			}
		}
	}
}

U32* pixelPosition(int x,int y);

#if ENABLE_32X_MODE

extern U8 FRAMEBUFFER[0x00040000];

extern U32 ActiveFrameBuffer;
extern U16 BitmapModeRegister;

#define USE_PALETTE		1

void Draw32XScreenRowPalette(U32* out,U8* displayPtr,int displaySizeX)
{
	int x;
#if USE_PALETTE
	U8 gr,bg;
#endif
	U32 colour;
	U8 pixel;

	for (x=0;x<displaySizeX*8;x++)
	{
		pixel = *displayPtr++;

#if !USE_PALETTE
		colour = (pixel<<16)+(pixel<<8)+(pixel);
#else
		bg=CRAM[pixel*2+0];
		gr=CRAM[pixel*2+1];

		colour=0;
		colour|=(gr&0x1F)<<(3+8+8);
		colour|=(bg&0x7C)<<(1);
		colour|=(bg&0x03)<<(6+8);
		colour|=(gr&0xE0)<<(8-2);
#endif
		*out++=colour;
	}
}

void Draw32XScreenRowDirect(U32* out,U8* displayPtr,int displaySizeX)
{
	int x;
	U8 gr,bg;
	U32 colour;
/*	U8 pixel;*/

	for (x=0;x<displaySizeX*8;x++)
	{
		bg = *displayPtr++;
		gr = *displayPtr++;

		colour=0;
		colour|=(gr&0x1F)<<(3+8+8);
		colour|=(bg&0x7C)<<(1);
		colour|=(bg&0x03)<<(6+8);
		colour|=(gr&0xE0)<<(8-2);
		*out++=colour;
	}
}

void Draw32XScreenRowRLE(U32* out,U8* displayPtr,int displaySizeX)
{
	int x=0;
	U8 gr,bg;
	U32 colour=0;
	U8 pixel;
	U16 rle=0;

	while (x<displaySizeX*8)
	{
		if (rle==0)
		{
			rle = *displayPtr++;
			pixel = *displayPtr++;

			bg=CRAM[pixel*2+0];
			gr=CRAM[pixel*2+1];

			colour=0;
			colour|=(gr&0x1F)<<(3+8+8);
			colour|=(bg&0x7C)<<(1);
			colour|=(bg&0x03)<<(6+8);
			colour|=(gr&0xE0)<<(8-2);

			rle++;
			continue;
		}
		else
		{
			rle--;
		}

		*out++=colour;
		x++;
	}
}


void Draw32XScreenRow(int y,U32* out,U8* zPos)
{
	int displaySizeX = (VDP_Registers[12]&0x01) ? 40 : 32;
	U16 lineOffset;
	U8* displayPtr;

	UNUSED_ARGUMENT(zPos);

	if (!ActiveFrameBuffer)
		displayPtr=&FRAMEBUFFER[0x20000];
	else
		displayPtr=&FRAMEBUFFER[0];

#if 0
	lineOffset = FRAMEBUFFER[y*2+0]<<8;
	lineOffset|= FRAMEBUFFER[y*2+1];
#else
	lineOffset = displayPtr[y*2+0]<<8;
	lineOffset|= displayPtr[y*2+1];
#endif

	displayPtr+=lineOffset*2;

	switch (BitmapModeRegister&0x0003)
	{
	case 0:
		return;
	case 1:
		Draw32XScreenRowPalette(out,displayPtr,displaySizeX);
		return;
	case 2:
		Draw32XScreenRowDirect(out,displayPtr,displaySizeX);
		return;
	case 3:
		Draw32XScreenRowRLE(out,displayPtr,displaySizeX);
		return;
	}
}

#endif

#if !SMS_MODE
void VID_DrawScreen(int y)
{
	{
		U32 *pixel = pixelPosition(128,128+y);
		U8 *zPos = zBuffer;

		DisplayFillBGColourLine(pixel,320);

		memset(zBuffer,0,40*8);
#if ENABLE_32X_MODE
		Draw32XScreenRow(y,pixel,zPos);
#endif
		DrawScreenRow(y,0,0x02,pixel,zPos);
		DrawScreenRow(y,1,0x01,pixel,zPos);
		DrawSpritesForLine(128+y,0x04);
		ComputeSpritesForNextLine(128+y+1);
	}
}
#else

int SMS_ComputeColourXY(int tx,int ty,U32 address,U32 flipH,U32 flipV)
{
	int colour;
	U8 mask=0x80;
	int planeShift=7,p;
	U8 plane[4];

	if (flipV)
	{
		address+=(7-ty)*4;
	}
	else
	{
		address+=ty*4;
	}
	if (flipH)
	{
		mask>>=7-tx;
		planeShift-=7-tx;
	}
	else
	{
		mask>>=tx;
		planeShift-=tx;
	}
	address&=0x3FFF;

	for (p=0;p<4;p++)
	{
		plane[p]=vRam[address+p];
	}

	colour=((plane[3]&mask)>>planeShift)<<3;
	colour|=((plane[2]&mask)>>planeShift)<<2;
	colour|=((plane[1]&mask)>>planeShift)<<1;
	colour|=((plane[0]&mask)>>planeShift)<<0;

	return colour;
}

void SMS_DrawTileXY(int tx,int ty,U32 address,int pal,U32 flipH,U32 flipV,U8 zValue,U32 *pixelPos,U8 *zPos)
{
	if (*zPos<zValue)
	{
		int colour = SMS_ComputeColourXY(tx,ty,address,flipH,flipV);

		/*if (colour!=0)			// seems the sms draws colour 0 - so we skip setting the zpos (which keeps the transparent value for sprite priority) */
		{
			U8 r=colour;
			U8 g=colour;
			U8 b=colour;
			U32 realColour;

			if (colour!=0)
			{
				*zPos=zValue;
			}

			r=(cRam[pal*16+colour]&0x03)<<6;
			b=(cRam[pal*16+colour]&0x0C)<<4;
			g=(cRam[pal*16+colour]&0x30)<<2;

			realColour = (r<<16)|(b<<8)|g;

			*pixelPos=realColour;
		}
	}
}

S32 latchedScrollAmountY;

void SMS_DrawScreenRow(int ty,U8 zValue,U32 *pixelPos,U8 *zPos)
{
	int tx,ox,oty=ty,uty;
	int displaySizeX = 32;
	S32 scrollAmountY=latchedScrollAmountY;
	S32 scrollAmountX=VDP_Registers[0x08];
	U32 address=(VDP_Registers[0x02])&0x0E;
	U32 baseAddress;
	address<<=10;
	baseAddress=address;

	ty+=scrollAmountY;
	if (ty>223)
	{
		ty-=224;
	}
	
	ox=0;
	if (VDP_Registers[0x00]&0x20)			/* left column disabled */
	{
		ox=8;
		pixelPos+=8;
		zPos+=8;
	}

	for (;ox<displaySizeX*8;ox++)			/* base address is vertical adjusted..  */
	{
		U16 tile;
		U16 tileAddress;
		int pal;
		
		if ((oty<16) && (VDP_Registers[0x00]&0x40))
		{
			tx=ox;
		}
		else
		{
			tx=ox - scrollAmountX;
			tx&=(32*8-1);
		}
		if ((ox>=24*8) && (VDP_Registers[0x00]&0x80))
		{
			uty=oty;
		}
		else
		{
			uty=ty;
		}
		address = baseAddress+(tx/8)*2 + (uty/8)*32*2;
		address&=0x3FFF;

		tile = (vRam[address+1]<<8)|vRam[address+0];

		tileAddress = tile & 0x01FF;			/* ---p cvhn nnnn nnnn */

		pal = (tile & 0x0800)>>11;

		tileAddress <<= 5;

		if (tile&0x1000)
		{
			SMS_DrawTileXY((tx&7),(uty&7),tileAddress,pal,tile&0x0200,tile&0x0400,zValue<<4,pixelPos,zPos);
		}
		else
		{
			SMS_DrawTileXY((tx&7),(uty&7),tileAddress,pal,tile&0x0200,tile&0x0400,zValue,pixelPos,zPos);
		}
		pixelPos++;
		zPos++;
	}
}

void SMS_ProcessSpriteTile(int xx,int yy,U32 address,int ScreenY)
{
	int x,y;
	int colour;

	address&=0x3FFF;

	for (y=0;y<8;y++)
	{
		U8 mask=0x80;
		int planeShift=7,p;
		U8 plane[4];
	
		for (p=0;p<4;p++)
		{
			plane[p]=vRam[address+(y*4)+p];
		}

		for (x=0;x<8;x++)
		{
			colour=((plane[3]&mask)>>planeShift)<<3;
			colour|=((plane[2]&mask)>>planeShift)<<2;
			colour|=((plane[1]&mask)>>planeShift)<<1;
			colour|=((plane[0]&mask)>>planeShift)<<0;

			mask>>=1;
			planeShift--;

			if (colour!=0)
			{
				ProcessSpritePixel(x+xx,y+yy,colour,ScreenY);
			}
		}
	}
}

void SMS_ComputeSpritesForNextLine(int nextLine)
{
	int x,y;
	U16 baseSpriteAddress = ((VDP_Registers[5]&0x3E)<<8)|0x100;
	int displaySizeX = 32;
	U16 spriteAddress;
	int curSNum=0;
	int totSpritesScan=8;

	memset(spriteTempBuffer,0,displaySizeX*8);			/* clear processing buffer down */

	spriteAddress=baseSpriteAddress&0x3FFF;
	
	while (1)
	{
		U16 yPos = vRam[spriteAddress+curSNum];
		U16 xPos = vRam[spriteAddress+0x80+curSNum*2];
		U16 tile = vRam[spriteAddress+0x80+curSNum*2+1]<<5;
		U16 vSize= 0;
		U16 hSize= 0;
		U16 tileAddress = (VDP_Registers[0x06]&0x04)<<11;

		if (VDP_Registers[0x01]&0x02)
		{
			tile&=~0x20;		/* sprite tiles for 16 high are always even first */
			vSize+=1;
		}
		if (yPos==0xD0)
			break;
		if (yPos>0xD0)
			yPos-=256;

		yPos+=128;
		xPos+=128;
		if ((nextLine>=yPos) && (nextLine<=(yPos+((8*(1+vSize))-1))))			/* not interested if not current line */
		{
			/* Need to draw correct number of sprites for one line etc. */

			totSpritesScan--;
			if (!totSpritesScan)
				break;

			for (x=0;x<=hSize;x++)
			{
				for (y=0;y<=vSize;y++)
				{
					SMS_ProcessSpriteTile(xPos+x*8,yPos+y*8,tileAddress+tile+y*32,nextLine);
				}
			}
		}

		curSNum++;
		if (curSNum>63)
			break;
	}
}

void SMS_DrawSpritesForLine(int curLine,U8 zValue)
{
	int x;
	int displaySizeX = 32;

	for (x=0;x<displaySizeX*8;x++)
	{
		U8 spriteInfo = spriteTempBuffer[x];

		if (spriteInfo!=0)
		{
			int colour = spriteInfo&0x0F;
			U8 r=colour;
			U8 gb=colour|(colour<<4);

			r=(cRam[1*16+colour]&0x03)<<2;
			gb=(cRam[1*16+colour]&0x0C)<<4;
			gb|=(cRam[1*16+colour]&0x30)>>2;

			doPixelZ(128+x,curLine,r,gb,zValue);
		}
	}
}



void VID_DrawScreen(int y)
{
/*
	if (y==0)		is this correct.. should the y scroll latch?
*/
	{
		latchedScrollAmountY=VDP_Registers[0x09];
	}

	if (y<192)
	{
		U32 *pixel = pixelPosition(128,128+y);
		U8 *zPos = zBuffer;

		SMS_DisplayFillBGColourLine(pixel,320);

		memset(zBuffer,0,40*8);

		SMS_DrawScreenRow(y,0x02,pixel,zPos);

		SMS_DrawSpritesForLine(128+y,0x04);
		SMS_ComputeSpritesForNextLine(128+y+1);

	}
}
#endif
