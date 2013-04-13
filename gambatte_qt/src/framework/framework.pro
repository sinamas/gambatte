SOURCES += framework/src/blittercontainer.cpp \
    framework/src/videodialog.cpp \
    framework/src/inputdialog.cpp \
    framework/src/blitterwidgets/qglblitter.cpp \
    framework/src/blitterwidgets/qpainterblitter.cpp \
    framework/src/SDL_Joystick/src/SDL_event.cpp \
    framework/src/SDL_Joystick/src/SDL_error.c \
    framework/src/SDL_Joystick/src/SDL_joystick.c \
    framework/src/SDL_Joystick/src/SDL_string.c \
    framework/src/sounddialog.cpp \
    framework/src/audioengines/customdevconf.cpp \
    framework/src/audioengineconf.cpp \
    framework/src/blitterconf.cpp \
    framework/src/blitterwidget.cpp \
    framework/src/dialoghelpers.cpp \
    framework/src/dwmcontrol.cpp \
    framework/src/frameratecontrol.cpp \
    framework/src/inputbox.cpp \
    framework/src/joysticklock.cpp \
    framework/src/mainwindow.cpp \
    framework/src/mediawidget.cpp \
    framework/src/mediaworker.cpp \
    framework/src/mmpriority.cpp \
    framework/src/sourceupdater.cpp
SOURCES += $$COMMONPATH/resample/src/chainresampler.cpp \
	$$COMMONPATH/resample/src/i0.cpp \
	$$COMMONPATH/resample/src/kaiser50sinc.cpp \
	$$COMMONPATH/resample/src/kaiser70sinc.cpp \
	$$COMMONPATH/resample/src/makesinckernel.cpp \
	$$COMMONPATH/resample/src/u48div.cpp \
	$$COMMONPATH/resample/src/resamplerinfo.cpp \
	$$COMMONPATH/adaptivesleep.cpp \
	$$COMMONPATH/rateest.cpp \
	$$COMMONPATH/skipsched.cpp
HEADERS += \
	framework/include/*.h \
	framework/src/*.h \
	framework/src/audioengines/*.h \
	framework/src/blitterwidgets/*.h \
	framework/src/fullmodetogglers/nulltoggler.h \
	framework/src/SDL_Joystick/include/*.h \
	framework/src/SDL_Joystick/src/*.h
HEADERS += \
	$$COMMONPATH/resample/src/*.h \
	$$COMMONPATH/resample/*.h \
	$$COMMONPATH/*.h
CONFIG += qt thread
QT += opengl
INCLUDEPATH += framework/include
INCLUDEPATH += framework/src/SDL_Joystick/include
INCLUDEPATH += $$COMMONPATH
DEPENDPATH  += $$COMMONPATH
DEPENDPATH  += framework/include
DEFINES += HAVE_STDINT_H

macx {
#    CONFIG += x86 ppc
#    QMAKE_CFLAGS += -Xarch_ppc -DWORDS_BIGENDIAN
#    QMAKE_CXXFLAGS += -fvisibility=hidden -Xarch_ppc -DWORDS_BIGENDIAN
#    QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.4u.sdk
#    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.3
    SOURCES += framework/src/addaudioengines_macx.cpp \
        framework/src/addblitterwidgets.cpp \
        framework/src/getfullmodetoggler_macx.cpp
    SOURCES += framework/src/SDL_Joystick/src/darwin/SDL_sysjoystick.c \
#        framework/src/audioengines/openalengine.cpp \
        framework/src/audioengines/coreaudioengine.cpp \
        framework/src/fullmodetogglers/quartztoggler.cpp
    HEADERS += framework/src/fullmodetogglers/quartztoggler.h
#	LIBS += -dead_strip
    LIBS += -framework IOKit \
            -framework CoreServices \
            -framework CoreFoundation \
            -framework Carbon \
            -framework ApplicationServices \
#	    -framework OpenAL \
            -framework AudioUnit
}
else:unix { 
    DEFINES += PLATFORM_UNIX
    SOURCES += \
        framework/src/addblitterwidgets_unix.cpp \
        framework/src/getfullmodetoggler_unix.cpp \
        framework/src/audioengines/ossengine.cpp \
        framework/src/blitterwidgets/xvblitter.cpp \
        framework/src/blitterwidgets/x11blitter.cpp \
        framework/src/fullmodetogglers/xrandrtoggler.cpp \
        framework/src/fullmodetogglers/xrandr12toggler.cpp #\
#        framework/src/fullmodetogglers/xf86vidmodetoggler.cpp
    HEADERS += \
        framework/src/fullmodetogglers/xrandrtoggler.h \
        framework/src/fullmodetogglers/xrandr12toggler.h #\
#        framework/src/fullmodetogglers/xf86vidmodetoggler.h
    LIBS += -L/usr/X11R6/lib \
        -Wl,--as-needed \
        -lX11 \
        -lXext \
        -lXv \
        -lXrandr #\
#        -lXxf86vm \
#        -lXinerama
    linux-* { 
        SOURCES += framework/src/addaudioengines_linux.cpp \
            framework/src/audioengines/alsaengine.cpp \
            framework/src/SDL_Joystick/src/linux/SDL_sysjoystick.c
        LIBS += -lasound
    }
    else { 
        SOURCES += framework/src/addaudioengines_unix.cpp
        freebsd-*|netbsd-*|openbsd-* { 
            exists( /usr/include/usb.h ):DEFINES += HAVE_USB_H
            exists( /usr/include/usbhid.h ):DEFINES += HAVE_USBHID_H
            exists( /usr/include/libusb.h ):DEFINES += HAVE_LIBUSB_H
            exists( /usr/include/libusbhid.h ):DEFINES += HAVE_LIBUSBHID_H
            SOURCES += framework/src/SDL_Joystick/src/bsd/SDL_sysjoystick.c
            openbsd-*:DEFINES += USBHID_NEW USBHID_UCR_DATA
            openbsd-*:LIBS += -lusbhid -lossaudio
        }
        else:darwin-*:SOURCES += framework/src/SDL_Joystick/src/darwin/SDL_sysjoystick.c
        else:SOURCES += framework/src/SDL_Joystick/src/dummy/SDL_sysjoystick.c
    }
}
else:win32 { 
    DEFINES += PLATFORM_WIN32
    SOURCES += framework/src/gdisettings.cpp \
        framework/src/blitterwidgets/direct3dblitter.cpp \
        framework/src/getfullmodetoggler_win32.cpp \
        framework/src/addaudioengines_win32.cpp \
        framework/src/addblitterwidgets_win32.cpp \
        framework/src/audioengines/directsoundengine.cpp \
        framework/src/audioengines/wasapiengine.cpp \
        framework/src/blitterwidgets/directdrawblitter.cpp \
        framework/src/fullmodetogglers/gditoggler.cpp \
        framework/src/SDL_Joystick/src/win32/SDL_mmjoystick.c
    HEADERS += framework/src/fullmodetogglers/gditoggler.h
    LIBS += -lwinmm \
        -lddraw \
        -ldxguid \
        -ldsound
}
else { 
    SOURCES += framework/src/addaudioengines.cpp \
        framework/src/addblitterwidgets.cpp \
        framework/src/getfullmodetoggler.cpp \
        framework/src/blitterwidget.cpp \
        framework/src/audioengines/aoengine.cpp \
        framework/src/audioengines/openalengine.cpp
    SOURCES += framework/src/SDL_Joystick/src/dummy/SDL_sysjoystick.c
    CONFIG += link_pkgconfig
    PKGCONFIG += ao openal
}
