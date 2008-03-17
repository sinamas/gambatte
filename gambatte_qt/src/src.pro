SOURCES += main.cpp \
    videodialog.cpp \
    blittercontainer.cpp \
    inputdialog.cpp \
    samplescalculator.cpp \
    blitterwidgets/qglblitter.cpp \
    blitterwidgets/qpainterblitter.cpp \
    SDL_Joystick/src/SDL_event.cpp \
    SDL_Joystick/src/SDL_error.c \
    SDL_Joystick/src/SDL_joystick.c \
    SDL_Joystick/src/SDL_string.c \
    sounddialog.cpp \
    audioengines/customdevconf.cpp \
    palettedialog.cpp \
    gambattesource.cpp \
    gambattemenuhandler.cpp \
    mainwindow.cpp \
    blitterwidget.cpp
HEADERS += blitterwidget.h \
    fullmodetoggler.h \
    videodialog.h \
    blittercontainer.h \
    resinfo.h \
    inputdialog.h \
    audioengine.h \
    samplescalculator.h \
    addaudioengines.h \
    addblitterwidgets.h \
    getfullmodetoggler.h \
    blitterwidgets/qglblitter.h \
    blitterwidgets/qpainterblitter.h \
    fullmodetogglers/nulltoggler.h \
    SDL_Joystick/include/SDL_config.h \
    SDL_Joystick/include/SDL_error.h \
    SDL_Joystick/include/SDL_event.h \
    SDL_Joystick/include/SDL_joystick.h \
    SDL_Joystick/include/SDL_stdinc.h \
    SDL_Joystick/src/SDL_error_c.h \
    SDL_Joystick/src/SDL_joystick_c.h \
    SDL_Joystick/src/SDL_sysjoystick.h \
    sounddialog.h \
    audioengines/nullaudioengine.h \
    audioengines/customdevconf.h \
    palettedialog.h \
    mediasource.h \
    gambattesource.h \
    pixelbuffersetter.h \
    gambattemenuhandler.h \
    mainwindow.h \
    swscale.h
TEMPLATE = app
CONFIG += warn_on \
    thread \
    qt \
    release
QT += opengl
TARGET = ../bin/gambatte_qt
INCLUDEPATH += ../../libgambatte/include \
    SDL_Joystick/include
LIBS += -L../../libgambatte \
    -lgambatte
DEFINES += HAVE_STDINT_H

# QMAKE_CXXFLAGS = -g
macx { 
    SOURCES += addaudioengines_macx.cpp \
        addblitterwidgets.cpp \
        getfullmodetoggler_macx.cpp
    SOURCES += SDL_Joystick/src/darwin/SDL_sysjoystick.c \
        audioengines/openalengine.cpp \
        fullmodetogglers/quartztoggler.cpp
    HEADERS += audioengines/openalengine.h \
        fullmodetogglers/quartztoggler.h
    LIBS += -framework \
        IOKit \
        -framework \
        OpenAL
}
else:unix { 
    DEFINES += PLATFORM_UNIX
    SOURCES += x11getprocaddress.cpp \
        addblitterwidgets_unix.cpp \
        getfullmodetoggler_unix.cpp \
        audioengines/ossengine.cpp \
        blitterwidgets/xvblitter.cpp \
        blitterwidgets/x11blitter.cpp \
        fullmodetogglers/xrandrtoggler.cpp \
        fullmodetogglers/xf86vidmodetoggler.cpp
    HEADERS += x11getprocaddress.h \
        audioengines/ossengine.h \
        blitterwidgets/xvblitter.h \
        blitterwidgets/x11blitter.h \
        fullmodetogglers/xrandrtoggler.h \
        fullmodetogglers/xf86vidmodetoggler.h
    LIBS += -L/usr/X11R6/lib \
        -lXv \
        -lXrandr
    linux-g++ { 
        SOURCES += addaudioengines_linux.cpp \
            audioengines/alsaengine.cpp \
            SDL_Joystick/src/linux/SDL_sysjoystick.c
        HEADERS += audioengines/alsaengine.h
        LIBS += -lasound
    }
    else { 
        SOURCES += addaudioengines_unix.cpp
        freebsd-g++|netbsd-g++|openbsd-g++ { 
            exists( /usr/include/usb.h ):DEFINES += HAVE_USB_H
            exists( /usr/include/usbhid.h ):DEFINES += HAVE_USBHID_H
            exists( /usr/include/libusb.h ):DEFINES += HAVE_LIBUSB_H
            exists( /usr/include/libusbhid.h ):DEFINES += HAVE_LIBUSBHID_H
            SOURCES += SDL_Joystick/src/bsd/SDL_sysjoystick.c
        }
        else:darwin-g++:SOURCES += SDL_Joystick/src/darwin/SDL_sysjoystick.c
        else:SOURCES += SDL_Joystick/src/dummy/SDL_sysjoystick.c
    }
}
else:win32 { 
    DEFINES += PLATFORM_WIN32
    SOURCES += getfullmodetoggler_win32.cpp \
        addaudioengines_win32.cpp \
        addblitterwidgets_win32.cpp \
        audioengines/directsoundengine.cpp \
        blitterwidgets/directdrawblitter.cpp \
        fullmodetogglers/gditoggler.cpp \
        SDL_Joystick/src/win32/SDL_mmjoystick.c
    HEADERS += audioengines/directsoundengine.h \
        blitterwidgets/directdrawblitter.h \
        fullmodetogglers/gditoggler.h
    LIBS += -lwinmm \
        -lddraw \
        -ldxguid \
        -ldsound
}
else { 
    SOURCES += addaudioengines.cpp \
        addblitterwidgets.cpp \
        getfullmodetoggler.cpp \
        blitterwidget.cpp \
        audioengines/aoengine.cpp
    SOURCES += SDL_Joystick/src/dummy/SDL_sysjoystick.c
    HEADERS += audioengines/aoengine.h
    CONFIG += link_pkgconfig
    PKGCONFIG += ao
}
