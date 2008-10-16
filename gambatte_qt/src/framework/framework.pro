SOURCES += framework/videodialog.cpp \
    framework/blittercontainer.cpp \
    framework/inputdialog.cpp \
    framework/blitterwidgets/qglblitter.cpp \
    framework/blitterwidgets/qpainterblitter.cpp \
    framework/SDL_Joystick/src/SDL_event.cpp \
    framework/SDL_Joystick/src/SDL_error.c \
    framework/SDL_Joystick/src/SDL_joystick.c \
    framework/SDL_Joystick/src/SDL_string.c \
    framework/sounddialog.cpp \
    framework/audioengines/customdevconf.cpp \
    framework/mainwindow.cpp \
    framework/blitterwidget.cpp
SOURCES += $$COMMONPATH/resample/chainresampler.cpp \
	$$COMMONPATH/resample/u48div.cpp \
	$$COMMONPATH/resample/resamplerinfo.cpp \
	$$COMMONPATH/adaptivesleep.cpp \
	$$COMMONPATH/rateest.cpp
HEADERS += framework/blitterwidget.h \
    framework/fullmodetoggler.h \
    framework/videodialog.h \
    framework/blittercontainer.h \
    framework/resinfo.h \
    framework/inputdialog.h \
    framework/audioengine.h \
    framework/addaudioengines.h \
    framework/addblitterwidgets.h \
    framework/getfullmodetoggler.h \
    framework/blitterwidgets/qglblitter.h \
    framework/blitterwidgets/qpainterblitter.h \
    framework/fullmodetogglers/nulltoggler.h \
    framework/SDL_Joystick/include/SDL_config.h \
    framework/SDL_Joystick/include/SDL_error.h \
    framework/SDL_Joystick/include/SDL_event.h \
    framework/SDL_Joystick/include/SDL_joystick.h \
    framework/SDL_Joystick/include/SDL_stdinc.h \
    framework/SDL_Joystick/src/SDL_error_c.h \
    framework/SDL_Joystick/src/SDL_joystick_c.h \
    framework/SDL_Joystick/src/SDL_sysjoystick.h \
    framework/sounddialog.h \
    framework/audioengines/nullaudioengine.h \
    framework/audioengines/customdevconf.h \
    framework/mediasource.h \
    framework/pixelbuffersetter.h \
    framework/mainwindow.h \
    framework/swscale.h \
    framework/rational.h
HEADERS += $$COMMONPATH/resample/blackmansinc.h \
	$$COMMONPATH/resample/chainresampler.h \
	$$COMMONPATH/resample/cic2.h \
	$$COMMONPATH/resample/cic3.h \
	$$COMMONPATH/resample/cic4.h \
	$$COMMONPATH/resample/convoluter.h \
	$$COMMONPATH/resample/hammingsinc.h \
	$$COMMONPATH/resample/linint.h \
	$$COMMONPATH/resample/makesinckernel.h \
	$$COMMONPATH/resample/rectsinc.h \
	$$COMMONPATH/resample/resampler.h \
	$$COMMONPATH/resample/subresampler.h \
	$$COMMONPATH/resample/u48div.h \
	$$COMMONPATH/resample/upsampler.h \
	$$COMMONPATH/resample/resamplerinfo.h \
	$$COMMONPATH/adaptivesleep.h \
	$$COMMONPATH/usec.h \
	$$COMMONPATH/rateest.h
CONFIG += qt
QT += opengl
INCLUDEPATH += framework/SDL_Joystick/include
INCLUDEPATH += $$COMMONPATH
DEFINES += HAVE_STDINT_H

# QMAKE_CXXFLAGS = -g
macx { 
    SOURCES += framework/addaudioengines_macx.cpp \
        framework/addblitterwidgets.cpp \
        framework/getfullmodetoggler_macx.cpp
    SOURCES += framework/SDL_Joystick/src/darwin/SDL_sysjoystick.c \
        framework/audioengines/openalengine.cpp \
        framework/fullmodetogglers/quartztoggler.cpp
    HEADERS += framework/audioengines/openalengine.h \
        framework/fullmodetogglers/quartztoggler.h
    LIBS += -framework \
        IOKit \
        -framework \
        OpenAL
}
else:unix { 
    DEFINES += PLATFORM_UNIX
    SOURCES += framework/x11getprocaddress.cpp \
        framework/addblitterwidgets_unix.cpp \
        framework/getfullmodetoggler_unix.cpp \
        framework/audioengines/ossengine.cpp \
        framework/blitterwidgets/xvblitter.cpp \
        framework/blitterwidgets/x11blitter.cpp \
        framework/fullmodetogglers/xrandrtoggler.cpp #\
#        framework/fullmodetogglers/xf86vidmodetoggler.cpp
    HEADERS += framework/x11getprocaddress.h \
        framework/audioengines/ossengine.h \
        framework/blitterwidgets/xvblitter.h \
        framework/blitterwidgets/x11blitter.h \
        framework/fullmodetogglers/xrandrtoggler.h #\
#        framework/fullmodetogglers/xf86vidmodetoggler.h
    LIBS += -L/usr/X11R6/lib \
        -lXv \
        -lXrandr #\
#        -lXxf86vm \
#        -lXinerama
    linux-g++ { 
        SOURCES += framework/addaudioengines_linux.cpp \
            framework/audioengines/alsaengine.cpp \
            framework/SDL_Joystick/src/linux/SDL_sysjoystick.c
        HEADERS += framework/audioengines/alsaengine.h
        LIBS += -lasound
    }
    else { 
        SOURCES += framework/addaudioengines_unix.cpp
        freebsd-g++|netbsd-g++|openbsd-g++ { 
            exists( /usr/include/usb.h ):DEFINES += HAVE_USB_H
            exists( /usr/include/usbhid.h ):DEFINES += HAVE_USBHID_H
            exists( /usr/include/libusb.h ):DEFINES += HAVE_LIBUSB_H
            exists( /usr/include/libusbhid.h ):DEFINES += HAVE_LIBUSBHID_H
            SOURCES += framework/SDL_Joystick/src/bsd/SDL_sysjoystick.c
        }
        else:darwin-g++:SOURCES += framework/SDL_Joystick/src/darwin/SDL_sysjoystick.c
        else:SOURCES += framework/SDL_Joystick/src/dummy/SDL_sysjoystick.c
    }
}
else:win32 { 
    DEFINES += PLATFORM_WIN32
    SOURCES += framework/gdisettings.cpp \
        framework/blitterwidgets/direct3dblitter.cpp \
        framework/getfullmodetoggler_win32.cpp \
        framework/addaudioengines_win32.cpp \
        framework/addblitterwidgets_win32.cpp \
        framework/audioengines/directsoundengine.cpp \
        framework/blitterwidgets/directdrawblitter.cpp \
        framework/fullmodetogglers/gditoggler.cpp \
        framework/SDL_Joystick/src/win32/SDL_mmjoystick.c
    HEADERS += framework/gdisettings.h \
        framework/blitterwidgets/direct3dblitter.h \
        framework/audioengines/directsoundengine.h \
        framework/blitterwidgets/directdrawblitter.h \
        framework/fullmodetogglers/gditoggler.h
    LIBS += -lwinmm \
        -lddraw \
        -ldxguid \
        -ldsound
}
else { 
    SOURCES += framework/addaudioengines.cpp \
        framework/addblitterwidgets.cpp \
        framework/getfullmodetoggler.cpp \
        framework/blitterwidget.cpp \
        framework/audioengines/aoengine.cpp
    SOURCES += framework/SDL_Joystick/src/dummy/SDL_sysjoystick.c
    HEADERS += framework/audioengines/aoengine.h
    CONFIG += link_pkgconfig
    PKGCONFIG += ao
}
