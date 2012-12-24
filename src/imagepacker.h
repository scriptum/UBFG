#ifndef IMAGEPACKER_H
#define IMAGEPACKER_H

#include <QObject>
#include <QImage>

struct packedImage
{
    QImage img;
    QRect rc;
    QRect crop;
    bool border;
    bool merged;
    int textureId;
    QChar ch;
};

class ImagePacker : public QObject
{
public:
    ImagePacker();
    bool compareImages(QImage* img1, QImage* img2, int* i, int *j);
    QList<QPoint> pack(QList<packedImage> *images, int heuristic, uint w = 128, uint h = 128);
    void crop(QList<packedImage*> *images);
    void sort(QList<packedImage*> *images);
    int compare;
    int area;
    int missingChars;
    int mergedChars;
    bool ltr, trim, merge, mergeBF;
    bool bruteForce;
    unsigned int borderTop, borderBottom, borderLeft, borderRight;
    int neededArea;
    int sortOrder;
    enum {GUILLOTINE, MAXRECTS}; //method
    enum {NONE, TL, BAF, BSSF, BLSF, MINW, MINH}; //heuristic
};




#endif // IMAGEPACKER_H
