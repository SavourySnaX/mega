/*
 * mytypes

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

#ifndef __MYTYPES_H
#define __MYTYPES_H

#define SOFT_BREAK	{ char* bob=0; 	*bob=0; }

#ifndef __IGNORE_TYPES
#ifndef _INT8_T
#define _INT8_T 
typedef char              S8;
#endif
                               
#ifndef _INT16_T
#define _INT16_T 
typedef  short            S16;
#endif
                               
#ifndef _INT32_T
#define _INT32_T 
typedef  int              S32;
#endif
#endif
typedef  unsigned short U16;
typedef unsigned char   U8; 
typedef  unsigned int   U32;

#endif/*__MYTYPES_H*/

