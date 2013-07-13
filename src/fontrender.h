#ifndef FONTRENDER_H
#define FONTRENDER_H

#include <QThread>
#include <QImage>
#include <QPainter>
#include <QList>
//#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "imagepacker.h"

class FontRender : public QObject
{
    Q_OBJECT
public:
    FontRender(Ui_MainWindow *ui = 0);
    ~FontRender();
    bool done;
    bool exporting;
    bool bruteForce;

signals:
    void renderedImage(const QImage &image);
private:
    QTextCodec *pCodec;

    QString imageFileName;
    QString imageExtension;
    QString fileName;

    struct kerningPair {
        QChar first;
        QChar second;
        float kerning;
    };

    struct FontRec {
        enum Metric {
            POINTS,
            PIXELS
        };
        enum {
            SMOOTH = 0x1,
            BOLD = 0x2,
            ITALIC = 0x4
        };
        FontRec(const QString& name, int size, Metric metric, int style)
            : m_font(name), m_size(size), m_metric(metric), m_style(style)
        {}
        inline static Metric GetMetric(const QString& postfix) {
            return (postfix == "pt") ? POINTS : PIXELS;
        }
        inline static QString GetMetricStr(Metric metric) {
            return (metric == POINTS) ? "pt" : "px";
        }
        inline static int GetStyle(const QStringList& strlst) {
            int retVal = 0;
            for (int i = 0; i < strlst.size(); ++i)
            {
                if ("smooth" == strlst.at(i)) retVal |= SMOOTH;
                else if ("b" == strlst.at(i)) retVal |= BOLD;
                else if ("i" == strlst.at(i)) retVal |= ITALIC;
            }
            return retVal;
        }
        QString m_font;
        int     m_size;
        Metric  m_metric;
        int     m_style;
        QFont   m_qfont;
        QList<const packedImage*> m_glyphLst;
        QList<kerningPair> m_kerningList;
    };

    bool outputFNT(const QList<FontRec>& fontLst, const QImage& texture);
    bool outputXML(const QList<FontRec>& fontLst, const QImage& texture);
    bool outputBMFont(const QList<FontRec>& fontLst, const QImage& texture);
    unsigned int qchar2ui(QChar ch);
    QImage texture;
    QList<QImage> glyphs;
    QObject p;
    Ui_MainWindow *ui;
    ImagePacker packer;
public slots:
    void run();
};

#endif // FONTRENDER_H
