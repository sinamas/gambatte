#!/bin/sh

echo "cd libgambatte && scons"
(cd libgambatte && scons) || exit

echo "cd gambatte_qt && qmake && make"
(cd gambatte_qt && qmake && make)
