/*
 * config.h

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

#ifndef __CONFIG_H
#define __CONFIG_H

#define LINE_LENGTH			((228)*4)					/* 228 / 227 alternating */
#define WIDTH 					(LINE_LENGTH)
#define HEIGHT 					(262*2)
#define BPP							(4)
#define DEPTH						(32)

#define CYCLES_PER_LINE			(450/2)
#define CYCLES_PER_FRAME		(CYCLES_PER_LINE*312)
#define FRAMES_PER_SECOND		(60)

#define PAL_PRETEND					0

#define ENABLE_DEBUGGER			1

#define	ENABLE_32X_MODE			0

#define SMS_MODE						0			/* Attempt to emulate SMS using megadrive core */
#define ENABLE_SMS_BIOS			0
#define SMS_CART_MISSING		0

#define OPENAL_SUPPORT			1
#define USE_8BIT_OUTPUT			0

#define UNUSED_ARGUMENT(x)		(void)x

#include "gui/debugger.h"

#endif/*__CONFIG_H*/

