COMMONPATH = ../../common
include(framework/framework.pro)

SOURCES += main.cpp \
    cheatdialog.cpp \
    fpsselector.cpp \
    gambattesource.cpp \
    gambattemenuhandler.cpp \
    miscdialog.cpp \
    palettedialog.cpp \
    pathselector.cpp
HEADERS += gambattesource.h \
    gambattemenuhandler.h \
    cheatdialog.h \
    fpsselector.h \
    miscdialog.h \
    palettedialog.h \
    pathselector.h
SOURCES += $$COMMONPATH/videolink/rgb32conv.cpp \
    $$COMMONPATH/videolink/vfilterinfo.cpp \
    $$COMMONPATH/videolink/vfilters/catrom2x.cpp \
    $$COMMONPATH/videolink/vfilters/catrom3x.cpp \
    $$COMMONPATH/videolink/vfilters/kreed2xsai.cpp \
    $$COMMONPATH/videolink/vfilters/maxsthq2x.cpp \
    $$COMMONPATH/videolink/vfilters/maxsthq3x.cpp
HEADERS += $$COMMONPATH/videolink/rgb32conv.h \
    $$COMMONPATH/videolink/vfilterinfo.h \
    $$COMMONPATH/videolink/videolink.h \
    $$COMMONPATH/videolink/vfilters/catrom2x.h \
    $$COMMONPATH/videolink/vfilters/catrom3x.h \
    $$COMMONPATH/videolink/vfilters/kreed2xsai.h \
    $$COMMONPATH/videolink/vfilters/maxsthq2x.h \
    $$COMMONPATH/videolink/vfilters/maxsthq3x.h
TEMPLATE = app
CONFIG += warn_on \
    release
QMAKE_CFLAGS   += -fomit-frame-pointer
QMAKE_CXXFLAGS += -fomit-frame-pointer -fno-exceptions -fno-rtti
TARGET = gambatte_qt
macx:TARGET = "Gambatte Qt"
DESTDIR = ../bin
INCLUDEPATH += ../../libgambatte/include
LIBS += -L../../libgambatte -lgambatte -lz
