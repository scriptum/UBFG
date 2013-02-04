#include "maxrects.h"
#include "imagepacker.h"
MaxRects::MaxRects()
{
}

QPoint MaxRects::insertNode(QImage * img)
{
    int i, j;
    int min = 999999999, mini = -1, weight = 0;
    if(img->width() == 0 || img->height() == 0)
        return QPoint(0,0);
    bool leftNeighbor = false, rightNeighbor = false, _leftNeighbor, _rightNeighbor;

    for(i = 0; i < F.size(); i++)
    {
        MaxRectsNode RectNode = F.at(i);
        if(RectNode.r.width() >= img->width() && RectNode.r.height() >= img->height())
        {
            switch(heuristic)
            {
                case ImagePacker::NONE:
                    mini = i;
                    i = F.size();
                    continue;
                case ImagePacker::TL:

                    weight = RectNode.r.y();
                    _leftNeighbor = _rightNeighbor = false;
                    //loop over all rectangles
                    for(int k = 0; k < R.size(); k++)
                    {
                        QRect RectIter = R.at(k);
                        //pffff whait is this? But it works!
                        if(qAbs(RectIter.y() +
                                RectIter.height() / 2 -
                                RectNode.r.y() -
                                RectNode.r.height() / 2) <
                                qMax(RectIter.height(), RectNode.r.height()) / 2)
                        {
                            if(RectIter.x() + RectIter.width() == RectNode.r.x())
                            {
                                weight -= 2;
                                _leftNeighbor = true;
                            }
                            if(RectIter.x() == RectNode.r.x() + RectNode.r.width())
                            {
                                weight -= 2;
                                _rightNeighbor = true;
                            }
                        }
                    }
                    break;
                case ImagePacker::BAF:
                    weight = F.at(i).r.width() * F.at(i).r.height();
                    break;
                case ImagePacker::BSSF:
                    weight = qMin(F.at(i).r.width() - img->width(), F.at(i).r.height() - img->height());
                    break;
                case ImagePacker::BLSF:
                    weight = qMax(F.at(i).r.width() - img->width(), F.at(i).r.height() - img->height());
                    break;
                case ImagePacker::MINW:
                    weight = F.at(i).r.width();
                    break;
                case ImagePacker::MINH:
                    weight = F.at(i).r.height();
            }
            if(weight < min)
            {
                min = weight;
                mini = i;
                leftNeighbor = _leftNeighbor;
                rightNeighbor = _rightNeighbor;
            }
        }
    }
    if(mini >= 0)
    {
        i = mini;
        MaxRectsNode n0;
        QRect buf(F.at(i).r.x(), F.at(i).r.y(), img->width(), img->height());
        if(heuristic == ImagePacker::TL) {
            if(!leftNeighbor && F.at(i).r.x() != 0 && F.at(i).r.width() + F.at(i).r.x() == w)
                buf = QRect(w - img->width(), F.at(i).r.y(), img->width(), img->height());
            if(!leftNeighbor && rightNeighbor)
                buf = QRect(F.at(i).r.x() + F.at(i).r.width() - img->width(), F.at(i).r.y(), img->width(), img->height());
        }
        n0.r = buf;
        R << buf;
        n0.i = img;
        if(F.at(i).r.width() > img->width())
        {
            MaxRectsNode n;
            n.r = QRect(F.at(i).r.x()+(buf.x()==F.at(i).r.x()?img->width():0), F.at(i).r.y(), F.at(i).r.width() - img->width(), F.at(i).r.height());
            n.i = NULL;
            F<<n;
        }
        if(F.at(i).r.height() > img->height())
        {
            MaxRectsNode n;
            n.r = QRect(F.at(i).r.x(), F.at(i).r.y()+img->height(), F.at(i).r.width(), F.at(i).r.height() - img->height());
            n.i = NULL;
            F<<n;
        }
        
        F.removeAt(i);
        //intersect
        for(i = 0; i < F.size(); i++)
        {
            if(F.at(i).r.intersects(n0.r))
            {
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
        return QPoint(n0.r.x(), n0.r.y());
    }
    return QPoint(999999, 999999);
}
