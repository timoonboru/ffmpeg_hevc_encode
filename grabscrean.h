#ifndef GRABSCREAN_H
#define GRABSCREAN_H


#include <QMainWindow>
#include <QDebug>
#include <QThread>
#include <QDesktopWidget>
#include <QPixmap>
#include <QApplication>
#include <QTimer>
#include <time.h>

class GrabScrean : public QObject
{
    Q_OBJECT
public:
    explicit GrabScrean(QObject *parent = 0);

signals:
    void snSendScreenPixmap(QPixmap pixmap);

public slots:
    void slGetScreenPixmap();
    void slStartGrabScrean();

private:
    QPixmap pixmap;
    QTimer *timer;

    bool tag_start;
};

#endif // GRABSCREAN_H
