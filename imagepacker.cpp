#include "imagepacker.h"
#include "guillotine.h"
#include "maxrects.h"
ImagePacker::ImagePacker()
{
}

//pack images, return list of positions
QList<QPoint> ImagePacker::pack(QList<packedImage> *im, int packMethod, int heur, uint w, uint h)
{
    int i, j, x, y;
    QList<packedImage*> images;
    for(i = 0; i < im->size(); i++)
        images << &im->operator [](i);
    crop(&images);
    sort(&images);

    QList<QPoint> out;
    Guillotine *head = new Guillotine;
    Guillotine *n;
    missingChars = 0;
    area = 0;
    mergedChars = 0;
    neededArea = 0;
    switch(packMethod)
    {
    case GUILLOTINE:

        head->rc = QRect(0, 0, w, h);
        head->head = head;
        head->heuristicMethod = heur;
        head->packer = this;

        for(i = 0; i < images.size(); i++)
        {
            n = head->insertNode(&images.operator [](i)->img);
            if(n)
            {
                if(!head->duplicate)
                    area += n->image->width() * n->image->height();
                else
                    mergedChars++;
                out << QPoint(n->rc.x(), n->rc.y());
                images.operator [](i)->rc = QRect(n->rc.x(), n->rc.y(), images.at(i)->rc.width(), images.at(i)->rc.height());
            }
            else
            {
                out << QPoint(99999, 99999);
                images.operator [](i)->rc = QRect(99999, 00000, images.at(i)->rc.width(), images.at(i)->rc.height());
                missingChars++;
            }
            if(!head->duplicate)
                neededArea += images.at(i)->img.width() * images.at(i)->img.height();
        }
        head->delGuillotine();
        break;
    case MAXRECTS:
        MaxRects rects;
        MaxRectsNode mrn;
        mrn.r = QRect(0, 0, w, h);
        mrn.i = NULL;
        rects.F << mrn;
        rects.heuristic = heur;
        rects.leftToRight = ltr;
        rects.w = w;
        rects.h = h;
        QPoint pt;
        bool t;
        for(i = 0; i < images.size(); i++)
        {
            t = false;
            for(j = 0; j < out.size(); j++)
            {
                if(compareImages(&images.at(j)->img, &images.at(i)->img, &x, &y))
                {
                    pt = out.at(j)+QPoint(x, y);
                    t = true;
                    mergedChars++;
                    break;
                }
            }
            if(!t)
                pt = rects.insertNode(&images.operator [](i)->img);
            if(pt != QPoint(999999,999999))
            {
                if(!t)
                    area += images.at(i)->img.width() * images.at(i)->img.height();
            }
            else
                missingChars++;
            if(!t)
                neededArea += images.at(i)->img.width() * images.at(i)->img.height();
            out << pt;
            images.operator [](i)->rc = QRect(pt.x(), pt.y(), images.at(i)->rc.width(), images.at(i)->rc.height());


        }
        break;
    }
    return out;
}

bool ImagePacker::compareImages(QImage* img1, QImage* img2, int* ii, int *jj)
{
    if(!merge) return false;
    if(!mergeBF) return img1->operator ==(*img2);
    int i1w = img1->width();
    int i1h = img1->height();
    int i2w = img2->width();
    int i2h = img2->height();
    int i, j, x, y;
    bool t;
    if(i1w >= i2w && i1h >= i2h)
    {
        for(i = 0; i <= i1w - i2w; i++)
        {
            for (j = 0; j <= i1h - i2h; j++)
            {
                t = true;
                for (y = 0; y < i2h; y++)
                {
                    for (x = 0; x < i2w; x++)
                    {
                        if(img1->pixel(x+i, y+j) != img2->pixel(x,y))
                        {
                            t = false;
                            break;
                        }
                    }
                    if(t == false) break;
                }
                if(t)
                {
                    *ii = i;
                    *jj = j;
                    return true;
                }
            }
        }
    }
    return false;
}
