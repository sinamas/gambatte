# GNU Make 3.x doesn't support the "!=" shell syntax, so here's an alternative

SDL_LFLAGS = $(shell $(PKG_CONFIG) --libs sdl)
ZLIB_LFLAGS = $(shell $(PKG_CONFIG) --libs zlib)
PKGCONFIG_CFLAGS = $(shell $(PKG_CONFIG) --cflags sdl libpng zlib)
PNG_LFLAGS = $(shell $(PKG_CONFIG) --libs libpng)

include Makefile
