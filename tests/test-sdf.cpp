#include <QtCore>
#include <QImage>
#include <QTime>
#include <QDebug>
#include <math.h>
#include "sdf.h"


double getPSNR(QImage &image1, QImage &image2)
{
    if (image1.size() != image2.size())
    {
        qDebug() << "Error: image sizes do not match!";
        qDebug() << "Actual:" << image1.size() << "Expected:" << image2.size();
        return -1.0;
    }
    int w = image1.width();
    int h = image1.height();

    long nSqrSum = 0;
    for(int j = 0; j < h; j++)
    {
        for(int i = 0; i < w; i++)
        {
            QRgb p1 = image1.pixel(i, j);
            QRgb p2 = image2.pixel(i, j);
            int tmp1 = (int)(0.3 * qRed(p1) + 0.59 * qGreen(p1) + 0.11 * qBlue(p1) + 0.5);
            int tmp2 = (int)(0.3 * qRed(p2) + 0.59 * qGreen(p2) + 0.11 * qBlue(p2) + 0.5);
            nSqrSum += (tmp1 - tmp2) * (tmp1 - tmp2);
        }
    }
    double dPSNR;
    double MSE = (double)nSqrSum/w/h;
    if (MSE > 0)
        dPSNR = 10. * log10(255. * 255. / MSE);
    else
        dPSNR = 100.;
    return dPSNR;
}
int main()
{
    QTime myTimer;
    QImage bigimg("test-big.png");
    myTimer.start();
    dfcalculate(bigimg);
    qDebug() << "8SED (BIG):\t" << myTimer.elapsed() << "ms";
    QImage img("test.png");
    myTimer.start();
    QImage res_8sed = dfcalculate(img);
    qDebug() << "8SED:\t\t" << myTimer.elapsed() << "ms";
    //res_8sed.save("result-8sed.png");
    myTimer.start();
    QImage res_brut = dfcalculate_bruteforce(img);
    qDebug() << "Brute-force:\t" << myTimer.elapsed() << "ms";
    //res_brut.save("result-bruteforce.png");
    double psnr = getPSNR(res_8sed, res_brut);
    qDebug() << "PSNR:\t\t"<< psnr;
    if(psnr < 70.)
        qDebug() << "FAIL!";
}
