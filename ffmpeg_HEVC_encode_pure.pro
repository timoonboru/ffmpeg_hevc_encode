#-------------------------------------------------
#
# Project created by QtCreator 2018-06-09T08:56:42
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ffmpeg_HEVC_encode_pure
TEMPLATE = app

INCLUDEPATH += D:/FFMPEG/ffmpeg_4.0_64/dev/include \
                D:/opencv/build/include \
                D:/opencv/build/include/opencv \
                D:/opencv/build/include/opencv2

LIBS += D:/opencv/build/x64/vc12/lib/opencv_core2410d.lib \
D:/opencv/build/x64/vc12/lib/opencv_highgui2410d.lib \
D:/opencv/build/x64/vc12/lib/opencv_imgproc2410d.lib \
D:/FFMPEG/ffmpeg_4.0_64/dev/lib/avcodec.lib \
D:/FFMPEG/ffmpeg_4.0_64/dev/lib/avformat.lib \
D:/FFMPEG/ffmpeg_4.0_64/dev/lib/swscale.lib \
D:/FFMPEG/ffmpeg_4.0_64/dev/lib/avutil.lib \
D:/FFMPEG/ffmpeg_4.0_64/dev/lib/avdevice.lib



SOURCES += main.cpp\
        mainwindow.cpp \
    grabscrean.cpp

HEADERS  += mainwindow.h \
    grabscrean.h

FORMS    += mainwindow.ui
