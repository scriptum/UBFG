#include "fontview.h"

FontView::FontView(QWidget *parent)
{
}


void FontView::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::magenta);
    painter.drawPixmap(0, 0, texture);
}


void FontView::updatePixmap(const QImage &image)
{
    texture = QPixmap::fromImage(image);
    //image.~QImage();
    this->setMinimumSize(texture.width(), texture.height());
    update();

    //qDebug("%d %d", texture.width(), texture.height());
}
