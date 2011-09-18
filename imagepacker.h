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
    int textureId;
    QChar ch;
};

class ImagePacker : public QObject
{
public:
    ImagePacker();
    QList<QPoint> pack(QList<packedImage> *images, int packMethod, int heuristic, uint w = 128, uint h = 128);
    void crop(QList<packedImage*> *images);
    void sort(QList<packedImage*> *images);
    int compare;
    int area;
    int missingChars;
    int mergedChars;
    bool ltr;
    bool border;
    int neededArea;
    int sortOrder;
    enum {GUILLOTINE, MAXRECTS}; //method
    enum {NONE, TL, BAF, BSSF, BLSF, MINW, MINH}; //heuristic
};


#endif // IMAGEPACKER_H
