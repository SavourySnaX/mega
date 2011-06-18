/*
 * SH2_IORegisters.h

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

#ifndef __SH2_IOREGISTERS_H
#define __SH2_IOREGISTERS_H

#include "mytypes.h"
#include "sh2.h"

/* IO Register Name Helpers */

/* Serial Communication Interface */
#define		SH2_IO_SMR					0x000								/* FFFFFE00 */
#define		SH2_IO_BRR					0x001
#define		SH2_IO_SCR					0x002
#define		SH2_IO_TDR					0x003
#define		SH2_IO_SSR					0x004
#define		SH2_IO_RDR					0x005

/* Free Running Timer */
#define		SH2_IO_TIER					0x010
#define		SH2_IO_FTCSR				0x011
#define		SH2_IO_FRCH					0x012
#define		SH2_IO_FRCL					0x013
#define		SH2_IO_OCRABH				0x014
#define		SH2_IO_OCRABL				0x015
#define		SH2_IO_TCR					0x016
#define		SH2_IO_TOCR					0x017
#define		SH2_IO_FICRH				0x018
#define		SH2_IO_FICRL				0x019

/* Interrupt Controller */
#define		SH2_IO_IPRAH				0x0E2
#define		SH2_IO_IPRAL				0x0E3
#define		SH2_IO_IPRBH				0x060
#define		SH2_IO_IPRBL				0x061
#define		SH2_IO_VCRAH				0x062
#define		SH2_IO_VCRAL				0x063
#define		SH2_IO_VCRBH				0x064
#define		SH2_IO_VCRBL				0x065
#define		SH2_IO_VCRCH				0x066
#define		SH2_IO_VCRCL				0x067
#define		SH2_IO_VCRDH				0x068
#define		SH2_IO_VCRDL				0x069
#define		SH2_IO_VCRWDTH			0x0E4
#define		SH2_IO_VCRWDTL			0x0E5
#define		SH2_IO_VCRDIVHH			0x10C
#define		SH2_IO_VCRDIVHL			0x10D
#define		SH2_IO_VCRDIVLH			0x10E
#define		SH2_IO_VCRDIVLL			0x10F
#define		SH2_IO_VCRDMA0HH		0x1A0
#define		SH2_IO_VCRDMA0HL		0x1A1
#define		SH2_IO_VCRDMA0LH		0x1A2
#define		SH2_IO_VCRDMA0LL		0x1A3
#define		SH2_IO_VCRDMA1HH		0x1A8
#define		SH2_IO_VCRDMA1HL		0x1A9
#define		SH2_IO_VCRDMA1LH		0x1AA
#define		SH2_IO_VCRDMA1LL		0x1AB
#define		SH2_IO_ICRH					0x0E0
#define		SH2_IO_ICRL					0x0E1

/* Watchdog Timer */
#define		SH2_IO_WTCSR				0x080
#define		SH2_IO_WTCNT				0x081
#define		SH2_IO_RSTCSRW			0x082
#define		SH2_IO_RSTCSRR			0x083

/* Division Unit */
#define		SH2_IO_DVSRHH				0x100
#define		SH2_IO_DVSRHL				0x101
#define		SH2_IO_DVSRLH				0x102
#define		SH2_IO_DVSRLL				0x103
#define		SH2_IO_DVDNTHH			0x104
#define		SH2_IO_DVDNTHL			0x105
#define		SH2_IO_DVDNTLH			0x106
#define		SH2_IO_DVDNTLL			0x107
#define		SH2_IO_DVCRHH				0x108
#define		SH2_IO_DVCRHL				0x109
#define		SH2_IO_DVCRLH				0x10A
#define		SH2_IO_DVCRLL				0x10B
#define		SH2_IO_DVDNTHHH			0x110				/* This set are mirrored at 118-11C (undocumented in my manual.. but sega sample relies on it */
#define		SH2_IO_DVDNTHHL			0x111
#define		SH2_IO_DVDNTHLH			0x112
#define		SH2_IO_DVDNTHLL			0x113
#define		SH2_IO_DVDNTLHH			0x114
#define		SH2_IO_DVDNTLHL			0x115
#define		SH2_IO_DVDNTLLH			0x116
#define		SH2_IO_DVDNTLLL			0x117

/* User Break Controller */
#define		SH2_IO_BARAHH				0x140
#define		SH2_IO_BARAHL				0x141
#define		SH2_IO_BARALH				0x142
#define		SH2_IO_BARALL				0x143
#define		SH2_IO_BAMRAHH			0x144
#define		SH2_IO_BAMRAHL			0x145
#define		SH2_IO_BAMRALH			0x146
#define		SH2_IO_BAMRALL			0x147
#define		SH2_IO_BBRAH				0x148
#define		SH2_IO_BBRAL				0x149
#define		SH2_IO_BARBHH				0x160
#define		SH2_IO_BARBHL				0x161
#define		SH2_IO_BARBLH				0x162
#define		SH2_IO_BARBLL				0x163
#define		SH2_IO_BAMRBHH			0x164
#define		SH2_IO_BAMRBHL			0x165
#define		SH2_IO_BAMRBLH			0x166
#define		SH2_IO_BAMRBLL			0x167
#define		SH2_IO_BBRBH				0x168
#define		SH2_IO_BBRBL				0x169
#define		SH2_IO_BDRBHH				0x170
#define		SH2_IO_BDRBHL				0x171
#define		SH2_IO_BDRBLH				0x172
#define		SH2_IO_BDRBLL				0x173
#define		SH2_IO_BDMRBHH			0x174
#define		SH2_IO_BDMRBHL			0x175
#define		SH2_IO_BDMRBLH			0x176
#define		SH2_IO_BDMRBLL			0x177
#define		SH2_IO_BRCRH				0x178
#define		SH2_IO_BRCRL				0x179

/* DMA Controller */
#define		SH2_IO_SAR0HH				0x180
#define		SH2_IO_SAR0HL				0x181
#define		SH2_IO_SAR0LH				0x182
#define		SH2_IO_SAR0LL				0x183
#define		SH2_IO_DAR0HH				0x184
#define		SH2_IO_DAR0HL				0x185
#define		SH2_IO_DAR0LH				0x186
#define		SH2_IO_DAR0LL				0x187
#define		SH2_IO_TCR0HH				0x188
#define		SH2_IO_TCR0HL				0x189
#define		SH2_IO_TCR0LH				0x18A
#define		SH2_IO_TCR0LL				0x18B
#define		SH2_IO_CHCR0HH			0x18C
#define		SH2_IO_CHCR0HL			0x18D
#define		SH2_IO_CHCR0LH			0x18E
#define		SH2_IO_CHCR0LL			0x18F
#define		SH2_IO_SAR1HH				0x190
#define		SH2_IO_SAR1HL				0x191
#define		SH2_IO_SAR1LH				0x192
#define		SH2_IO_SAR1LL				0x193
#define		SH2_IO_DAR1HH				0x194
#define		SH2_IO_DAR1HL				0x195
#define		SH2_IO_DAR1LH				0x196
#define		SH2_IO_DAR1LL				0x197
#define		SH2_IO_TCR1HH				0x198
#define		SH2_IO_TCR1HL				0x199
#define		SH2_IO_TCR1LH				0x19A
#define		SH2_IO_TCR1LL				0x19B
#define		SH2_IO_CHCR1HH			0x19C
#define		SH2_IO_CHCR1HL			0x19D
#define		SH2_IO_CHCR1LH			0x19E
#define		SH2_IO_CHCR1LL			0x19F
#define		SH2_IO_DRCR0				0x071
#define		SH2_IO_DRCR1				0x072
#define		SH2_IO_DMAORHH			0x1B0
#define		SH2_IO_DMAORHL			0x1B1
#define		SH2_IO_DMAORLH			0x1B2
#define		SH2_IO_DMAORLL			0x1B3

#define		SH2_IO_CHCRLL_DE			(1<<0)
#define		SH2_IO_CHCRLL_TE			(1<<1)
#define		SH2_IO_CHCRLL_TA			(1<<3)

#define		SH2_IO_CHCRLH_AR			(1<<1)
#define		SH2_IO_CHCRLH_TS0			(1<<2)
#define		SH2_IO_CHCRLH_TS1			(1<<3)
#define		SH2_IO_CHCRLH_SM0			(1<<4)
#define		SH2_IO_CHCRLH_SM1			(1<<5)
#define		SH2_IO_CHCRLH_DM0			(1<<6)
#define		SH2_IO_CHCRLH_DM1			(1<<7)

/* Bus State Controller */
#define		SH2_IO_BCR1H				0x1E2
#define		SH2_IO_BCR1L				0x1E3
#define		SH2_IO_BCR2H				0x1E6
#define		SH2_IO_BCR2L				0x1E7
#define		SH2_IO_WCRH					0x1EA
#define		SH2_IO_WCRL					0x1EB
#define		SH2_IO_MCRH					0x1EE
#define		SH2_IO_MCRL					0x1EF
#define		SH2_IO_RTCSRH				0x1F2
#define		SH2_IO_RTCSRL				0x1F3
#define		SH2_IO_RTCNTH				0x1F6
#define		SH2_IO_RTCNTL				0x1F7
#define		SH2_IO_RTCORH				0x1FA
#define		SH2_IO_RTCORL				0x1FB

/* Cache */
#define		SH2_IO_CCR					0x092

#define		SH2_IO_CCR_W1				(1<<7)
#define		SH2_IO_CCR_W0				(1<<6)
#define		SH2_IO_CCR_CP				(1<<4)
#define		SH2_IO_CCR_TW				(1<<3)
#define		SH2_IO_CCR_OD				(1<<2)
#define		SH2_IO_CCR_ID				(1<<1)
#define		SH2_IO_CCR_CE				(1<<0)

/* Power Down */
#define		SH2_IO_SBYCR				0x091

void SH2_IO_Write_Byte(SH2_State* cpu,U32 reg,U8 data);
U8 SH2_IO_Read_Byte(SH2_State* cpu,U32 reg);

void SH2_IO_Reset(SH2_State* cpu);

#endif/*__SH2_IOREGISTERS_H*/

