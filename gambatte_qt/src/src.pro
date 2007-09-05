SOURCES += gambatte_qt.cpp \
           main.cpp \
           videodialog.cpp \
           videobufferreseter.cpp \
           blittercontainer.cpp \
           inputdialog.cpp \
           resizesignalingmenubar.cpp \
           samplescalculator.cpp \
           blitterwidgets/qglblitter.cpp \
           blitterwidgets/qpainterblitter.cpp \
           SDL_Joystick/src/SDL_event.cpp \
           SDL_Joystick/src/SDL_error.c \
           SDL_Joystick/src/SDL_joystick.c \
           SDL_Joystick/src/SDL_string.c

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
fullrestogglers/nulltoggler.h \
SDL_Joystick/include/SDL_config.h \
SDL_Joystick/include/SDL_error.h \
SDL_Joystick/include/SDL_event.h \
SDL_Joystick/include/SDL_joystick.h \
SDL_Joystick/include/SDL_stdinc.h \
SDL_Joystick/src/SDL_error_c.h \
SDL_Joystick/src/SDL_joystick_c.h \
SDL_Joystick/src/SDL_sysjoystick.h

TEMPLATE = app
CONFIG += warn_on \
          thread \
          qt \
          release
QT += opengl
TARGET = ../bin/gambatte_qt
INCLUDEPATH += ../../libgambatte/include SDL_Joystick/include
LIBS += -L../../libgambatte -lgambatte -lz

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
        SOURCES += addaudioengines_linux.cpp audioengines/alsaengine.cpp SDL_Joystick/src/linux/SDL_sysjoystick.c
        HEADERS += audioengines/alsaengine.h
        LIBS += -lasound
    } else {
        SOURCES += addaudioengines_unix.cpp

        freebsd-g++|netbsd-g++|openbsd-g++ {
            exists( /usr/include/usb.h ):DEFINES += HAVE_USB_H
            exists( /usr/include/usbhid.h ):DEFINES += HAVE_USBHID_H
            exists( /usr/include/libusb.h ):DEFINES += HAVE_LIBUSB_H
            exists( /usr/include/libusbhid.h ):DEFINES += HAVE_LIBUSBHID_H
            SOURCES += SDL_Joystick/src/bsd/SDL_sysjoystick.c
        } else:darwin-g++ {
            SOURCES += SDL_Joystick/src/darwin/SDL_sysjoystick.c
        } else {
            SOURCES += SDL_Joystick/src/dummy/SDL_sysjoystick.c
        }
    }
} else:win32 {
    DEFINES += PLATFORM_WIN32

    SOURCES += getfullrestoggler_win32.cpp \
               blitterwidget_win32.cpp \
               addaudioengines_win32.cpp \
               addblitterwidgets_win32.cpp \
               audioengines/directsoundengine.cpp \
               blitterwidgets/directdrawblitter.cpp \
               fullrestogglers/gditoggler.cpp \
               SDL_Joystick/src/win32/SDL_mmjoystick.c

    HEADERS += audioengines/directsoundengine.h \
               blitterwidgets/directdrawblitter.h \
               fullrestogglers/gditoggler.h

    LIBS += -lwinmm -lddraw -ldxguid -ldsound
} else {
    SOURCES += addaudioengines.cpp addblitterwidgets.cpp getfullrestoggler.cpp audioengines/aoengine.cpp
    HEADERS += audioengines/aoengine.h
    LIBS += -lao

    macx {
        SOURCES += SDL_Joystick/src/darwin/SDL_sysjoystick.c
    } else {
        SOURCES += SDL_Joystick/src/dummy/SDL_sysjoystick.c
    }
}
