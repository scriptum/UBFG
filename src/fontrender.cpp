#include "fontrender.h"
#include "imagepacker.h"
#include <QPainter>
#include <QTextCodec>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <QBuffer>
#include <QDebug>
#include <stdio.h>
#include <math.h>

FontRender::FontRender(Ui_MainWindow *_ui) : ui(_ui)
{}

FontRender::~FontRender()
{}

#define WIDTH  1024
#define HEIGHT 1024

struct Point
{
    int dx, dy;

    int DistSq() const { return dx * dx + dy * dy; }
};

struct Grid
{
    Point grid[HEIGHT][WIDTH];
};

Point pointInside = { 0, 0 };
Point pointEmpty = { 9999, 9999 };
Grid grid1, grid2;

static inline Point Get(Grid &g, int x, int y, int maxW, int maxH)
{
    // OPTIMIZATION: you can skip the edge check code if you make your grid
    // have a 1-pixel gutter.
    if ( x >= 0 && y >= 0 && x < maxW && y < maxH )
        return g.grid[y][x];
    else
        return pointEmpty;
}

static inline void Put( Grid &g, int x, int y, const Point &p )
{
    g.grid[y][x] = p;
}

static inline void Compare( Grid &g, Point &p, int x, int y, int offsetx, int offsety, int maxW, int maxH )
{
    Point other = Get( g, x+offsetx, y+offsety, maxW, maxH );
    other.dx += offsetx;
    other.dy += offsety;

    if (other.DistSq() < p.DistSq())
        p = other;
}

static void GenerateSDF(Grid &g, int maxW, int maxH)
{
    // Pass 0
    for (int y=0;y<maxH;y++)
    {
        for (int x=0;x<maxW;x++)
        {
            Point p = Get( g, x, y, maxW, maxH );
            Compare( g, p, x, y, -1,  0, maxW, maxH );
            Compare( g, p, x, y,  0, -1, maxW, maxH );
            Compare( g, p, x, y, -1, -1, maxW, maxH );
            Compare( g, p, x, y,  1, -1, maxW, maxH );
            Put( g, x, y, p );
        }

        for (int x=maxW-1;x>=0;x--)
        {
            Point p = Get( g, x, y, maxW, maxH );
            Compare( g, p, x, y, 1, 0, maxW, maxH );
            Put( g, x, y, p );
        }
    }

    // Pass 1
    for (int y=maxH-1;y>=0;y--)
    {
        for (int x=maxW-1;x>=0;x--)
        {
            Point p = Get( g, x, y, maxW, maxH );
            Compare( g, p, x, y,  1,  0, maxW, maxH );
            Compare( g, p, x, y,  0,  1, maxW, maxH );
            Compare( g, p, x, y, -1,  1, maxW, maxH );
            Compare( g, p, x, y,  1,  1, maxW, maxH );
            Put( g, x, y, p );
        }

        for (int x=0;x<maxW;x++)
        {
            Point p = Get( g, x, y, maxW, maxH );
            Compare( g, p, x, y, -1, 0, maxW, maxH );
            Put( g, x, y, p );
        }
    }
}

static void dfcalculate(QImage *img, int distanceFieldScale, bool exporting)
{
    int x, y;
    int maxW = img->width(), maxH = img->height();
    for(y = 0; y < maxH; y++)
        for(x = 0; x < maxW; x++)
        {
            if ( qGreen(img->pixel(x, y)) < 128 )
            {
                Put( grid1, x, y, pointInside );
                Put( grid2, x, y, pointEmpty );
            }
            else
            {
                Put( grid2, x, y, pointInside );
                Put( grid1, x, y, pointEmpty );
            }
        }
    // Generate the SDF.
    GenerateSDF(grid1, maxW, maxH );
    GenerateSDF(grid2, maxW, maxH );
    for(y = 0; y < maxH; y++)
        for(x = 0; x < maxW; x++)
        {
            // Calculate the actual distance from the dx/dy
            double dist1 = sqrt( (double)Get( grid1, x, y, maxW, maxH ).DistSq() );
            double dist2 = sqrt( (double)Get( grid2, x, y, maxW, maxH ).DistSq() );
            double dist = dist1 - dist2;
            // Clamp and scale it, just for display purposes.
            int c = dist * 64 / distanceFieldScale + 128;
            if ( c < 0 ) c = 0;
            if ( c > 255 ) c = 255;
            if(exporting)
                img->setPixel(x, y, qRgba(255,255,255,c));
            else
                img->setPixel(x, y, qRgb(c,c,c));
        }
}

void FontRender::run()
{
    done = false;
    QList<FontRec> fontLst;
    QList<packedImage> glyphLst;
    int i, k, base;
    uint width, height;
    QImage::Format baseTxtrFormat;
    QString charList = ui->plainTextEdit->toPlainText();
    packer.sortOrder = ui->sortOrder->currentIndex();
    packer.borderTop = ui->borderTop->value();
    packer.borderLeft = ui->borderLeft->value();
    packer.borderRight = ui->borderRight->value();
    packer.borderBottom = ui->borderBottom->value();
    packer.trim = ui->trim->isChecked();
    packer.merge = ui->merge->isChecked();
    packer.mergeBF = ui->mergeBF->isChecked();
    QColor fontColor = ui->fontColor->palette().brush(QPalette::Button).color();
    QColor bkgColor = ui->transparent->isEnabled() && ui->transparent->isChecked() ? Qt::transparent : ui->backgroundColor->palette().brush(QPalette::Button).color();
    bool distanceField;
    if(ui->distanceField->isChecked())
    {
        distanceField = true;
        baseTxtrFormat = QImage::Format_ARGB32;
    }
    else
    {
        distanceField = false;
        baseTxtrFormat = QImage::Format_ARGB32_Premultiplied;
    }
    int distanceFieldScale = 8;
    if(!distanceField)
        distanceFieldScale = 1;
    for(k = 0; k < ui->listOfFonts->count(); k++)
    {
        // extract font paramaters
        QStringList fontName = ui->listOfFonts->item(k)->text().split(QString(", "), QString::SkipEmptyParts);
        if(fontName.size() != 2)
            continue;
        QStringList fontOptList = fontName.at(1).split(' ', QString::SkipEmptyParts);
        if(fontOptList.size() < 2)
            continue;
        // make font record and qfont
        FontRec fontRec(fontName.at(0), fontOptList.at(0).toInt(), FontRec::GetMetric(fontOptList.at(1)), FontRec::GetStyle(fontOptList.mid(2)));
        QFont   font(fontRec.m_font);
        // set fonst size
        if (FontRec::POINTS == fontRec.m_metric)
            font.setPointSize(fontRec.m_size * distanceFieldScale);
        else
            font.setPixelSize(fontRec.m_size * distanceFieldScale);
        // set font style
        font.setStyleStrategy(QFont::NoAntialias);
        if (fontRec.m_style & FontRec::SMOOTH)
            font.setStyleStrategy((QFont::StyleStrategy)(QFont::PreferDevice|QFont::PreferMatch));
        if (fontRec.m_style & FontRec::BOLD)
            font.setWeight(QFont::Bold);
        if (fontRec.m_style & FontRec::ITALIC)
            font.setItalic(true);
        //rendering glyphs
        QFontMetrics fontMetrics(font);
        base = fontMetrics.ascent();
        for (i = 0; i < charList.size(); i++)
        {
            packedImage packed_image;
            if(charList.indexOf(charList.at(i), i + 1) > 0)
                continue;
            QChar charFirst = charList.at(i);
            QSize charSize = fontMetrics.size(0, charFirst);
            packed_image.charWidth = fontMetrics.width(charFirst) / distanceFieldScale;
            int firstBearing = fontMetrics.leftBearing(charFirst);
//            firstBearing = firstBearing > 0 ? 0 : firstBearing;
            packed_image.bearing = firstBearing / distanceFieldScale;
//            qDebug() << fontMetrics.leftBearing(charFirst) << charFirst;
            width = charSize.width() - firstBearing;
            if(exporting && ui->exportKerning->isChecked())
            {
                for (int j = 0; j < charList.size(); ++j)
                {
                    QChar charSecond = charList.at(j);
//                    int secondBearing = fontMetrics.leftBearing(charSecond);
//                    secondBearing = secondBearing > 0 ? 0 : secondBearing;
//                    int widthAll = charSize.width() + fontMetrics.size(0, charSecond).width() - secondBearing;
                    int widthAll = fontMetrics.width(charFirst) + fontMetrics.width(charSecond);
                    QString kernPair(QString(charFirst) + QString(charSecond));
//                    float kerning = (float)(fontMetrics.size(0, kernPair).width() - widthAll) / (float)distanceFieldScale;
                    float kerning = (float)(fontMetrics.width(kernPair) - widthAll) / (float)distanceFieldScale;
                    if(kerning != 0)
                    {
                        kerningPair kp = {charFirst, charSecond, kerning};
                        fontRec.m_kerningList << kp;
                    }
                }
            }

            height = charSize.height();
            QImage buffer;
            if(distanceField)
            {
                buffer = QImage(width, height, baseTxtrFormat);
                buffer.fill(Qt::transparent);
            }
            else
            {
                packed_image.img = QImage(width, height, baseTxtrFormat);
                packed_image.img.fill(Qt::transparent);
            }

            packed_image.ch = charFirst;
            QPainter painter(distanceField ? &buffer : &packed_image.img);
            painter.setFont(font);
            if(exporting)
                painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            else
                painter.fillRect(0, 0, width, height,
                                 ui->transparent->isEnabled() && ui->transparent->isChecked() ? Qt::black : bkgColor);
            painter.setPen(fontColor);
            painter.drawText(-firstBearing, base, charFirst);
            if(distanceField)
            {
                dfcalculate(&buffer, distanceFieldScale, exporting);
                packed_image.img = buffer.scaled(buffer.size() / distanceFieldScale);
            }
            packed_image.crop = packed_image.img.rect();
            // add rendered glyph
            glyphLst << packed_image;
            fontRec.m_glyphLst << &glyphLst.last();
        }
        fontLst << fontRec;
    }
    QList<QPoint> points;
    width = ui->textureW->value();
    height = ui->textureH->value();
    points = packer.pack(&glyphLst, ui->comboHeuristic->currentIndex(), width, height);
    QImage texture(width, height, baseTxtrFormat);
    texture.fill(bkgColor.rgba());
    QPainter p(&texture);
    if(exporting)
    {
        // Some sort of unicode hack...
        if(ui->encoding->currentText() == "UNICODE")
            pCodec = NULL;
        else
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
            pCodec = QTextCodec::codecForName(ui->encoding->currentText().toLatin1());
#else
            pCodec = QTextCodec::codecForName(ui->encoding->currentText().toAscii());
#endif
        // draw glyphs
        if(!ui->transparent->isChecked() || ui->transparent->isEnabled())
            p.fillRect(0,0,texture.width(),texture.height(), bkgColor);
        for (i = 0; i < glyphLst.size(); ++i)
            if(glyphLst.at(i).merged == false)
                    p.drawImage(QPoint(glyphLst.at(i).rc.x(), glyphLst.at(i).rc.y()), glyphLst.at(i).img);

        if (ui->transparent->isEnabled() && ui->transparent->isChecked())
        {
            if (0 == ui->bitDepth->currentIndex()) // 8 bit alpha image
                texture.convertToFormat(QImage::Format_Indexed8, Qt::DiffuseAlphaDither | Qt::PreferDither);
        }
        else
        {
            if (0 == ui->bitDepth->currentIndex()) // 8 bit
                texture.convertToFormat(QImage::Format_Indexed8, Qt::ThresholdAlphaDither |Qt::PreferDither);
            else  // 24 bit image
                texture.convertToFormat(QImage::Format_RGB888, Qt::ThresholdAlphaDither | Qt::PreferDither);
        }
        bool result;
        // output files
        fileName = ui->outDir->text() + QDir::separator() + ui->outFile->text();
        imageExtension = ui->outFormat->currentText().toLower();
        imageFileName = fileName + "." + imageExtension;
        if (ui->outputFormat->currentText().toLower() == QString("xml"))
            result = outputXML(fontLst, texture);
        else
            result = outputFNT(fontLst, texture);
        // notify user
        if(result)
            QMessageBox::information(0, "Done", "Your font successfully saved in " + ui->outDir->text());
        exporting = false; // reset flag
    }
    else
    {
        for (i = 0; i < glyphLst.size(); i++)
            p.drawImage(QPoint(glyphLst.at(i).rc.x(), glyphLst.at(i).rc.y()), glyphLst.at(i).img);

        int percent = (int)(((float)packer.area / (float)width / (float)height) * 100.0f + 0.5f);
        float percent2 = (float)(((float)packer.neededArea / (float)width / (float)height) * 100.0f );
        ui->preview->setText(QString("Preview: ") +
                             QString::number(percent) + QString("% filled, ") +
                             QString::number(packer.missingChars) + QString(" chars missed, ") +
                             QString::number(packer.mergedChars) + QString(" chars merged, needed area: ") +
                             QString::number(percent2) + QString("%."));
        if(packer.missingChars == 0) done = true;
        emit renderedImage(texture);
    }
}

unsigned int FontRender::qchar2ui(QChar ch)
{
    // fast UNICODE fallback
    if(pCodec == NULL)
        return ch.unicode();
    QByteArray encodedString = pCodec->fromUnicode((QString)ch);
    unsigned int chr = (unsigned char)encodedString.data()[0];
    for(int j = 1; j < encodedString.size(); j++)
        chr = (chr << 8) + (unsigned char)encodedString.data()[j];
    return chr;
}

bool FontRender::outputFNT(const QList<FontRec>& fontLst, const QImage& texture)
{
    // create output file names
    QString fntFileName = fileName + ".fnt";
    // attempt to make output font file
    QFile fntFile(fntFileName);
    if (!fntFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::critical(0, "Error", "Cannot create file " + fntFileName);
        return false;
    }
    QTextStream fontStream(&fntFile);
    fontStream << "textures: " << ui->outFile->text() + "." + imageExtension << "\n";
    // output fnt file
    QList<FontRec>::const_iterator fontRecIt;
    for (fontRecIt = fontLst.begin(); fontRecIt != fontLst.end(); ++fontRecIt)
    {
        // output font record
        fontStream << fontRecIt->m_font << " "
                   << fontRecIt->m_size << FontRec::GetMetricStr(fontRecIt->m_metric);
        if (fontRecIt->m_style & FontRec::BOLD)
            fontStream << " bold";
        if (fontRecIt->m_style & FontRec::ITALIC)
            fontStream << " italic";
        fontStream << "\n";
        // output each glyph record
        QList<const packedImage*>::const_iterator chrItr;
        for (chrItr = fontRecIt->m_glyphLst.begin(); chrItr != fontRecIt->m_glyphLst.end(); ++chrItr)
        {
            const packedImage* pGlyph = *chrItr;
            // output glyph metrics
            fontStream <<
                          qchar2ui(pGlyph->ch) << "\t" <<
                          pGlyph->rc.x() << "\t" <<
                          pGlyph->rc.y() << "\t" <<
                          pGlyph->crop.width() << "\t" <<
                          pGlyph->crop.height() << "\t" <<
                          pGlyph->crop.x() + pGlyph->bearing<< "\t" <<
                          pGlyph->crop.y() << "\t" <<
                          pGlyph->charWidth << "\t" <<
                          pGlyph->rc.height() << "\n";
        }
        const QList<kerningPair> *kerningList = &fontRecIt->m_kerningList;
        if(kerningList->length() > 0)
        {
            fontStream << "kerning pairs:\n";
            for (int i = 0; i < kerningList->length(); ++i) {
                fontStream << qchar2ui(kerningList->at(i).first) << '\t' <<
                              qchar2ui(kerningList->at(i).second) << '\t' <<
                              kerningList->at(i).kerning << '\n';
            }
        }
    }
    /* output font texture */
    if(!texture.save(imageFileName, qPrintable(ui->outFormat->currentText())))
    {
        QMessageBox::critical(0, "Error", "Cannot save image " + imageFileName);
        return false;
    }
    return true;
}

bool FontRender::outputXML(const QList<FontRec>& fontLst, const QImage& texture)
{
    // create output file names
    QString xmlFileName = fileName + ".xml";
    // attempt to make output font file
    QFile fntFile(xmlFileName);
    if (!fntFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::critical(0, "Error", "Cannot create file " + xmlFileName);
        return false;
    }
    QTextStream fontStream(&fntFile);
    // output fnt file
    fontStream << "<?xml version=\"1.0\"?>\n";
    fontStream << "<fontList>\n";
    QList<FontRec>::const_iterator fontRecIt;
    for (fontRecIt = fontLst.begin(); fontRecIt != fontLst.end(); ++fontRecIt)
    {
        // output font record
        fontStream <<
                   "\t<font name=\"" << fontRecIt->m_font << "\" " <<
                   "size=\"" << fontRecIt->m_size << FontRec::GetMetricStr(fontRecIt->m_metric) << "\" ";
        if (fontRecIt->m_style & FontRec::BOLD)
            fontStream << "bold=\"true\" ";
        if (fontRecIt->m_style & FontRec::ITALIC)
            fontStream << "italic=\"true\"";
        fontStream << ">\n";
        // output each glyph record
        QList<const packedImage*>::const_iterator chrItr;
        for (chrItr = fontRecIt->m_glyphLst.begin(); chrItr != fontRecIt->m_glyphLst.end(); ++chrItr)
        {
            const packedImage* pGlyph = *chrItr;
            // output glyph metrics
            fontStream << "\t\t<char " <<
                       "id=\"" << qchar2ui(pGlyph->ch) << "\" " <<
                       "x=\"" << pGlyph->rc.x() << "\" " <<
                       "y=\"" << pGlyph->rc.y() << "\" " <<
                       "width=\"" << pGlyph->crop.width() << "\" " <<
                       "height=\"" << pGlyph->crop.height() << "\" " <<
                       "Xoffset=\"" << pGlyph->crop.x() << "\" " <<
                       "Yoffset=\"" << pGlyph->crop.y() << "\" " <<
                       "OrigWidth=\"" << pGlyph->charWidth << "\" " <<
                       "OrigHeight=\"" << pGlyph->rc.height() << "\" " <<
                       "/>\n";
        }
        const QList<kerningPair> *kerningList = &fontRecIt->m_kerningList;
        for (int i = 0; i < kerningList->length(); ++i) {
            fontStream << "\t\t<kerning " <<
                          "first=\"" << qchar2ui(kerningList->at(i).first) << "\" " <<
                          "second=\"" << qchar2ui(kerningList->at(i).second) << "\" " <<
                          "value=\"" << kerningList->at(i).kerning << "\" />\n";
        }
        fontStream << "\t</font>\n";
    }
    if(ui->saveImageInsideXML->isChecked()){
        QByteArray imgArray;
        QBuffer imgBuffer(&imgArray);
        imgBuffer.open(QIODevice::WriteOnly);
        texture.save(&imgBuffer, qPrintable(ui->outFormat->currentText()));
        QString imgBase64(imgArray.toBase64());
        fontStream << "\t<texture width=\"" << texture.width() << "\" height=\"" << texture.height() << "\" format=\"" << imageExtension << "\">\n";
        fontStream << imgBase64 << "\n";
        fontStream << "\t</texture>\n";
    }
    else
    {
        if(!texture.save(imageFileName, qPrintable(ui->outFormat->currentText())))
        {
            QMessageBox::critical(0, "Error", "Cannot save image " + imageFileName);
            return false;
        }
    }
    fontStream << "</fontList>\n";
    return true;
}
