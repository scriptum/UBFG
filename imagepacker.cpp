#include "imagepacker.h"
#include "guillotine.h"
#include "maxrects.h"
ImagePacker::ImagePacker()
{
}

//pack images, return list of positions
QList<QPoint> ImagePacker::pack(QList<packedImage> *im, int packMethod, int heur, uint w, uint h)
{
    int i, j;
    QList<packedImage*> images;
    for(i = 0; i < im->size(); i++)
        images << &im->operator [](i);
    crop(&images);
    sort(&images);

    QList<QPoint> out;
    Guillotine *head = new Guillotine;
    Guillotine *n, *p;
    missingChars = 0;
    area = 0;
    mergedChars = 0;
    neededArea = 0;
    switch(packMethod)
    {
    case GUILLOTINE:

        //qDebug("%d %d %d %d", n->rc.x(), n->rc.y(), n->rc.width(), n->rc.height());
        head->rc = QRect(0, 0, w, h);
        head->head = head;
        head->heuristicMethod = heur;
        //qDebug("%d %d %d %d", head->rc.x(), head->rc.y(), head->rc.width(), head->rc.height());

        for(i = 0; i < images.size(); i++)
        {
            n = head->insertNode(&images.operator [](i)->img);
            //qDebug("%d %d %d %d", n->rc.x(), n->rc.y(), n->rc.width(), n->rc.height());
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
        }
        //p = head->child[0]->child[1];
        //qDebug("%d", p);
        head->delGuillotine();
        //delete p;
        //qDebug("%d", p->child);
        break;
    case MAXRECTS:
        MaxRects rects;
        MaxRectsNode mrn;
        mrn.r = QRect(0, 0, w, h);
        mrn.i = NULL;
        rects.F << mrn;
        rects.heuristic = heur;
        rects.leftToRight = ltr;

        //qDebug("%d%", rects.F.at(0).r.width());
        QPoint pt;
        bool t;
        for(i = 0; i < images.size(); i++)
        {

            //qDebug("%d", images.operator [](i)->rc.width());
            t = false;
            for(j = 0; j < out.size(); j++)
            {
                if(images.at(j)->img.operator ==(images.at(i)->img))
                {
                    pt = out.at(j);
                    t = true;
                    mergedChars++;
                    break;
                }
            }

            //qDebug("%d", images.operator [](i)->rc.width());
            if(!t)
                pt = rects.insertNode(&images.operator [](i)->img);
            if(pt != QPoint(999999,999999))
            {
                if(!t)
                    area += images.at(i)->img.width() * images.at(i)->img.height();
            }
            else
                missingChars++;

            //qDebug("%d", images.operator [](i)->rc.width());
            if(!t)
                neededArea += images.at(i)->img.width() * images.at(i)->img.height();
            out << pt;
            images.operator [](i)->rc = QRect(pt.x(), pt.y(), images.at(i)->rc.width(), images.at(i)->rc.height());


        }
        //qDebug("%d", rects.F.size());
        break;
    }
    return out;
}
