/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
 *   aamas@stud.ntnu.no                                                    *
 *                                                                         *
 *   Based on source code from The OpenGL Extension Wrangler Library.      *
 *   Copyright (C) 2002-2007, Milan Ikits <milan ikits[]ieee org>          *
 *   Copyright (C) 2002-2007, Marcelo E. Magallon <mmagallo[]debian org>   *
 *   Copyright (C) 2002, Lev Povalahev                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License version 2 for more details.                *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   version 2 along with this program; if not, write to the               *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "x11getprocaddress.h"

#include <dlfcn.h>
#include <GL/gl.h>

void* x11GetProcAddress(const char* name) {
	static void* h = NULL;
	static void* gpa;
	
	if (h == NULL) {
		if ((h = dlopen(NULL, RTLD_LAZY | RTLD_LOCAL)) == NULL)
			return NULL;
		
		if ((gpa = dlsym(h, "glXGetProcAddress")) == NULL)
			gpa = dlsym(h, "glXGetProcAddressARB");
	}
	
	if (gpa != NULL)
		return (reinterpret_cast<void*(*)(const GLubyte*)>(gpa))((const GLubyte*)name);
	else
		return dlsym(h, name);
}
