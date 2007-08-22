SOURCES += gambatte_qt.cpp \
           main.cpp \
           videodialog.cpp \
           videobufferreseter.cpp \
           blittercontainer.cpp \
           inputdialog.cpp \
           resizesignalingmenubar.cpp \
           samplescalculator.cpp \
           blitterwidgets/qglblitter.cpp \
           blitterwidgets/qpainterblitter.cpp

HEADERS += gambatte_qt.h \
blitterwidget.h \
fullrestoggler.h \
videodialog.h \
videobufferreseter.h \
blittercontainer.h \
resinfo.h \
inputdialog.h \
resizesignalingmenubar.h \
audioengine.h \
samplescalculator.h \
addaudioengines.h \
addblitterwidgets.h \
getfullrestoggler.h \
blitterwidgets/qglblitter.h \
blitterwidgets/qpainterblitter.h \
fullrestogglers/nulltoggler.h

TEMPLATE = app
CONFIG += warn_on \
          thread \
          qt \
          release
QT += opengl
TARGET = ../bin/gambatte_qt
INCLUDEPATH += ../../libgambatte/include
LIBS += -L../../libgambatte -lgambatte

unix {
    DEFINES += PLATFORM_UNIX

    SOURCES += blitterwidget.cpp \
               x11getprocaddress.cpp \
               addblitterwidgets_unix.cpp \
               getfullrestoggler_unix.cpp \
               audioengines/ossengine.cpp \
               blitterwidgets/xvblitter.cpp \
               blitterwidgets/x11blitter.cpp \
               fullrestogglers/xrandrtoggler.cpp

    HEADERS += x11getprocaddress.h \
               audioengines/ossengine.h \
               blitterwidgets/xvblitter.h \
               blitterwidgets/x11blitter.h \
               fullrestogglers/xrandrtoggler.h

    LIBS += -L/usr/X11R6/lib -lXv -lXrandr

    linux-g++ {
        SOURCES += addaudioengines_linux.cpp audioengines/alsaengine.cpp
        HEADERS += audioengines/alsaengine.h
        LIBS += -lasound
    } else {
        SOURCES += addaudioengines_unix.cpp
    }
}

win32 {
    DEFINES += PLATFORM_WIN32

    SOURCES += getfullrestoggler_win32.cpp \
               blitterwidget_win32.cpp \
               addaudioengines_win32.cpp \
               addblitterwidgets_win32.cpp \
               audioengines/directsoundengine.cpp \
               blitterwidgets/directdrawblitter.cpp \
               fullrestogglers/gditoggler.cpp

    HEADERS += audioengines/directsoundengine.h \
               blitterwidgets/directdrawblitter.h \
               fullrestogglers/gditoggler.h

    LIBS += -lwinmm -lddraw -ldxguid -ldsound
}

!win32 : !unix {
        SOURCES += addaudioengines.cpp addblitterwidgets.cpp getfullrestoggler.cpp audioengines/aoengine.cpp
        HEADERS += audioengines/aoengine.h
        LIBS += -lao
    }
