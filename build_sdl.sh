#!/bin/sh

echo "cd libgambatte && scons"
(cd libgambatte && scons) || exit

echo "cd gambatte_sdl && scons"
(cd gambatte_sdl && scons)
