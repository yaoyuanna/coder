QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

include(./opengl/opengl.pri)

INCLUDEPATH += ./opengl/

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH +=C:/my_Develp_lib/ffmpeg-4.2.2-win64-dev/include\
              C:/my_Develp_lib/SDL2-2.0.10/include\
              C:/my_Develp_lib/OpenCV-MinGW-Build-OpenCV-3.4.8-x64/include/opencv2\
              C:/my_Develp_lib/OpenCV-MinGW-Build-OpenCV-3.4.8-x64/include

LIBS += C:/my_Develp_lib/ffmpeg-4.2.2-win64-dev/lib/avcodec.lib\
        C:/my_Develp_lib/ffmpeg-4.2.2-win64-dev/lib/avdevice.lib\
        C:/my_Develp_lib/ffmpeg-4.2.2-win64-dev/lib/avfilter.lib\
        C:/my_Develp_lib/ffmpeg-4.2.2-win64-dev/lib/avformat.lib\
        C:/my_Develp_lib/ffmpeg-4.2.2-win64-dev/lib/avutil.lib\
        C:/my_Develp_lib/ffmpeg-4.2.2-win64-dev/lib/postproc.lib\
        C:/my_Develp_lib/ffmpeg-4.2.2-win64-dev/lib/swresample.lib\
        C:/my_Develp_lib/ffmpeg-4.2.2-win64-dev/lib/swscale.lib\
        C:/my_Develp_lib/SDL2-2.0.10/lib/x64/SDL2.lib\
        C:/my_Develp_lib/OpenCV-MinGW-Build-OpenCV-3.4.8-x64/x64/mingw/lib/libopencv_*.dll.a


SOURCES += \
    main.cpp \
    mainwindow.cpp \
    picinpic_read.cpp \
    picinpic_window.cpp \
    savevideofilethread.cpp
HEADERS += \
    common.h \
    mainwindow.h \
    picinpic_read.h \
    picinpic_window.h \
    savevideofilethread.h

FORMS += \
    mainwindow.ui \
    picinpic_window.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
