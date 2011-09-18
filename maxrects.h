#ifndef MAXRECTS_H
#define MAXRECTS_H
#include <QImage>

struct trbl
{
    QPoint t, r, b, l;
};

struct MaxRectsNode
{
    QRect r; //rect
    QImage * i; //image
    trbl b; //border
};
class MaxRects
{
public:
    MaxRects();
    QList<MaxRectsNode> F;
    QList<MaxRectsNode*> FR;
    QPoint insertNode(QImage * img);
    int heuristic;
    bool leftToRight;
};

#endif // MAXRECTS_H
