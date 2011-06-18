/* 

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
/*

 For reference : Video Timings

	TYPE										PAL														NTSC
	
	FPS											25														30 B&W  - 29.97 COLOUR
  SCANLINES								625														525
	SCANS PER FIELD					312.5													262.5
	
	ONE SCAN LINE						64micro seconds								63.55micro seconds
	FRONT PORCH							1.65micro seconds							1.5micro seconds
	HSYNC PULSE							4.7micro seconds							4.7micro seconds
	BACK PORCH							5.7micro seconds							4.5micro seconds
	TOTAL BLANKING LEN			12.05micro seconds						10.7micro seconds
	ACTIVE DISPLAY					51.95micro seconds						52.85micro seconds


	SCANLINE : HSYNCPULSE  BACKPORCH  ACTIVEDISPLAY FRONTPORCH

	PAL DISPLAY BREAKDOWN :

			INTERLACED																				FAKE PROGRESSIVE

			0 -> 22.5			(FIELD 1 START + BLANKING PERIOD)		0 -> 22.5			(BLANKING PERIOD)
			22.5 -> 309		(DISPLAY PERIOD)										22.5 -> 308		(DISPLAY PERIOD)
			310 -> 312.5	(BLANKING PERIOD)										309 -> 311		(BLANKING PERIOD)
			312.5 -> 334	(FIELD 2 START + BLANKING PERIOD)
			335 -> 622.5	(DISPLAY PERIOD)
			622.5 -> 624	(BLANKING PERIOD)

*/





#include "config.h"

#include "platform.h"

#include "cpu.h"
#include "z80.h"
#include "memory.h"
#if ENABLE_32X_MODE
#include "sh2.h"
#endif

#include "gui/debugger.h"

U8	inHBlank=0;
U8	inVBlank=0;

#if ENABLE_32X_MODE
/*------------------------------------*/

SH2_State* master;
SH2_State* slave;

U8* cpu68kbios=NULL;
U8* masterbios=NULL;
U8* slavebios=NULL;

U32 cpu68kbiosSize;
U32 masterbiosSize;
U32 slavebiosSize;

U8 CRAM[0x200];
U8 FRAMEBUFFER[0x00040000];
U8 SDRAM[0x00040000];
U8 SH2_68K_COMM_REGISTER[16]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

extern U16 AdapterControlRegister;
U8 SH2_Master_AdapterControlRegister=0x00;
U8 SH2_Slave_AdapterControlRegister=0x00;

U16 SH2_HCountRegister=0x0000;

U16 SH2_VRESInterruptClearRegister=0x0000;
U16 SH2_VInterruptClearRegister=0x0000;
U16 SH2_HInterruptClearRegister=0x0000;
U16 SH2_CMDInterruptClearRegister=0x0000;
U16 SH2_PWMInterruptClearRegister=0x0000;
U16 SH2_StandByChangeRegister=0x0000;
U16 SH2_PWMControlRegister=0x0000;
U16 SH2_PWMCycleRegister=0x0000;
U16 SH2_PWMLchPulseWidthRegister=0x0000;
U16 SH2_PWMRchPulseWidthRegister=0x0000;
U16 SH2_PWMMonoPulseWidthRegister=0x0000;

U16 FrameBufferSwitchWrite=0x0000;
U16 FrameBufferSwitchRead=0x0000;

#if PAL_PRETEND
U16 BitmapModeRegister=0x0000;							/* PAL mode */
#else
U16 BitmapModeRegister=0x8000;							/* NTSC mode */
#endif
U16 ScreenShiftRegister=0x0000;
U16 AutoFillLengthRegister=0x0000;
U16 AutoFillStartAddressRegister=0x0000;
U16 AutoFillDataRegister=0x0000;

U32 ActiveFrameBuffer=0;

extern U16 DREQControlRegister;
extern U16 DREQSourceAddressHi;
extern U16 DREQSourceAddressLo;
extern U16 DREQDestinationAddressHi;
extern U16 DREQDestinationAddressLo;
extern U16 DREQLengthRegister;
extern U16 FIFORegister;
U16 FIFO_GET();

void VDP_32X_ScreenFlipHack()
{
	FrameBufferSwitchRead=FrameBufferSwitchWrite;
	ActiveFrameBuffer=FrameBufferSwitchWrite&0x01;

}

U8 SYS_32X_Read(U16 adr,int accessor)
{
	if (adr>=0x20 && adr<=0x2F)
	{
		return SH2_68K_COMM_REGISTER[adr-0x20]; 
	}

	switch (adr)
	{
	case 0x00:
		return ((AdapterControlRegister & 0x8000)|((AdapterControlRegister&0x1)<<9))>>8;
	case 0x01:
		if (accessor==DEB_Mode_SH2_Master)
			return SH2_Master_AdapterControlRegister;
		return SH2_Slave_AdapterControlRegister;
	case 0x02:
		return 0xFF;		/* write only */
	case 0x03:
		return 0xFF;
	case 0x04:
		return SH2_HCountRegister>>8;
	case 0x05:
		return SH2_HCountRegister&0xFF;
	case 0x06:
		return DREQControlRegister>>8;
	case 0x07:
		return DREQControlRegister&0xFF;
	case 0x08:
		return DREQSourceAddressHi>>8;
	case 0x09:
		return DREQSourceAddressHi&0xFF;
	case 0x0A:
		return DREQSourceAddressLo>>8;
	case 0x0B:
		return DREQSourceAddressLo&0xFF;
	case 0x0C:
		return DREQDestinationAddressHi>>8;
	case 0x0D:
		return DREQDestinationAddressHi&0xFF;
	case 0x0E:
		return DREQDestinationAddressLo>>8;
	case 0x0F:
		return DREQDestinationAddressLo&0xFF;
	case 0x10:
		return DREQLengthRegister>>8;
	case 0x11:
		return DREQLengthRegister&0xFF;
	case 0x12:
		FIFORegister=FIFO_GET();
		return FIFORegister>>8;
	case 0x13:
		return FIFORegister&0xFF;
	case 0x14:
		return 0xFF;		/* write only */
	case 0x15:
		return 0xFF;
	case 0x16:
		return 0xFF;
	case 0x17:
		return 0xFF;
	case 0x18:
		return 0xFF;
	case 0x19:
		return 0xFF;
	case 0x1A:
		return 0xFF;
	case 0x1B:
		return 0xFF;
	case 0x1C:
		return 0xFF;
	case 0x1D:
		return 0xFF;
	case 0x30:
		return SH2_PWMControlRegister>>8;
	case 0x31:
		return SH2_PWMControlRegister&0xFF;
	case 0x32:
		return SH2_PWMCycleRegister>>8;
	case 0x33:
		return SH2_PWMCycleRegister&0xFF;
	case 0x34:
		return (SH2_PWMLchPulseWidthRegister>>8)&0xC0;
	case 0x35:
		return (SH2_PWMLchPulseWidthRegister&0xFF)&0x00;
	case 0x36:
		return (SH2_PWMRchPulseWidthRegister>>8)&0xC0;
	case 0x37:
		return (SH2_PWMRchPulseWidthRegister&0xFF)&0x00;
	case 0x38:
		return (SH2_PWMMonoPulseWidthRegister>>8)&0xC0;
	case 0x39:
		return (SH2_PWMMonoPulseWidthRegister&0xFF)&0x00;
	}
	printf("%08X",adr);
	DEB_PauseEmulation(accessor,"32X SYS Unsupported Read");
	return 0xFF;
}

void SYS_32X_Write(U16 adr,U8 byte,int accessor)
{
	if (adr>=0x20 && adr<=0x2F)
	{
		SH2_68K_COMM_REGISTER[adr-0x20]=byte;
		return;
	}

	switch (adr)
	{
	case 0x00:
		AdapterControlRegister&=0x7FFF;
		AdapterControlRegister|=(byte&0x80)<<8;
		return;
	case 0x01:
		if (accessor==DEB_Mode_SH2_Master)
		{
			SH2_Master_AdapterControlRegister=byte;
			SH2_Slave_AdapterControlRegister&=0x7F;					/* need to ensure HEN copied to both internal registers */
			SH2_Slave_AdapterControlRegister|=byte&0x80;
		}
		else
		{
			SH2_Slave_AdapterControlRegister=byte;
			SH2_Master_AdapterControlRegister&=0x7F;
			SH2_Master_AdapterControlRegister|=byte&0x80;
		}
		return;
	case 0x02:
		SH2_StandByChangeRegister&=0x00FF;
		SH2_StandByChangeRegister|=byte<<8;
		return;
	case 0x03:
		SH2_StandByChangeRegister&=0xFF00;
		SH2_StandByChangeRegister|=byte;
		return;
	case 0x04:
		SH2_HCountRegister&=0x00FF;
		SH2_HCountRegister|=byte<<8;
		return;
	case 0x05:
		SH2_HCountRegister&=0xFF00;
		SH2_HCountRegister|=byte;
		return;
	case 0x06:
		return;			/* Read Only		(megadrive side is reverse) */
	case 0x07:
		return;			/* Read Only */
	case 0x08:
		return;			/* Read Only */
	case 0x09:
		return;			/* Read Only */
	case 0x0A:
		return;			/* Read Only */
	case 0x0B:
		return;			/* Read Only */
	case 0x0C:
		return;			/* Read Only */
	case 0x0D:
		return;			/* Read Only */
	case 0x0E:
		return;			/* Read Only */
	case 0x0F:
		return;			/* Read Only */
	case 0x10:
		return;			/* Read Only */
	case 0x11:
		return;			/* Read Only */
	case 0x12:
		return;			/* Read Only */
	case 0x13:
		return;			/* Read Only */
	case 0x14:
		SH2_VRESInterruptClearRegister&=0x00FF;
		SH2_VRESInterruptClearRegister|=byte<<8;
		return;
	case 0x15:
		SH2_VRESInterruptClearRegister&=0xFF00;
		SH2_VRESInterruptClearRegister|=byte;
		return;
	case 0x16:
		SH2_VInterruptClearRegister&=0x00FF;
		SH2_VInterruptClearRegister|=byte<<8;
		return;
	case 0x17:
		SH2_VInterruptClearRegister&=0xFF00;
		SH2_VInterruptClearRegister|=byte;
		return;
	case 0x18:
		SH2_HInterruptClearRegister&=0x00FF;
		SH2_HInterruptClearRegister|=byte<<8;
		return;
	case 0x19:
		SH2_HInterruptClearRegister&=0xFF00;
		SH2_HInterruptClearRegister|=byte;
		return;
	case 0x1A:
		SH2_CMDInterruptClearRegister&=0x00FF;
		SH2_CMDInterruptClearRegister|=byte<<8;
		return;
	case 0x1B:
		SH2_CMDInterruptClearRegister&=0xFF00;
		SH2_CMDInterruptClearRegister|=byte;
		return;
	case 0x1C:
		SH2_PWMInterruptClearRegister&=0x00FF;
		SH2_PWMInterruptClearRegister|=byte<<8;
		return;
	case 0x1D:
		SH2_PWMInterruptClearRegister&=0xFF00;
		SH2_PWMInterruptClearRegister|=byte;
		return;
	case 0x30:
		SH2_PWMControlRegister&=0x00FF;
		SH2_PWMControlRegister|=byte<<8;
		return;
	case 0x31:
		SH2_PWMControlRegister&=0xFF00;
		SH2_PWMControlRegister|=byte;
		return;
	case 0x32:
		SH2_PWMCycleRegister&=0x00FF;
		SH2_PWMCycleRegister|=byte<<8;
		return;
	case 0x33:
		SH2_PWMCycleRegister&=0xFF00;
		SH2_PWMCycleRegister|=byte;
		return;
	case 0x34:
		SH2_PWMLchPulseWidthRegister&=0xC0FF;
		SH2_PWMLchPulseWidthRegister|=(byte&0x3F)<<8;
		return;
	case 0x35:
		SH2_PWMLchPulseWidthRegister&=0xFF00;
		SH2_PWMLchPulseWidthRegister|=byte;
		return;
	case 0x36:
		SH2_PWMRchPulseWidthRegister&=0xC0FF;
		SH2_PWMRchPulseWidthRegister|=(byte&0x3F)<<8;
		return;
	case 0x37:
		SH2_PWMRchPulseWidthRegister&=0xFF00;
		SH2_PWMRchPulseWidthRegister|=byte;
		return;
	case 0x38:
		SH2_PWMMonoPulseWidthRegister&=0xC0FF;
		SH2_PWMMonoPulseWidthRegister|=(byte&0x3F)<<8;
		return;
	case 0x39:
		SH2_PWMMonoPulseWidthRegister&=0xFF00;
		SH2_PWMMonoPulseWidthRegister|=byte;
		return;
	}
	printf("%08X",adr);
	DEB_PauseEmulation(accessor,"32X SYS Unsupported Write");
}

U8 VDP_32X_Read(U16 adr,int accessor)
{
	switch (adr)
	{
	case 0x00:
		return BitmapModeRegister>>8;
	case 0x01:
		return BitmapModeRegister&0xFF;
	case 0x02:
		return ScreenShiftRegister>>8;
	case 0x03:
		return ScreenShiftRegister&0xFF;
	case 0x04:
		return AutoFillLengthRegister>>8;
	case 0x05:
		return AutoFillLengthRegister&0xFF;
	case 0x06:
		return AutoFillStartAddressRegister>>8;
	case 0x07:
		return AutoFillStartAddressRegister&0xFF;
	case 0x08:
		return AutoFillDataRegister>>8;
	case 0x09:
		return AutoFillDataRegister&0xFF;
	case 0x0A:
		return (FrameBufferSwitchRead>>8)|(inVBlank<<7)|(inHBlank<<6)|((inVBlank|inHBlank)<<5);
	case 0x0B:
		return FrameBufferSwitchRead&0xFF;
	}
	printf("%08X",adr);
	DEB_PauseEmulation(accessor,"32X VDP Unsupported Read");
	return 0xFF;
}

U8 vdpFillGo=0;

void VDP_32X_Tick()			/* HACK. does not work on a tick for some reason */
{
	U16 tmp;
	AutoFillLengthRegister++;
	while (vdpFillGo)
	{
		FrameBufferSwitchRead|=2;

		if (ActiveFrameBuffer)
		{
			FRAMEBUFFER[AutoFillStartAddressRegister*2+0+0x20000]=AutoFillDataRegister>>8;
			FRAMEBUFFER[AutoFillStartAddressRegister*2+1+0x20000]=AutoFillDataRegister&0xFF;
		}
		else
		{
			FRAMEBUFFER[AutoFillStartAddressRegister*2+0]=AutoFillDataRegister>>8;
			FRAMEBUFFER[AutoFillStartAddressRegister*2+1]=AutoFillDataRegister&0xFF;
		}

		tmp=AutoFillStartAddressRegister&0xFF00;
		AutoFillStartAddressRegister+=1;
		tmp|=AutoFillStartAddressRegister&0x00FF;
		AutoFillLengthRegister--;
		if (AutoFillLengthRegister==0)
		{
			vdpFillGo=0;
			FrameBufferSwitchRead&=~2;
		}
	}
}

void VDP_32X_Write(U16 adr,U8 byte,int accessor)
{
	switch (adr)
	{
	case 0x00:
		BitmapModeRegister&=0x80FF;
		BitmapModeRegister|=(byte&0x7F)<<8;			/* Can't affect PAL bit */
		return;
	case 0x01:
		BitmapModeRegister&=0xFF00;
		BitmapModeRegister|=byte;
		return;
	case 0x02:
		ScreenShiftRegister&=0x00FF;
		return;
	case 0x03:
		ScreenShiftRegister&=0xFF00;
		ScreenShiftRegister|=byte&0x01;
	case 0x04:
		AutoFillLengthRegister&=0x00FF;
		AutoFillLengthRegister|=byte<<8;
		return;
	case 0x05:
		AutoFillLengthRegister&=0xFF00;
		AutoFillLengthRegister|=byte;
		return;
	case 0x06:
		AutoFillStartAddressRegister&=0x00FF;
		AutoFillStartAddressRegister|=byte<<8;
		return;
	case 0x07:
		AutoFillStartAddressRegister&=0xFF00;
		AutoFillStartAddressRegister|=byte;
		return;
	case 0x08:
		AutoFillDataRegister&=0x00FF;
		AutoFillDataRegister|=byte<<8;
		return;
	case 0x09:
		AutoFillDataRegister&=0xFF00;
		AutoFillDataRegister|=byte;
		vdpFillGo=1;
		return;
	case 0x0A:
		FrameBufferSwitchWrite&=0xE0FF;
		FrameBufferSwitchWrite|=(byte&0x1F)<<8;
		return;
	case 0x0B:
		FrameBufferSwitchWrite&=0xFF02;
		FrameBufferSwitchWrite|=(byte&0xFD);
		return;
	}

	printf("%08X",adr);
	DEB_PauseEmulation(accessor,"32X VDP Unsupported Write");
}

void MEM_Set_Byte32X(U32 adr,U8 byte,int accessor)
{
	if (adr>=0x4000 && adr<0x403A)
	{
		SYS_32X_Write(adr-0x4000,byte,accessor);
		return;
	}

	if (adr>=0x4100 && adr<0x4200)
	{
		if ((AdapterControlRegister&0x8000)==0)
			return;
		VDP_32X_Write(adr-0x4100,byte,accessor);
		return;
	}

	if (adr>=0x4200 && adr<0x4400)
	{
		if ((AdapterControlRegister&0x8000)==0)
			return;
		CRAM[adr-0x4200]=byte;
		return;
	}

	if (adr>=0x04000000 && adr<0x04020000)				/* Normal write mode */
	{
		if ((AdapterControlRegister&0x8000)==0)
			return;
		if (ActiveFrameBuffer)
		{
			FRAMEBUFFER[(adr-0x04000000) + 0x20000]=byte;
		}
		else
		{
			FRAMEBUFFER[adr-0x04000000]=byte;
		}
		return;
	}

	if (adr>=0x04020000 && adr<0x04040000)			/* Frame Overwrite mode */
	{
		if ((AdapterControlRegister&0x8000)==0)
			return;
		if (ActiveFrameBuffer)
		{
			if (byte!=0)
			{
				FRAMEBUFFER[(adr-0x04020000) + 0x20000]=byte;
			}
		}
		else
		{
			if (byte!=0)
			{
				FRAMEBUFFER[adr-0x04020000]=byte;
			}
		}
		return;
	}

	if (adr>=0x06000000 && adr<0x06040000)
	{
		SDRAM[adr-0x06000000]=byte;
		return;
	}

	printf("Write To Unmapped Address : %08X <- %02X\n",adr,byte);
	DEB_PauseEmulation(accessor,"Unmapped SH2 write");
}

void SH2_Master_Write(U32 adr,U8 byte)
{
	MEM_Set_Byte32X(adr,byte,DEB_Mode_SH2_Master);
}

void SH2_Slave_Write(U32 adr,U8 byte)
{
	MEM_Set_Byte32X(adr,byte,DEB_Mode_SH2_Slave);
}
extern unsigned char *cartRom;

U8 MEM_Get_Byte32X(U32 adr,int accessor)
{
	if (adr>=0x4000 && adr<0x403A)
	{
		return SYS_32X_Read(adr-0x4000,accessor);
	}

	if (adr>=0x4100 && adr<0x4200)
	{
		if ((AdapterControlRegister&0x8000)==0)
			return 0xFF;
		return VDP_32X_Read(adr-0x4100,accessor);
	}

	if (adr>=0x4200 && adr<0x4400)
	{
		if ((AdapterControlRegister&0x8000)==0)
			return 0xFF;
		return CRAM[adr-0x4200];
	}

	if (adr>=0x02000000 && adr<0x02400000)
		return cartRom[adr-0x02000000];

	if (adr>=0x04000000 && adr<0x04020000)
	{
		if ((AdapterControlRegister&0x8000)==0)
			return 0xFF;
		if (ActiveFrameBuffer)
		{
			return FRAMEBUFFER[(adr-0x04000000)+0x20000];
		}
		else
		{
			return FRAMEBUFFER[adr-0x04000000];
		}
	}
	if (adr>=0x04020000 && adr<0x04040000)			/* assume reading from frame overwrite buffer just returns current working frame */
	{
		if ((AdapterControlRegister&0x8000)==0)
			return 0xFF;
		if (ActiveFrameBuffer)
		{
			return FRAMEBUFFER[(adr-0x04020000)+0x20000];
		}
		else
		{
			return FRAMEBUFFER[adr-0x04020000];
		}
	}
	if (adr>=0x06000000 && adr<0x06040000)
		return SDRAM[adr-0x06000000];


	printf("Read From Unmapped Address : %08X",adr);

	DEB_PauseEmulation(accessor,"Unmapped read from address");
	return 0xFF;
}

U8 SH2_Master_Read(U32 adr)
{
	if (adr<masterbiosSize)
		return masterbios[adr];

	return MEM_Get_Byte32X(adr,DEB_Mode_SH2_Master);
}

U8 SH2_Slave_Read(U32 adr)
{
	if (adr<slavebiosSize)
		return slavebios[adr];

	return MEM_Get_Byte32X(adr,DEB_Mode_SH2_Slave);
}

/* END OF STUFF TO MOVE */
#endif

unsigned char *SRAM=NULL;
U32 SRAM_Size=0;
U32 SRAM_StartAddress=0;
U32 SRAM_EndAddress=0;
U32 SRAM_OddAccess=0;
U32 SRAM_EvenAccess=0;

void ParseRomHeader(unsigned char *header)
{
	U32 romStart,romEnd;
	char tmpBuffer[256];

	memcpy(tmpBuffer,header,16);
	tmpBuffer[16]=0;
	printf("Console : %s\n",tmpBuffer);
	header+=16;
	memcpy(tmpBuffer,header,16);
	tmpBuffer[16]=0;
	printf("Copyright : %s\n",tmpBuffer);
	header+=16;
	memcpy(tmpBuffer,header,48);
	tmpBuffer[48]=0;
	printf("Domestic Name : %s\n",tmpBuffer);
	header+=48;
	memcpy(tmpBuffer,header,48);
	tmpBuffer[48]=0;
	printf("International Name : %s\n",tmpBuffer);
	header+=48;
	memcpy(tmpBuffer,header,2);
	tmpBuffer[2]=0;
	printf("Game Type : %s\n",tmpBuffer);
	header+=2;
	memcpy(tmpBuffer,header,12);
	tmpBuffer[12]=0;
	printf("ProductCode-Version : %s\n",tmpBuffer);
	header+=12;

	/*skip checksum */
	header+=2;
	memcpy(tmpBuffer,header,16);
	tmpBuffer[16]=0;
	printf("IO Support : %s\n",tmpBuffer);
	header+=16;

	printf("Rom Start : %02X%02X%02X%02X\n",header[0],header[1],header[2],header[3]);
	romStart=(header[0]<<24)|(header[1]<<16)|(header[2]<<8)|header[3];
	header+=4;
	printf("Rom End : %02X%02X%02X%02X\n",header[0],header[1],header[2],header[3]);
	romEnd=(header[0]<<24)|(header[1]<<16)|(header[2]<<8)|header[3];
	header+=4;
	printf("Ram Start : %02X%02X%02X%02X\n",header[0],header[1],header[2],header[3]);
	header+=4;
	printf("Ram End : %02X%02X%02X%02X\n",header[0],header[1],header[2],header[3]);
	header+=4;
	if (header[0]=='R' && header[1]=='A')
	{
		U32 start,end;

		printf("Backup Ram Available %02X%02X: %s %s",header[2],header[3],header[2]&0x40 ? "Backup" : "ERam",(((header[2]&0xA7)==0xA0)&&(header[3]==0x20))?"normal":"unknown");
		printf(" - Accessed at ");
		switch (header[2]&0x18)
		{
		case 0:
			printf("Odd & Even\n");
			SRAM_OddAccess=1;
			SRAM_EvenAccess=1;
			break;
		case 0x10:
			printf("Even Only\n");
			SRAM_EvenAccess=1;
			break;
		case 0x18:
			printf("Odd Only\n");
			SRAM_OddAccess=1;
			break;
		default:
			printf("unknown\n");
			printf("SERIAL EEPROM - Not supported.. disabling SRAM\n");
			return;
			break;
		}
	
		header+=4;
		start=(header[0]<<24)|(header[1]<<16)|(header[2]<<8)|header[3];
		printf("Backup Ram Start : %08X\n",start);
		header+=4;
		end=(header[0]<<24)|(header[1]<<16)|(header[2]<<8)|header[3];
		printf("Backup Ram End : %08X\n",end);
   
		if (romEnd>start)
		{
			printf("SRAM conflicts with ROM - disabling SRAM\n");
			return;
		}
 
		SRAM_StartAddress=start;
		SRAM_EndAddress=end;
		SRAM_Size=end-start;
		if (SRAM_EvenAccess^SRAM_OddAccess)
		{
			SRAM_Size/=2;
		}
		SRAM_Size+=1;

		SRAM = (unsigned char *)malloc(SRAM_Size);
		memset(SRAM,0,SRAM_Size);
	}
	else
	{
		header+=8;
	}
	header+=4;

	memcpy(tmpBuffer,header,12);
	tmpBuffer[12]=0;
	printf("MODEM Support : %s\n",tmpBuffer);
	header+=12;
	memcpy(tmpBuffer,header,40);
	tmpBuffer[40]=0;
	printf("Notes : %s\n",tmpBuffer);
	header+=40;
	memcpy(tmpBuffer,header,16);
	tmpBuffer[16]=0;
	printf("Country Codes : %s\n",tmpBuffer);
	header+=16;
}

unsigned long romSize;
unsigned char *load_rom(char *romName,unsigned int *numBanks)
{
    FILE *inRom;
    unsigned char *romData;
	
    inRom = fopen(romName,"rb");
    if (!inRom)
    {
			return 0;
    }
    fseek(inRom,0,SEEK_END);
    romSize = ftell(inRom);

		if ((romSize&0xFFFF) != 0)
		{
			printf("Bugger.. not 64k rom thing\n");
			*numBanks= (romSize>>16) + 1;
		}
		else
		{
			*numBanks = romSize>>16;
		}
		fseek(inRom,0,SEEK_SET);
    romData = (unsigned char *)malloc(romSize);
    if (romSize != fread(romData,1,romSize,inRom))
	{
		fclose(inRom);
		return 0;
	}
    fclose(inRom);

#if (!SMS_MODE) && (!ENABLE_32X_MODE)
  ParseRomHeader(romData+0x100);
#endif

	return romData;
}

extern unsigned char *cartRam;

void LoadSRAM(const char* filename)
{
	FILE *inSRam;

	inSRam = fopen(filename,"rb");
	if (!inSRam)
	{
		return;
	}
#if SMS_MODE
	SRAM=cartRam;
	SRAM_Size=32*1024;
#endif
	if (SRAM_Size!=fread(SRAM,1,SRAM_Size,inSRam))
	{
	}
	fclose(inSRam);
}

void SaveSRAM(const char* filename)
{
	FILE *inSRam;

	inSRam = fopen(filename,"wb");
	if (!inSRam)
	{
		return;
	}
	fwrite(SRAM,1,SRAM_Size,inSRam);
	fclose(inSRam);
}

extern int startDebug;

U8 videoMemory[LINE_LENGTH*HEIGHT*sizeof(U32)];

int g_newScreenNotify = 0;

U32* pixelPosition(int x,int y)
{
	return &((U32*)videoMemory)[y*LINE_LENGTH + x];
}

void doPixel32(int x,int y,U32 colour)
{
	U32 *pixmem32;
	
	if (y>=HEIGHT || x>=LINE_LENGTH)
		return;
	
	pixmem32 = &((U32*)videoMemory)[y*LINE_LENGTH + x];
	*pixmem32 = colour;
}


void doPixel(int x,int y,U8 colHi,U8 colLo)
{
	U32 colour;
	U8 r = (colHi&0x0F)<<4;
	U8 g = (colLo&0xF0);
	U8 b = (colLo&0x0F)<<4;
	
	colour = (r<<16) | (g<<8) | (b<<0);
	doPixel32(x,y,colour);
}

void doPixelClipped(int x,int y,U8 colHi,U8 colLo)
{
	U32 *pixmem32;
	U32 colour;
	U8 r = (colHi&0x0F)<<4;
	U8 g = (colLo&0xF0);
	U8 b = (colLo&0x0F)<<4;
	
	if (x<128 || x>128+40*8 || y<128 || y>128+224)
		return;
	
	colour = (r<<16) | (g<<8) | (b<<0);
	pixmem32 = &((U32*)videoMemory)[y*LINE_LENGTH + x];
	*pixmem32 = colour;
}

extern int g_pause;

void DrawScreen() 
{
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, 1);
	
	/* glTexSubImage2D is faster when not using a texture range */
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_NV, 0, 0, 0, LINE_LENGTH, HEIGHT, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, videoMemory);
	glBegin(GL_QUADS);

#if ENABLE_DEBUGGER
	if (g_pause)
#else
	if (0)
#endif
	{
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(-1.0f, 1.0f);

		glTexCoord2f(0.0f, HEIGHT);
		glVertex2f(-1.0f, -1.0f);

		glTexCoord2f(LINE_LENGTH, HEIGHT);
		glVertex2f(1.0f, -1.0f);

		glTexCoord2f(LINE_LENGTH, 0.0f);
		glVertex2f(1.0f, 1.0f);
	}
	else
	{
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(-1.24f, 1.75f);

		glTexCoord2f(0.0f, HEIGHT);
		glVertex2f(-1.24f, -2.25f);

		glTexCoord2f(LINE_LENGTH, HEIGHT);
		glVertex2f(2.74f, -2.25f);

		glTexCoord2f(LINE_LENGTH, 0.0f);
		glVertex2f(2.74f, 1.75f);
	}
	glEnd();
	
	glFlush();
}

void setupGL(int w, int h) 
{
    /*Tell OpenGL how to convert from coordinates to pixel values */
    glViewport(0, 0, w, h);
	
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glClearColor(1.0f, 0.f, 1.0f, 1.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	
	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity(); 
	
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_RECTANGLE_NV);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, 1);
	
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	
	glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA, LINE_LENGTH,
				 HEIGHT, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, videoMemory);
	
	glDisable(GL_DEPTH_TEST);
}

U8 keyArray[512*3];

int KeyDown(int key)
{
	return keyArray[key*3+1]==GLFW_PRESS;
}

int CheckKey(int key)
{
	return keyArray[key*3+2];
}

void ClearKey(int key)
{
	keyArray[key*3+2]=0;
}

void GLFWCALL kbHandler( int key, int action )
{
	keyArray[key*3 + 0]=keyArray[key*3+1];
	keyArray[key*3 + 1]=action;
	keyArray[key*3 + 2]|=(keyArray[key*3+0]==GLFW_RELEASE)&&(keyArray[key*3+1]==GLFW_PRESS);
}

int captureMouse=0;

#if OPENAL_SUPPORT
void AudioKill();
void AudioInitialise();
void UpdateAudio();
#endif

U32 lineNo=0;
U32 colNo=0;

extern U8 VDP_Registers[0x20];

U32 LineCounter=0xFF;
U32 _32XLineCounter=0xFF;

void VID_DrawScreen(int lineNo);
void DisplaySprites();
void DisplayApproximationOfScreen(int x,int y,int winNumber);

U16 keyStatusJoyA=0;			/* 1 means pressed (flipped in memory code) */
U8 SMS_KeyJoyA=0;

/* Some global variables used to measure the time */
float  timeAtGameStart;
/* Called every time you need the current game time */
float GetGameTime()
{
  float time;
 
  time=(float)glfwGetTime();
  time -= timeAtGameStart;
  return time;
}
/* Called once at the start of the game */
void InitGameTime()
{
  timeAtGameStart = 0;
  timeAtGameStart = GetGameTime();
}

/* Global variables for measuring fps */
float lastUpdate        = 0;
float fpsUpdateInterval = 1.0f;
U32  numFrames    = 0;
float fps               = 0;
/* Called once for every frame */
void UpdateFPS()
{
  float currentUpdate = GetGameTime();
  numFrames++;
  if( currentUpdate - lastUpdate > fpsUpdateInterval )
  {
    fps = numFrames / (currentUpdate - lastUpdate);
		printf("fps %f\n",fps);
    lastUpdate = currentUpdate;
    numFrames = 0;
  }
}

void FM_Update();

#ifdef _WIN32
#define BASE_PATH	"g:\\"
#else
#define BASE_PATH	"/media/3848-3209/"
#endif

#define ROM(a,b)		char* romName=BASE_PATH a; char* romNameExt=BASE_PATH a b; char* saveName=BASE_PATH a ".srm";

#if SMS_MODE
ROM("sprite",".sms")
#else
ROM("vdptest",".bin")
#endif

double					atStart;
int lockFrameRate = 1;

extern unsigned char *smsBios;
#if SMS_MODE
extern U8 SMS_VDP_Status;
#endif
void PSG_Update();

U32 PSG_OutOn=1;
U32 YM_OutOn=1;

char tmpRomName[1024];
char tmpRomSave[1024];

int main(int argc,char **argv)
{
	unsigned int numBlocks;
	unsigned char *romPtr;
	int running=1;
	int a;

	if (argc==2)
	{
		strcpy(tmpRomName,argv[1]);
		strcpy(tmpRomSave,argv[1]);
		strcat(tmpRomSave,".srm");
		romName=argv[1];
		romNameExt=tmpRomName;
		saveName=tmpRomSave;
	}

	InitGameTime();

	for (a=0;a<512*3;a++)
	{
		keyArray[a]=0;
	}
	
#if OPENAL_SUPPORT
    
	AudioInitialise();
	
#endif
	/* Initialize GLFW  */
	glfwInit(); 
	/* Open an OpenGL window  */
	if( !glfwOpenWindow( LINE_LENGTH, HEIGHT, 0,0,0,0,0,0, GLFW_WINDOW ) ) 
	{ 
		glfwTerminate(); 
		return 1; 
	} 
	
	glfwSetWindowTitle("Mega");
	glfwSetWindowPos(300,300);
	
	setupGL(LINE_LENGTH,HEIGHT);	

	glfwSwapInterval(0);			/* Disable VSYNC */

#if SMS_MODE
	smsBios=load_rom(BASE_PATH "bios.sms",&numBlocks);
	if (!smsBios)
	{
		printf("[ERR] Failed to load sms bios\n");
		glfwTerminate();
		return 1;
	}
#endif

#if ENABLE_32X_MODE
	cpu68kbios=load_rom(BASE_PATH "poormans_sega_32x_bios/BIOS32XG.BIN",&numBlocks);
	if (!cpu68kbios)
	{
		printf("[ERR] Failed to load genesis bios\n");
		glfwTerminate();
		return 1;
	}
	cpu68kbiosSize=romSize;
	
	masterbios=load_rom(BASE_PATH "poormans_sega_32x_bios/BIOS32XM.BIN",&numBlocks);
	if (!masterbios)
	{
		printf("[ERR] Failed to load master bios\n");
		glfwTerminate();
		return 1;
	}
	masterbiosSize=romSize;
	slavebios=load_rom(BASE_PATH "poormans_sega_32x_bios/BIOS32XS.BIN",&numBlocks);
	if (!slavebios)
	{
		printf("[ERR] Failed to load slave bios\n");
		glfwTerminate();
		return 1;
	}
	slavebiosSize=romSize;
#endif

	romPtr=load_rom(romNameExt,&numBlocks);

	if (!romPtr)
	{
		printf("[ERR] Failed to load rom image\n");
		glfwTerminate(); 
		return 1; 
  }

	if (SRAM)
	{
		LoadSRAM(saveName);
	}

	CPU_BuildTable();
	Z80_BuildTable();
	
	MEM_Initialise(romPtr,numBlocks);

#if SMS_MODE
	LoadSRAM(saveName);
#endif

#if ENABLE_32X_MODE

	master=SH2_CreateCPU(SH2_Master_Read,SH2_Master_Write,DEB_Mode_SH2_Master);
	slave=SH2_CreateCPU(SH2_Slave_Read,SH2_Slave_Write,DEB_Mode_SH2_Slave);

	SH2_Reset(master);
	SH2_Reset(slave);

#endif

#if !SMS_MODE
	CPU_Reset();
#endif
	Z80_Reset();

	glfwSetKeyCallback(kbHandler);

#if ENABLE_32X_MODE
	DEB_PauseEmulation(DEB_Mode_SH2_Master,"BOOT");
#else
	DEB_PauseEmulation(DEB_Mode_68000,"BOOT");
#endif

	while (running)
	{
		static int massiveHack=0;
		int debuggerRunning=UpdateDebugger();

		if (!debuggerRunning)
		{
#if ENABLE_32X_MODE
			for (a=0;a<3;a++)
			{
				VDP_32X_Tick();
				SH2_Step(master);			/* 3 steps per 68000 */
				SH2_Step(slave);
			}
#endif

#if !SMS_MODE
			FM_Update();
			CPU_Step();
#endif
/*			if (massiveHack&1) */
			{
				PSG_Update();
				Z80_Step();				/* will be stepping way too quick at present */
			}
			massiveHack++;

/*
			//
			///////////////////////////////////////////////////////////////////////////
			//
			// Need to stop hacking shit in!
			//
			//

			// Approximation of screen to cpu timings (so I can get the irqs firing)
*/
			{
				lineNo= massiveHack/CYCLES_PER_LINE;
				colNo = massiveHack%CYCLES_PER_LINE;

				if (lineNo>223)
				{
#if ENABLE_32X_MODE
					_32XLineCounter=(SH2_HCountRegister&0xFF);
					VDP_32X_ScreenFlipHack();
#endif
					inVBlank=1;
				}
				else
				{
#if ENABLE_32X_MODE
						_32XLineCounter--;
						if (_32XLineCounter==0xFFFFFFFF)
						{
							_32XLineCounter=(SH2_HCountRegister&0xFF);
							if (SH2_Master_AdapterControlRegister&0x04)
							{
								SH2_Interrupt(master,11);
							}
							if (SH2_Slave_AdapterControlRegister&0x04)
							{
								SH2_Interrupt(slave,11);
							}
						}
#endif
					inVBlank=0;
				}
				if (colNo>((CYCLES_PER_LINE*3)/4))
				{
					inHBlank=1;
				}
				else
				{
					inHBlank=0;
				}

				if (colNo==((CYCLES_PER_LINE*3)/4))
				{
#if !SMS_MODE
					U32 displaySizeY = (VDP_Registers[1]&0x08) ? 30*8 : 28*8;			/* not quite true.. ntsc can never be 30! */
					
					if (lineNo>224)
					{
						LineCounter=VDP_Registers[0x0A];
					}
					else
					{
						LineCounter--;
						if (LineCounter==0xFFFFFFFF)
						{
							LineCounter=VDP_Registers[0x0A];
							CPU_SignalInterrupt(4);
						}
					}
#else
					U32 displaySizeY = 192;

					if (lineNo>192)
					{
						LineCounter=VDP_Registers[0x0A];
					}
					else
					{
						LineCounter--;
						if (LineCounter==0xFFFFFFFF)
						{
							LineCounter=VDP_Registers[0x0A];
							SMS_VDP_Status|=0x40;
						}
					}

#endif
					if (lineNo<displaySizeY)
					{
						VID_DrawScreen(lineNo);
					}
				}

#if SMS_MODE
				if ( ((SMS_VDP_Status&0x40) && (VDP_Registers[0x00]&0x10)) || ((SMS_VDP_Status&0x80) && (VDP_Registers[0x01]&0x20)) )
				{	
						Z80_SignalInterrupt(0);
				}
#endif


#if !SMS_MODE
				if ((lineNo == 225) && (colNo == 8))
#else
				if ((lineNo == 193) && (colNo == 0))
#endif
				{
#if ENABLE_32X_MODE
					if (SH2_Master_AdapterControlRegister&0x08)
					{
						SH2_Interrupt(master,15);
					}
					if (SH2_Slave_AdapterControlRegister&0x08)
					{
						SH2_Interrupt(slave,15);
					}
#endif
#if !SMS_MODE
					CPU_SignalInterrupt(6);
					Z80_SignalInterrupt(0);
#else
					SMS_VDP_Status|=0x80;					/* TODO Check if megadrive expects a similar flag? */
#endif
				}
			}


		}
		DisplayDebugger();
		
		UpdateAudio();

		/* ~(450/2) ticks per line		(312 lines per screen @50hz) - */

		if ((massiveHack>=CYCLES_PER_FRAME) || g_newScreenNotify)
		{
			if (massiveHack>=CYCLES_PER_FRAME)
			{
				massiveHack=0;
			}
			
			/*if (debuggerRunning)*/
			{
				double now,remain;
				DrawScreen();
				glfwSwapBuffers();
				now=glfwGetTime();
			
				remain = now-atStart;
				while ((remain<(1.0f/FRAMES_PER_SECOND)) && lockFrameRate)
				{
					now=glfwGetTime();

					remain = now-atStart;
					/*Sleep(20-remain);*/
				}
			}
			atStart=glfwGetTime();
			
			UpdateFPS();

			g_newScreenNotify=0;

			keyStatusJoyA=0;
			SMS_KeyJoyA=0;

			if (KeyDown('S'))
			{
				keyStatusJoyA|=0x2000;
				SMS_KeyJoyA|=0x10;
			}
			if (KeyDown('D'))
			{
				keyStatusJoyA|=0x1000;
				SMS_KeyJoyA|=0x20;
			}
			if (KeyDown(GLFW_KEY_RIGHT))
			{
				keyStatusJoyA|=0x0800;
				SMS_KeyJoyA|=0x08;
			}
			if (KeyDown(GLFW_KEY_LEFT))
			{
				keyStatusJoyA|=0x0400;
				SMS_KeyJoyA|=0x04;
			}
			if (KeyDown(GLFW_KEY_DOWN))
			{
				keyStatusJoyA|=0x0202;
				SMS_KeyJoyA|=0x02;
			}
			if (KeyDown(GLFW_KEY_UP))
			{
				keyStatusJoyA|=0x0101;
				SMS_KeyJoyA|=0x01;
			}
			if (KeyDown(GLFW_KEY_ENTER))
			{
				keyStatusJoyA|=0x0020;
			}
			if (KeyDown('A'))
			{
				keyStatusJoyA|=0x0010;
			}
			if (CheckKey('.'))
			{
				lockFrameRate^=1;
				ClearKey('.');
			}
			if (CheckKey('1'))
			{
				YM_OutOn^=1;
				ClearKey('1');
			}
			if (CheckKey('2'))
			{
				PSG_OutOn^=1;
				ClearKey('2');
			}
			if (CheckKey('5'))
			{
#if ENABLE_32X_MODE
				memset(SH2_68K_COMM_REGISTER,0,16);			/* MASSIVE HACK TO STARTUP SYNC ISSUE */
#endif
				ClearKey('5');
			}
		}
		
		/* Check if ESC key was pressed or window was closed */
		running = /*!glfwGetKey( GLFW_KEY_ESC ) && */glfwGetWindowParam( GLFW_OPENED ); 
	}

	if (SRAM)
	{
		SaveSRAM(saveName);
	}

	/* Close window and terminate GLFW */
	glfwTerminate();

#if OPENAL_SUPPORT
	AudioKill(); 
#endif

	/* Exit program */
	return 0; 
}

#ifdef _WIN32
int WINAPI WinMain(      
									 HINSTANCE hInstance,
									 HINSTANCE hPrevInstance,
									 LPSTR lpCmdLine,
									 int nCmdShow
									 )
{
  AllocConsole();
  freopen ("CONOUT$", "w", stdout ); 

	main(0,0);
}
#endif

#if OPENAL_SUPPORT

#define NUMBUFFERS            (3)				/* living dangerously*/

ALuint		  uiBuffers[NUMBUFFERS];
ALuint		  uiSource;

ALboolean ALFWInitOpenAL()
{
	ALCcontext *pContext = NULL;
	ALCdevice *pDevice = NULL;
	ALboolean bReturn = AL_FALSE;
	
	pDevice = alcOpenDevice(NULL);				/* Request default device*/
	if (pDevice)
	{
		pContext = alcCreateContext(pDevice, NULL);
		if (pContext)
		{
			printf("\nOpened %s Device\n", alcGetString(pDevice, ALC_DEVICE_SPECIFIER));
			alcMakeContextCurrent(pContext);
			bReturn = AL_TRUE;
		}
		else
		{
			alcCloseDevice(pDevice);
		}
	}

	return bReturn;
}

ALboolean ALFWShutdownOpenAL()
{
	ALCcontext *pContext;
	ALCdevice *pDevice;

	pContext = alcGetCurrentContext();
	pDevice = alcGetContextsDevice(pContext);
	
	alcMakeContextCurrent(NULL);
	alcDestroyContext(pContext);
	alcCloseDevice(pDevice);

	return AL_TRUE;
}

#if USE_8BIT_OUTPUT

#define AL_FORMAT						(AL_FORMAT_MONO8)
#define BUFFER_FORMAT				U8
#define BUFFER_FORMAT_SIZE	(1)
#define BUFFER_FORMAT_SHIFT	(8)

#else

#define AL_FORMAT						(AL_FORMAT_MONO16)
#define BUFFER_FORMAT				S16
#define BUFFER_FORMAT_SIZE	(2)
#define BUFFER_FORMAT_SHIFT	(0)

#endif

int curPlayBuffer=0;

#define BUFFER_LEN		(44100/FRAMES_PER_SECOND)

BUFFER_FORMAT audioBuffer[BUFFER_LEN];
int amountAdded=0;

void AudioInitialise()
{
	int a=0;
	for (a=0;a<BUFFER_LEN;a++)
	{
		audioBuffer[a]=0;
	}

	ALFWInitOpenAL();

  /* Generate some AL Buffers for streaming */
	alGenBuffers( NUMBUFFERS, uiBuffers );

	/* Generate a Source to playback the Buffers */
  alGenSources( 1, &uiSource );

	for (a=0;a<NUMBUFFERS;a++)
	{
		alBufferData(uiBuffers[a], AL_FORMAT, audioBuffer, BUFFER_LEN*BUFFER_FORMAT_SIZE, 44100);
		alSourceQueueBuffers(uiSource, 1, &uiBuffers[a]);
	}

	alSourcePlay(uiSource);
}


void AudioKill()
{
	ALFWShutdownOpenAL();
}

S16 currentDAC[2] = {0,0};

void _AudioAddData(int channel,S16 dacValue)
{
	currentDAC[channel]=dacValue;
}

float tickCnt=0;
float tickRate=((CYCLES_PER_FRAME*FRAMES_PER_SECOND)/44100.f);

/* audio ticked at same clock as everything else... so need a step down */
void UpdateAudio()
{
	tickCnt+=1.f;
	
	if (tickCnt>=tickRate)
	{
		tickCnt-=tickRate;

		if (amountAdded!=BUFFER_LEN)
		{
			S32 res=0;
			if (PSG_OutOn)
				res+=currentDAC[1];
			if (YM_OutOn)
				res+=currentDAC[0];
			if (PSG_OutOn && YM_OutOn)
				res>>=1;
			audioBuffer[amountAdded]=res>>BUFFER_FORMAT_SHIFT;
			amountAdded++;
		}
	}

	if (amountAdded==BUFFER_LEN)
		{
			/* 1 second has passed by */

			ALint processed;
			ALint state;
			alGetSourcei(uiSource, AL_SOURCE_STATE, &state);
			alGetSourcei(uiSource, AL_BUFFERS_PROCESSED, &processed);
			if (processed>0)
			{
				ALuint buffer;

				amountAdded=0;
				alSourceUnqueueBuffers(uiSource,1, &buffer);
				alBufferData(buffer, AL_FORMAT, audioBuffer, BUFFER_LEN*BUFFER_FORMAT_SIZE, 44100);
				alSourceQueueBuffers(uiSource, 1, &buffer);
			}

			if (state!=AL_PLAYING)
			{
				alSourcePlay(uiSource);
			}
		}
	

}

#endif


