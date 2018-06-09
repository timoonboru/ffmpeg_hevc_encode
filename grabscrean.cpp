#include "grabscrean.h"

GrabScrean::GrabScrean(QObject *parent) : QObject(parent)
{
    timer=new QTimer(this); //ÉèÖÃ¶¨Ê±Æ÷
    QObject::connect(timer,SIGNAL(timeout()),this,SLOT(slGetScreenPixmap()));
    timer->start(100);

    tag_start = false;
}

void GrabScrean::slGetScreenPixmap()
{
    if(tag_start == true)
    {
        pixmap=QPixmap::grabWindow(QApplication::desktop()->winId(),0,0,1366,768);
        emit snSendScreenPixmap(pixmap);
    }
}

void GrabScrean::slStartGrabScrean()
{
    tag_start = true;
}

