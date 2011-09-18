#ifndef FONTRENDER_H
#define FONTRENDER_H

#include <QThread>
#include <QImage>
#include <QPainter>
#include <QList>
//#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "imagepacker.h"
class FontRender : public QThread
{
     Q_OBJECT
public:
    FontRender(Ui_MainWindow *ui = 0);
    ~FontRender();
    void run();
    bool done;

signals:
        void renderedImage(const QImage &image);
private:
    QImage texture;
    QList<QImage> glyphs;
    QObject p;
    Ui_MainWindow *ui;
    ImagePacker packer;
};

#endif // FONTRENDER_H
