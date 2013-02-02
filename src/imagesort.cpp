#include "imagepacker.h"

bool ImageCompareByHeight(const packedImage *i1, const packedImage *i2)
{
    return (i1->img.height() << 10) + i1->img.width() > (i2->img.height() << 10) +i2->img.width();
}
bool ImageCompareByWidth(const packedImage *i1, const packedImage *i2)
{
    return (i1->img.width() << 10) + i1->img.height() > (i2->img.width() << 10) + i2->img.height();
}
bool ImageCompareByArea(const packedImage *i1, const packedImage *i2)
{
    return i1->img.height() * i1->img.width() > i2->img.height() * i2->img.width();
}

void ImagePacker::sort(QList<packedImage*> *images)
{
    switch(sortOrder)
    {
        case 1:
            qSort(images->begin(), images->end(), ImageCompareByWidth);
            break;
        case 2:
            qSort(images->begin(), images->end(), ImageCompareByHeight);
            break;
        case 3:
            qSort(images->begin(), images->end(), ImageCompareByArea);
            break;
    }
}
