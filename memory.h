/*
 *  memory.h

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

#include "mytypes.h"

U8 Z80_MEM_getByte(U16 address);
void Z80_MEM_setByte(U16 address,U8 data);

U8 Z80_IO_getByte(U16 address);
void Z80_IO_setByte(U16 address,U8 data);

void MEM_SaveState(FILE *outStream);
void MEM_LoadState(FILE *inStream);

void MEM_Initialise(unsigned char *_romPtr,unsigned int num64Banks);

void MEM_MapKickstartLow(int yes);

U8	MEM_getByte(U32 address);
void		MEM_setByte(U32 address,U8 byte);
U16	MEM_getWord(U32 address);
void		MEM_setWord(U32 address,U16 word);
U32	MEM_getLong(U32 address);
void		MEM_setLong(U32 address,U32 dword);
