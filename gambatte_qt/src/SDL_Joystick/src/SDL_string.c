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
#include "SDL_config.h"

/* This file contains portable string manipulation functions for SDL */

#include "SDL_stdinc.h"


#define SDL_isupperhex(X)   (((X) >= 'A') && ((X) <= 'F'))
#define SDL_islowerhex(X)   (((X) >= 'a') && ((X) <= 'f'))

#ifndef HAVE_STRLCPY
size_t SDL_strlcpy(char *dst, const char *src, size_t maxlen)
{
    size_t srclen = SDL_strlen(src);
    if ( maxlen > 0 ) {
        size_t len = SDL_min(srclen, maxlen-1);
        SDL_memcpy(dst, src, len);
        dst[len] = '\0';
    }
    return srclen;
}
#endif

#ifndef HAVE_STRDUP
char *SDL_strdup(const char *string)
{
    size_t len = SDL_strlen(string)+1;
    char *newstr = SDL_malloc(len);
    if ( newstr ) {
        SDL_strlcpy(newstr, string, len);
    }
    return newstr;
}
#endif
