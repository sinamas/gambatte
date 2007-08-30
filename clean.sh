#!/bin/sh

echo "cd gambatte_qt && make distclean"
(cd gambatte_qt && make distclean)

echo "cd gambatte_sdl && scons -c"
(cd gambatte_sdl && scons -c)

echo "cd libgambatte && scons -c"
(cd libgambatte && scons -c)

echo "rm -f *gambatte*/config.log"
rm -f *gambatte*/config.log

echo "rm -rf *gambatte*/.scon*"
rm -rf *gambatte*/.scon*
