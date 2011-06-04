/*
 *  platform.h

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


#ifndef _PLATFORM_H
#define _PLATFORM_H

#ifdef _WIN32

#define GLFW_DLL
#include <windows.h>

#include <GL/GL.h>

#include "include/GL/glext.h"
#include "include/GL/glfw.h"

#include"al.h"
#include"alc.h"

#else

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glfw.h>

#include <AL/al.h>
#include <AL/alc.h>

#define UINT64	u_int64_t
#define __int64 int64_t
#endif


#endif/*_PLATFORM_H*/
