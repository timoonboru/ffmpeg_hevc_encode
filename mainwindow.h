#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QThread>
#include <QDesktopWidget>
#include <QUdpSocket>
#include <QEventLoop>
#include <QDateTime>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

#include <grabscrean.h>

using namespace std;
using namespace cv;


#define __STDC_CONSTANT_MACROS
//#ifdef __cplusplus
extern "C" {
//#endif
    #include <libavutil/avutil.h>
    #include <libavutil/imgutils.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavdevice/avdevice.h>
    #include <libavcodec/version.h>
    #include <libavutil/time.h>
    #include <libavutil/mathematics.h>
//#ifdef __cplusplus
}
//#endif

#define SIZE_W 800
#define SIZE_H 500

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    int hevcEncodeInit();
    void hevcEncode();
    void hevcFirstFrameEncode();

private slots:
    void slReceiveScreenPixmap(QPixmap qpixmax);
signals:
    void snStartGrabScrean();

private:
    Ui::MainWindow *ui;

    AVCodec *pCodec;
    AVCodecContext *pCodecCtx= NULL;
    int i, ret, got_output;
    FILE *fp_out;
    AVFrame *pFrame;
    AVPacket pkt;
    int y_size;
    int framecnt=0;

    QString filename_out;
    bool saveFrames;

    int in_w = SIZE_W, in_h = SIZE_H;
    AVCodecID codec_id=AV_CODEC_ID_HEVC;

    cv::Mat img;
    cv::Mat img_yuv;
    QPixmap pixmap;

    GrabScrean *grabscrean;
    QThread *thread;

    QUdpSocket *frameSender;
};

#endif // MAINWINDOW_H
