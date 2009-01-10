COMMONPATH = ../../common
include(framework/framework.pro)

SOURCES += main.cpp \
    gambattesource.cpp \
    gambattemenuhandler.cpp \
    palettedialog.cpp
HEADERS += gambattesource.h \
    gambattemenuhandler.h \
    palettedialog.h
TEMPLATE = app
CONFIG += warn_on \
    release
TARGET = gambatte_qt
macx:TARGET = "Gambatte Qt"
DESTDIR = ../bin
INCLUDEPATH += ../../libgambatte/include
LIBS += -L../../libgambatte -lgambatte
