/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

/* Simple error message routines for SDL */

#ifndef _SDL_error_h
#define _SDL_error_h

#include "SDL_stdinc.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Public functions */
extern void SDL_SetError(const char *fmt, ...);
extern char * SDL_GetError(void);
extern void SDL_ClearError(void);

/* Private error message function - used internally */
#define SDL_OutOfMemory()	SDL_Error(SDL_ENOMEM)
#define SDL_Unsupported()	SDL_Error(SDL_UNSUPPORTED)
typedef enum {
	SDL_ENOMEM,
	SDL_EFREAD,
	SDL_EFWRITE,
	SDL_EFSEEK,
	SDL_UNSUPPORTED,
	SDL_LASTERROR
} SDL_errorcode;
extern void SDL_Error(SDL_errorcode code);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _SDL_error_h */
