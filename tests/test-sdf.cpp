#include <QtCore>
#include "../src/sdf.h"

qreal getPSNR(QImage &image1, QImage &image2)
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
            int tmp1 = (int)(qGreen(p1) + 0.5);
            int tmp2 = (int)(qGreen(p2) + 0.5);
            nSqrSum += (tmp1 - tmp2) * (tmp1 - tmp2);
        }
    }
    qreal dPSNR;
    qreal MSE = (qreal)nSqrSum/w/h;
    if (MSE > 0)
        dPSNR = 10. * qLn(255. * 255. / MSE) / qLn(10);
    else
        dPSNR = 100.;
    return dPSNR;
}

int main()
{
    QTime myTimer;
    SDF bigimg("test-big.png");
    myTimer.start();
    bigimg.calculate();
    qDebug() << "Performance test:" << myTimer.elapsed() << "ms";
    bool ok = true;
    const int tests = 5;
    qDebug() << "PSNR test...";
    for(int i = 1; i <= tests; i++)
    {
        SDF img(QString("test-%1.png").arg(i));
        QImage ref(QString("test-%1-ref.png").arg(i));
        QImage *sdf;
        for(int j = SDF::METHOD_4SED; j <= SDF::METHOD_8SED; j++)
        {
            img.setMethod(j);
            sdf = img.calculate();
            qreal psnr = getPSNR(*sdf, ref);
            qDebug() << "Test" << i << "method" << j << '-' << psnr;
            ok &= psnr > 50.;
        }
        sdf->save(QString("test-%1-sdf.png").arg(i));
    }
    if(ok)
        qDebug() << "OK";
    else
        qDebug() << "FAIL!";
}
