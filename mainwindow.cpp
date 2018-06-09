#include "mainwindow.h"
#include "ui_mainwindow.h"

//将QImage转化为Mat
inline cv::Mat QImageToCvMat( const QImage &inImage, bool inCloneImageData = true )
{
    switch ( inImage.format() )
    {
    // 8-bit, 4 channel
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
    {
        cv::Mat  mat( inImage.height(), inImage.width(),
                      CV_8UC4,
                      const_cast<uchar*>(inImage.bits()),
                      static_cast<size_t>(inImage.bytesPerLine())
                      );

        return (inCloneImageData ? mat.clone() : mat);
    }

    // 8-bit, 3 channel
    case QImage::Format_RGB32:
    case QImage::Format_RGB888:
    {
        if ( !inCloneImageData )
        {
            qWarning() << "CVS::QImageToCvMat() - Conversion requires cloning because we use a temporary QImage";
        }

        QImage   swapped = inImage;

        if ( inImage.format() == QImage::Format_RGB32 )
        {
            swapped = swapped.convertToFormat( QImage::Format_RGB888 );
        }

        swapped = swapped.rgbSwapped();

        return cv::Mat( swapped.height(), swapped.width(),
                        CV_8UC3,
                        const_cast<uchar*>(swapped.bits()),
                        static_cast<size_t>(swapped.bytesPerLine())
                        ).clone();
    }

    // 8-bit, 1 channel
    case QImage::Format_Indexed8:
    {
        cv::Mat  mat( inImage.height(), inImage.width(),
                      CV_8UC1,
                      const_cast<uchar*>(inImage.bits()),
                      static_cast<size_t>(inImage.bytesPerLine())
                      );

        return (inCloneImageData ? mat.clone() : mat);
    }

    default:
        qWarning() << "CVS::QImageToCvMat() - QImage format not handled in switch:" << inImage.format();
        break;
    }

    return cv::Mat();
}

//将QPixmap转化为Mat
inline cv::Mat QPixmapToCvMat( const QPixmap &inPixmap, bool inCloneImageData = true )
{
    return QImageToCvMat( inPixmap.toImage(), inCloneImageData );
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    frameSender = new QUdpSocket();

    grabscrean = new GrabScrean();
    thread = new QThread(this);
    grabscrean->moveToThread(thread);
    connect( thread, SIGNAL(finished()),grabscrean,SLOT(deleteLater()));
    connect(grabscrean, SIGNAL(snSendScreenPixmap(QPixmap)) , this , SLOT(slReceiveScreenPixmap(QPixmap)));

    connect(this, SIGNAL(snStartGrabScrean()) , grabscrean , SLOT(slStartGrabScrean()));

    saveFrames = false;

    thread->start();

    hevcEncodeInit();

    //with slow encode mod
    //hevcFirstFrameEncode();

     emit snStartGrabScrean();
}

MainWindow::~MainWindow()
{

    fclose(fp_out);
    avcodec_close(pCodecCtx);
    av_free(pCodecCtx);
    av_freep(&pFrame->data[0]);
    av_frame_free(&pFrame);

    thread->quit();
    thread->wait();

    delete ui;
}

int MainWindow::hevcEncodeInit()
{
    avcodec_register_all();
    pCodec = avcodec_find_encoder(codec_id);
    if (!pCodec) {
        qDebug()<<"Codec not found\n";
        return -1;
    }
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx) {
        qDebug()<<"Could not allocate video codec context\n";
        return -1;
    }
    pCodecCtx->bit_rate = 2*500000;
    pCodecCtx->width = in_w;
    pCodecCtx->height = in_h;
    pCodecCtx->time_base.num=1;
    pCodecCtx->time_base.den=25;
    pCodecCtx->gop_size = 10;
    pCodecCtx->max_b_frames = 1;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    av_opt_set(pCodecCtx->priv_data, "preset", "superfast", 0);
    av_opt_set(pCodecCtx->priv_data, "tune", "zerolatency", 0);

    if (codec_id == AV_CODEC_ID_H264)
        av_opt_set(pCodecCtx->priv_data, "preset", "slow", 0);

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open codec\n");
        return -1;
    }

    pFrame = av_frame_alloc();
    if (!pFrame) {
        printf("Could not allocate video frame\n");
        return -1;
    }
    pFrame->format = pCodecCtx->pix_fmt;
    pFrame->width  = pCodecCtx->width;
    pFrame->height = pCodecCtx->height;

    ret = av_image_alloc(pFrame->data, pFrame->linesize, pCodecCtx->width, pCodecCtx->height,
                         pCodecCtx->pix_fmt, 16);
    if (ret < 0) {
        printf("Could not allocate raw picture buffer\n");
        return -1;
    }

    y_size = pCodecCtx->width * pCodecCtx->height;

    filename_out=QString("save.hevc");

    fp_out = fopen(filename_out.toStdString().c_str(), "wb");
    if (!fp_out) {
        printf("Could not open %s\n", filename_out);
    }

    i=1;

    return 1;
}

void MainWindow::hevcFirstFrameEncode()
{

    pixmap = QPixmap::grabWindow(QApplication::desktop()->winId(),1366,768);
    img = QPixmapToCvMat(pixmap);

    Mat temp;
    cv::resize(img,temp,Size(SIZE_W,SIZE_H),0,0,INTER_LINEAR);

    cvtColor(temp, img_yuv, CV_RGB2YUV_I420);

    got_output = 0;
    while(!got_output)
    {
        av_init_packet(&pkt);
        pkt.data = NULL;    // packet data will be allocated by the encoder
        pkt.size = 0;

        for(int k = 0; k < y_size; k++ )
        {
            int quotient  = k / pCodecCtx->width;
            int remainder  = k % pCodecCtx->width;
             *(pFrame->data[0] + pFrame->linesize[0]*quotient +remainder)=
                    *(img_yuv.data + k);
        }

        for(int k = 0; k < y_size/4; k++ )
        {
            int quotient  = k / (pCodecCtx->width/2);
            int remainder  = k % (pCodecCtx->width/2);
             *(pFrame->data[1] + pFrame->linesize[1]*quotient +remainder)=
                    *(img_yuv.data + y_size + k);
        }

        for(int k = 0; k < y_size/4; k++ )
        {
            int quotient  = k / (pCodecCtx->width/2);
            int remainder  =  k % (pCodecCtx->width/2);
             *(pFrame->data[2] + pFrame->linesize[2]*quotient +remainder)=
                    *(img_yuv.data + y_size + y_size/4 + k);
        }

        pFrame->pts = 0;
        /* encode the image */
        ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_output);
        if (ret < 0) {
            printf("Error encoding frame\n");
        }
    }

    {
        framecnt++;

        if(saveFrames == true)
        {
            fwrite(pkt.data, 1, pkt.size, fp_out);
        }

        int quotient = pkt.size/60000;
        int remainder = pkt.size%60000;

        for(int i = 0; i < quotient; i++)
        {
            QByteArray datagram;
            datagram.resize(60000);
            for(int j= 0;j<60000;j++)
            {
                datagram[j] = pkt.data[j + 60000*i];
            }
            frameSender->writeDatagram(datagram, QHostAddress("192.168.1.141"),6652);
        }

        QByteArray datagram;
        datagram.resize(remainder);
        for(int i= 0;i<remainder;i++)
        {
            datagram[i] = pkt.data[i + 60000*quotient];
        }
        frameSender->writeDatagram(datagram, QHostAddress("192.168.1.141"),6652);


        av_free_packet(&pkt);
    }
}

void MainWindow::hevcEncode()
{
    av_init_packet(&pkt);
    pkt.data = NULL;    // packet data will be allocated by the encoder
    pkt.size = 0;

    //H
    for(int k = 0; k < y_size; k++ )
    {
        int quotient  = k / pCodecCtx->width;
        int remainder  = k % pCodecCtx->width;
         *(pFrame->data[0] + pFrame->linesize[0]*quotient +remainder)=
                *(img_yuv.data + k);
    }

    //U
    for(int k = 0; k < y_size/4; k++ )
    {
        int quotient  = k / (pCodecCtx->width/2);
        int remainder  = k % (pCodecCtx->width/2);
         *(pFrame->data[1] + pFrame->linesize[1]*quotient +remainder)=
                *(img_yuv.data + y_size + k);
    }

    //V
    for(int k = 0; k < y_size/4; k++ )
    {
        int quotient  = k / (pCodecCtx->width/2);
        int remainder  =  k % (pCodecCtx->width/2);
         *(pFrame->data[2] + pFrame->linesize[2]*quotient +remainder)=
                *(img_yuv.data + y_size + y_size/4 + k);
    }

    pFrame->pts = i;
    /* encode the image */
    ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_output);
    if (ret < 0) {
        printf("Error encoding frame\n");
    }


    if (got_output) {
        //framecnt++;

        if(saveFrames == true)
        {
            fwrite(pkt.data, 1, pkt.size, fp_out);
        }

        int quotient = pkt.size/60000;
        int remainder = pkt.size%60000;

        for(int i = 0; i < quotient; i++)
        {
            QByteArray datagram;
            datagram.resize(60000);
            for(int j= 0;j<60000;j++)
            {
                datagram[j] = pkt.data[j + 60000*i];
            }
            frameSender->writeDatagram(datagram, QHostAddress("192.168.1.141"),6652);
        }

        QByteArray datagram;
        datagram.resize(remainder);
        for(int i= 0;i<remainder;i++)
        {
            datagram[i] = pkt.data[i + 60000*quotient];
        }
        frameSender->writeDatagram(datagram, QHostAddress("192.168.1.141"),6652);

        av_free_packet(&pkt);
    }
    i++;
}

void MainWindow::slReceiveScreenPixmap(QPixmap qpixmap)
{
    img = QPixmapToCvMat(qpixmap);
    cv::Mat temp;
    cv::resize(img,temp,cv::Size(SIZE_W,SIZE_H),0,0,INTER_LINEAR);

    cv::cvtColor(temp, img_yuv, CV_RGB2YUV_I420);

    hevcEncode();
}
