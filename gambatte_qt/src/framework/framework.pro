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
    framework/src/dwmcontrol.cpp \
    framework/src/frameratecontrol.cpp \
    framework/src/mainwindow.cpp \
    framework/src/mediawidget.cpp \
    framework/src/mediaworker.cpp \
    framework/src/mmpriority.cpp \
    framework/src/samplebuffer.cpp \
    framework/src/blitterwidget.cpp \
    framework/src/joysticklock.cpp \
    framework/src/persistcheckbox.cpp \
    framework/src/inputbox.cpp
SOURCES += $$COMMONPATH/resample/src/chainresampler.cpp \
	$$COMMONPATH/resample/src/i0.cpp \
	$$COMMONPATH/resample/src/makesinckernel.cpp \
	$$COMMONPATH/resample/src/u48div.cpp \
	$$COMMONPATH/resample/src/resamplerinfo.cpp \
	$$COMMONPATH/adaptivesleep.cpp \
	$$COMMONPATH/rateest.cpp \
	$$COMMONPATH/skipsched.cpp
HEADERS += framework/include/audioengineconf.h \
    framework/include/auto_vector.h \
    framework/include/callqueue.h \
    framework/include/inputdialog.h \
    framework/include/mainwindow.h \
    framework/include/mediasource.h \
    framework/include/mutual.h \
    framework/include/persistcheckbox.h \
    framework/include/pixelbuffer.h \
    framework/include/resinfo.h \
    framework/include/scalingmethod.h \
    framework/include/sounddialog.h \
    framework/include/videodialog.h \
    framework/src/audioengine.h \
    framework/src/blittercontainer.h \
    framework/src/blitterwidget.h \
    framework/src/dwmcontrol.h \
    framework/src/frameratecontrol.h \
    framework/src/fullmodetoggler.h \
    framework/src/mmpriority.h \
    framework/src/addaudioengines.h \
    framework/src/addblitterwidgets.h \
    framework/src/getfullmodetoggler.h \
    framework/src/blitterwidgets/qglblitter.h \
    framework/src/blitterwidgets/qpainterblitter.h \
    framework/src/fullmodetogglers/nulltoggler.h \
    framework/src/SDL_Joystick/include/SDL_config.h \
    framework/src/SDL_Joystick/include/SDL_error.h \
    framework/src/SDL_Joystick/include/SDL_event.h \
    framework/src/SDL_Joystick/include/SDL_joystick.h \
    framework/src/SDL_Joystick/include/SDL_stdinc.h \
    framework/src/SDL_Joystick/src/SDL_error_c.h \
    framework/src/SDL_Joystick/src/SDL_joystick_c.h \
    framework/src/SDL_Joystick/src/SDL_sysjoystick.h \
    framework/src/audioengines/nullaudioengine.h \
    framework/src/audioengines/customdevconf.h \
    framework/src/mediawidget.h \
    framework/src/mediaworker.h \
    framework/src/samplebuffer.h \
    framework/src/swscale.h \
    framework/src/rational.h \
    framework/src/joysticklock.h \
    framework/src/syncvar.h \
    framework/src/atomicvar.h \
    framework/src/inputbox.h
HEADERS += $$COMMONPATH/resample/src/chainresampler.h \
	$$COMMONPATH/resample/src/cic2.h \
	$$COMMONPATH/resample/src/cic3.h \
	$$COMMONPATH/resample/src/cic4.h \
	$$COMMONPATH/resample/src/convoluter.h \
	$$COMMONPATH/resample/src/i0.h \
	$$COMMONPATH/resample/src/kaiser50sinc.h \
	$$COMMONPATH/resample/src/kaiser70sinc.h \
	$$COMMONPATH/resample/src/linint.h \
	$$COMMONPATH/resample/src/makesinckernel.h \
	$$COMMONPATH/resample/src/rectsinc.h \
	$$COMMONPATH/resample/resampler.h \
	$$COMMONPATH/resample/src/rshift16_round.h \
	$$COMMONPATH/resample/src/subresampler.h \
	$$COMMONPATH/resample/src/u48div.h \
	$$COMMONPATH/resample/src/upsampler.h \
	$$COMMONPATH/resample/resamplerinfo.h \
	$$COMMONPATH/adaptivesleep.h \
	$$COMMONPATH/usec.h \
	$$COMMONPATH/rateest.h \
	$$COMMONPATH/uncopyable.h \
	$$COMMONPATH/skipsched.h
CONFIG += qt thread
QT += opengl
INCLUDEPATH += framework/include
INCLUDEPATH += framework/src/SDL_Joystick/include
INCLUDEPATH += $$COMMONPATH
DEFINES += HAVE_STDINT_H

macx {
#    CONFIG += x86 ppc
#    QMAKE_CFLAGS += -Xarch_ppc -DWORDS_BIGENDIAN
#    QMAKE_CXXFLAGS += -fvisibility=hidden -Xarch_ppc -DWORDS_BIGENDIAN
#    QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.4u.sdk
#    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.3
    HEADERS += $$COMMONPATH/ringbuffer.h
    SOURCES += framework/src/addaudioengines_macx.cpp \
        framework/src/addblitterwidgets.cpp \
        framework/src/getfullmodetoggler_macx.cpp
    SOURCES += framework/src/SDL_Joystick/src/darwin/SDL_sysjoystick.c \
#        framework/src/audioengines/openalengine.cpp \
        framework/src/audioengines/coreaudioengine.cpp \
        framework/src/fullmodetogglers/quartztoggler.cpp
    HEADERS += framework/src/audioengines/coreaudioengine.h \
#	    framework/src/audioengines/openalengine.h \
        framework/src/fullmodetogglers/quartztoggler.h
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
    SOURCES += framework/src/x11getprocaddress.cpp \
        framework/src/addblitterwidgets_unix.cpp \
        framework/src/getfullmodetoggler_unix.cpp \
        framework/src/audioengines/ossengine.cpp \
        framework/src/blitterwidgets/xvblitter.cpp \
        framework/src/blitterwidgets/x11blitter.cpp \
        framework/src/fullmodetogglers/xrandrtoggler.cpp \
        framework/src/fullmodetogglers/xrandr12toggler.cpp #\
#        framework/src/fullmodetogglers/xf86vidmodetoggler.cpp
    HEADERS += framework/src/x11getprocaddress.h \
        framework/src/audioengines/ossengine.h \
        framework/src/blitterwidgets/xvblitter.h \
        framework/src/blitterwidgets/x11blitter.h \
        framework/src/fullmodetogglers/xrandrtoggler.h \
        framework/src/fullmodetogglers/xrandr12toggler.h #\
#        framework/src/fullmodetogglers/xf86vidmodetoggler.h
    LIBS += -L/usr/X11R6/lib \
        --Wl,--as-needed \
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
        HEADERS += framework/src/audioengines/alsaengine.h
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
    HEADERS += framework/src/gdisettings.h \
        framework/src/blitterwidgets/direct3dblitter.h \
        framework/src/audioengines/directsoundengine.h \
        framework/src/audioengines/wasapiengine.h \
        framework/src/blitterwidgets/directdrawblitter.h \
        framework/src/fullmodetogglers/gditoggler.h
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
        framework/src/audioengines/aoengine.cpp
    SOURCES += framework/src/SDL_Joystick/src/dummy/SDL_sysjoystick.c
    HEADERS += framework/src/audioengines/aoengine.h
    CONFIG += link_pkgconfig
    PKGCONFIG += ao
}
