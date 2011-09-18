#include "maxrects.h"
#include "imagepacker.h"
MaxRects::MaxRects()
{
}

QPoint MaxRects::insertNode(QImage * img)
{
    int i, j;
    int min = 999999999, mini = -1, m;
    if(img->width() == 0 || img->height() == 0)
        return QPoint(0,0);
    static int nextx = 0, nexty = 0, prevx;
    static bool ltr = true;

    for(i = 0; i < F.size(); i++)
    {
        if(F.at(i).r.width() >= img->width() && F.at(i).r.height() >= img->height())
        {
            switch(heuristic)
            {
                case ImagePacker::NONE:
                    mini = i;
                    i = F.size();
                    continue;
                case ImagePacker::TL:
                    m = F.at(i).r.y() + F.at(i).r.x()/10;
                    break;
                case ImagePacker::BAF:
                    m = F.at(i).r.width() * F.at(i).r.height();
                    break;
                case ImagePacker::BSSF:
                    m = qMin(F.at(i).r.width() - img->width(), F.at(i).r.height() - img->height());
                    break;
                case ImagePacker::BLSF:
                    m = qMax(F.at(i).r.width() - img->width(), F.at(i).r.height() - img->height());
                    break;
                case ImagePacker::MINW:
                    m = F.at(i).r.width();
                    break;
                case ImagePacker::MINH:
                    m = F.at(i).r.height();
            }
            if(leftToRight)
            {
                if(F.at(i).r.x() == nextx && ltr)
                    m -= 100;
                if(F.at(i).r.x() == prevx && !ltr)
                    m -= 10000;
            }
            //if(F.at(i).r.y() == nexty)
            //    m -= 1000;
            //if(F.at(i).r.width() == img->width() && F.at(i).r.height() == img->height()) m-= 100000;
            if(m < min)
            {
                min = m;
                mini = i;
            }
            //qDebug("# %d", m);
        }
    }
    //if(min < -5000)
    //qDebug(" -- %d %d %d", i, F.at(i).r.width(), F.at(i).r.height());
    if(mini>=0)
    {
        i = mini;
        //qDebug("%d %d", F.at(i).r.width(), F.at(i).r.height());
        MaxRectsNode n0;
        if(!ltr)
            n0.r = QRect(F.at(i).r.x() + F.at(i).r.width() - img->width(), F.at(i).r.y(), img->width(), img->height());
        else
        {
            n0.r = QRect(F.at(i).r.x(), F.at(i).r.y(), img->width(), img->height());
        }
        n0.i = img;
        nextx = n0.r.x() + n0.r.width();
        nexty = F.at(i).r.y() + img->height();
        prevx = n0.r.x();
        if(F.at(i).r.width() > img->width())
        {
            MaxRectsNode n;
            if(!ltr)
                n.r = QRect(F.at(i).r.x(), F.at(i).r.y(), F.at(i).r.width() - img->width(), F.at(i).r.height());
            else
                n.r = QRect(F.at(i).r.x()+img->width(), F.at(i).r.y(), F.at(i).r.width() - img->width(), F.at(i).r.height());
            n.i = NULL;
            F<<n;
            //qDebug("%d %d", F.at(i).b.t.x(), F.at(i).b.t.y());
            //qDebug(" (1)");
        }
        if(F.at(i).r.height() > img->height())
        {
            MaxRectsNode n;
            n.r = QRect(F.at(i).r.x(), F.at(i).r.y()+img->height(), F.at(i).r.width(), F.at(i).r.height() - img->height());
            n.i = NULL;
            F<<n;
            //qDebug(" (2)");
        }
        F.removeAt(i);
        //qDebug("%d", F.size());
        //intersect
        for(i = 0; i < F.size(); i++)
        {
            //qDebug(" ++ %d - %d %d %d %d", i, F.at(i).r.x(), F.at(i).r.y(), F.at(i).r.width(), F.at(i).r.height());
            if(F.at(i).r.intersects(n0.r))
            {
                //qDebug("  %d - %d %d %d %d || %d %d %d %d", i, F.at(i).r.x(), F.at(i).r.y(), F.at(i).r.width(), F.at(i).r.height(), n0.r.x(), n0.r.y(),n0.r.width(),n0.r.height());
                //qDebug("!!!!");
                if(n0.r.x() + n0.r.width() < F.at(i).r.x() + F.at(i).r.width())
                {
                    MaxRectsNode n;
                    n.r = QRect(n0.r.width() + n0.r.x(), F.at(i).r.y(),
                                F.at(i).r.width() + F.at(i).r.x() - n0.r.width() - n0.r.x(), F.at(i).r.height());
                    n.i = NULL;
                    F << n;
                }
                if(n0.r.y() + n0.r.height() < F.at(i).r.y() + F.at(i).r.height())
                {
                    MaxRectsNode n;
                    n.r = QRect(F.at(i).r.x(), n0.r.height() + n0.r.y(),
                                F.at(i).r.width(), F.at(i).r.height() + F.at(i).r.y()- n0.r.height() - n0.r.y());
                    n.i = NULL;
                    F << n;
                }
                if(n0.r.x() > F.at(i).r.x())
                {
                    MaxRectsNode n;
                    n.r = QRect(F.at(i).r.x(), F.at(i).r.y(), n0.r.x() - F.at(i).r.x(), F.at(i).r.height());
                    n.i = NULL;
                    F << n;
                }
                if(n0.r.y() > F.at(i).r.y())
                {
                    MaxRectsNode n;
                    n.r = QRect(F.at(i).r.x(), F.at(i).r.y(), F.at(i).r.width(), n0.r.y() - F.at(i).r.y());
                    n.i = NULL;
                    F << n;
                }
                F.removeAt(i);
                i--;
            }

        }
        //qDebug(" (3)");
        for(i = 0; i < F.size(); i++)
        {
            for(j = i + 1; j < F.size(); j++)
            {
                if(i!=j  && F.at(i).r.contains(F.at(j).r))
                {
                    F.removeAt(j);
                    j--;
                }
            }
        }
        //for(i = 0; i < F.size(); i++)qDebug(" -- %d - %d %d %d %d", i, F.at(i).r.x(), F.at(i).r.y(), F.at(i).r.width(), F.at(i).r.height());
        return QPoint(n0.r.x(), n0.r.y());
    }
    return QPoint(999999, 999999);
}
