#include "imagepacker.h"

//auto-cropping algorithm
void ImagePacker::crop(QList<packedImage*> *images)
{
    int i, j, w, h, x, y;
    QRgb pix;
    bool t;
    if(trim) for(i = 0; i < images->size(); i++)
    {
        pix = images->at(i)->img.pixel(0,0);
        t = true;
        //top trimming
        for(y = 0; y < images->at(i)->img.height(); y++)
        {
            for(j = 0; j < images->at(i)->img.width(); j++)
                if(images->at(i)->img.pixel(j,y) != pix) {t = false; break;}
            if(!t) break;
        }
        t = true;
        //left
        for(x = 0; x < images->at(i)->img.width(); x++){
            for(j = y; j < images->at(i)->img.height(); j++)
                if(images->at(i)->img.pixel(x,j) != pix) {t = false; break;}
            if(!t) break;
        }
        t = true;
        //right
        for(w = images->at(i)->img.width(); w > 0; w--){
            for(j = y; j < images->at(i)->img.height(); j++)
                if(images->at(i)->img.pixel(w-1,j) != pix) {t = false; break;}
            if(!t) break;
        }
        t = true;
        //else
        {
            //bottom
            for(h = images->at(i)->img.height(); h > 0; h--){
                for(j = x; j < w; j++)
                    if(images->at(i)->img.pixel(j,h-1) != pix) {t = false; break;}
                if(!t) break;
            }
        }
        w = w - x;
        h = h - y;
        if(w < 0) w = 0;
        if(h < 0) h = 0;
        QImage newImg;
        QRect pos(0, 0, images->at(i)->img.width(), images->at(i)->img.height());
        
        if(w > 0) newImg = images->at(i)->img.copy(QRect(x-borderLeft, y-borderTop, w+borderLeft+borderRight, h+borderTop+borderBottom));
        images->operator [](i)->img = newImg;
        images->operator [](i)->crop = QRect(x, y, w, h);
        images->operator [](i)->rc = pos;
    }
}
