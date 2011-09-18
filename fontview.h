#ifndef FONTVIEW_H
#define FONTVIEW_H

#include <QWidget>
#include <QPainter>
#include <QPixmap>
#include <QImage>
class FontView : public QWidget
{
Q_OBJECT
public:
    FontView(QWidget *parent = 0);
protected:
    void paintEvent(QPaintEvent *event);
private:
    QPixmap texture;
private slots:
    void updatePixmap(const QImage &image);
};

#endif // FONTVIEW_H
