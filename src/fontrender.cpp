#include "fontrender.h"
#include "imagepacker.h"
#include <QPainter>
#include <QTextCodec>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <QBuffer>
#include <stdio.h>
#include <math.h>
#include <ft2build.h>
#include FT_FREETYPE_H
FontRender::FontRender(Ui_MainWindow *_ui) : ui(_ui)
{ }

FontRender::~FontRender()
{ }

#define WIDTH  1024
#define HEIGHT 1024

struct Point
{
    int dx, dy;

    int DistSq() const { return dx*dx + dy*dy; }
};

struct Grid
{
    Point grid[HEIGHT][WIDTH];
};

Point pointInside = { 0, 0 };
Point pointEmpty = { 9999, 9999 };
Grid grid1, grid2;

Point Get(Grid &g, int x, int y, int maxW, int maxH)
{
    // OPTIMIZATION: you can skip the edge check code if you make your grid
    // have a 1-pixel gutter.
    if ( x >= 0 && y >= 0 && x < maxW && y < maxH )
        return g.grid[y][x];
    else
        return pointEmpty;
}

void Put( Grid &g, int x, int y, const Point &p )
{
    g.grid[y][x] = p;
}

void Compare( Grid &g, Point &p, int x, int y, int offsetx, int offsety, int maxW, int maxH )
{
    Point other = Get( g, x+offsetx, y+offsety, maxW, maxH );
    other.dx += offsetx;
    other.dy += offsety;

    if (other.DistSq() < p.DistSq())
        p = other;
}

void GenerateSDF(Grid &g, int maxW, int maxH)
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

void dfcalculate(QImage *img, int distanceFieldScale, bool exporting)
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
    QImage::Format baseTxtrFormat = QImage::Format_ARGB32;
    QString s = ui->plainTextEdit->toPlainText();
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
        distanceField = true;
    else
        distanceField = false;
    int distanceFieldScale = 8;
    if(!distanceField)
        distanceFieldScale = 1;
    for(k = 0; k < ui->listOfFonts->count(); k++)
    {
        // extract font paramaters
        QStringList fontName = ui->listOfFonts->item(k)->text().split(QString(", "), QString::SkipEmptyParts);
        if(fontName.size() != 2) continue;
        QStringList fontOptList = fontName.at(1).split(' ', QString::SkipEmptyParts);
        if(fontOptList.size() < 2) continue;
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
        QFontMetrics fm(font);
        for (i = 0; i < s.size(); i++)
        {
            packedImage packed_image;
            if(s.indexOf(s.at(i), i + 1) > 0) continue;
            width = fm.size(Qt::TextSingleLine, s.at(i)).width();
            height = fm.height();
            base = fm.ascent();
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
            packed_image.crop = QRect(0,0,width,height);
            packed_image.ch = s.at(i);
            QPainter painter(distanceField ? &buffer : &packed_image.img);
            painter.setFont(font);
            if(exporting)
                painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            else
                painter.fillRect(0, 0, width, height,
                                 ui->transparent->isEnabled() && ui->transparent->isChecked() ? Qt::black : bkgColor);
            painter.setPen(fontColor);
            painter.drawText(0,base,s.at(i));
            if(distanceField)
            {
                dfcalculate(&buffer, distanceFieldScale, exporting);
                packed_image.img = buffer.scaled(buffer.size() / distanceFieldScale);
            }
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
        // draw glyphs
        if(!ui->transparent->isChecked() || ui->transparent->isEnabled())
            p.fillRect(0,0,texture.width(),texture.height(), bkgColor);
        for (i = 0; i < glyphLst.size(); ++i) {
            p.drawImage(QPoint(glyphLst.at(i).rc.x(), glyphLst.at(i).rc.y()), glyphLst.at(i).img);
        }
        if (ui->transparent->isEnabled() && ui->transparent->isChecked()) {
            if (0 == ui->bitDepth->currentIndex()) // 8 bit alpha image
                texture = texture.convertToFormat(QImage::Format_Indexed8, Qt::DiffuseAlphaDither | Qt::PreferDither);
        } else {
            if (0 == ui->bitDepth->currentIndex()) // 8 bit
                texture = texture.convertToFormat(QImage::Format_Indexed8, Qt::ThresholdAlphaDither |Qt::PreferDither);
            else  // 24 bit image
                texture = texture.convertToFormat(QImage::Format_RGB888, Qt::ThresholdAlphaDither | Qt::PreferDither);
        }
        // output files
        QString fileName = ui->outDir->text() + QDir::separator() + ui->outFile->text();
        if (ui->outputFormat->currentText().toLower() == QString("xml"))
            outputXML(fontLst, texture, fileName);
        else
            outputFNT(fontLst, texture, fileName);
        // notify user
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

void FontRender::outputFNT(const QList<FontRec>& fontLst, const QImage& texture, QString& fileName)
{
    QTextCodec *pCodec = QTextCodec::codecForName(ui->encoding->currentText().toAscii());
    // create output file names
    QString fntFileName = fileName + ".fnt";
    QString imageFileName = fileName + "." + ui->outFormat->currentText().toLower();
    // attempt to make output font file
    QFile fntFile(fntFileName);
    if (!fntFile.open(QIODevice::WriteOnly | QIODevice::Text))
        QMessageBox::critical(0, "Error", "Cannot create file " + fntFileName);
    QTextStream fontStream(&fntFile);
    fontStream << "textures: " << imageFileName << "\n";
    // output fnt file
    QList<FontRec>::const_iterator itr;
    for (itr = fontLst.begin(); itr != fontLst.end(); ++itr)
    {
        // output font record
        fontStream << itr->m_font << " "
                   << itr->m_size << FontRec::GetMetricStr(itr->m_metric);
        if (itr->m_style & FontRec::BOLD)
            fontStream << " bold";
        if (itr->m_style & FontRec::ITALIC)
            fontStream << " italic";
        fontStream << "\n";
        // output each glyph record
        QList<const packedImage*>::const_iterator chrItr;
        for (chrItr = itr->m_glyphLst.begin(); chrItr != itr->m_glyphLst.end(); ++chrItr)
        {
            const packedImage* pGlyph = *chrItr;
            // calc character value
            QByteArray encodedString = pCodec->fromUnicode((QString)pGlyph->ch);
            unsigned int chr = (unsigned char)encodedString.data()[0];
            for(int j = 1; j < encodedString.size(); j++)
                chr = (chr << 8) +(unsigned char)encodedString.data()[j];
            // output glyph metrics
            fontStream <<
                          chr << "\t" <<
                          pGlyph->rc.x() << "\t" <<
                          pGlyph->rc.y() << "\t" <<
                          pGlyph->crop.width() << "\t" <<
                          pGlyph->crop.height() << "\t" <<
                          pGlyph->crop.x() << "\t" <<
                          pGlyph->crop.y() << "\t" <<
                          pGlyph->rc.width() << "\t" <<
                          pGlyph->rc.height() << "\t" << "\n";
        }
    }
    /* output font texture */
    texture.save(imageFileName, qPrintable(ui->outFormat->currentText()));
}

void FontRender::outputXML(const QList<FontRec>& fontLst, const QImage& texture, QString& fileName)
{
    QTextCodec *pCodec = QTextCodec::codecForName(ui->encoding->currentText().toAscii());
    // create output file names
    QString fntFileName = fileName + ".xml";
    // attempt to make output font file
    QFile fntFile(fntFileName);
    if (!fntFile.open(QIODevice::WriteOnly | QIODevice::Text))
        QMessageBox::critical(0, "Error", "Cannot create file " + fntFileName);
    QTextStream fntStrm(&fntFile);
    // output fnt file
    fntStrm << "<?xml version=\"1.0\"?>\n";
    fntStrm << "<fontList>\n";
    QList<FontRec>::const_iterator itr;
    for (itr = fontLst.begin(); itr != fontLst.end(); ++itr)
    {
        // output font record
        fntStrm <<
                   "\t<font name=\"" << itr->m_font << "\" " <<
                   "size=\"" << itr->m_size << FontRec::GetMetricStr(itr->m_metric) << "\" ";
        if (itr->m_style & FontRec::BOLD)
            fntStrm << "bold=\"true\" ";
        if (itr->m_style & FontRec::ITALIC)
            fntStrm << "italic=\"true\"";
        fntStrm << ">\n";
        // output each glyph record
        QList<const packedImage*>::const_iterator chrItr;
        for (chrItr = itr->m_glyphLst.begin(); chrItr != itr->m_glyphLst.end(); ++chrItr)
        {
            const packedImage* pGlyph = *chrItr;
            // calc character value
            QByteArray encodedString = pCodec->fromUnicode((QString)pGlyph->ch);
            unsigned int chr = (unsigned char)encodedString.data()[0];
            for(int j = 1; j < encodedString.size(); j++)
                chr = (chr << 8) +(unsigned char)encodedString.data()[j];
            // output glyph metrics
            fntStrm << "\t\t<char " <<
                       "id=\"" << chr << "\" " <<
                       "x=\"" << pGlyph->rc.x() << "\" " <<
                       "y=\"" << pGlyph->rc.y() << "\" " <<
                       "width=\"" << pGlyph->rc.width() << "\" " <<
                       "height=\"" << pGlyph->rc.height() << "\" " <<
                       "Xoffset=\"" << pGlyph->crop.x() << "\" " <<
                       "Yoffset=\"" << pGlyph->crop.y() << "\" " <<
                       "OrigWidth=\"" << pGlyph->rc.width() << "\" " <<
                       "OrigHeight=\"" << pGlyph->rc.height() << "\" " <<
                       "/>\n";
        }
        fntStrm << "\t</font>\n";
    }
    /* output font texture */
    QByteArray imgArray;
    QBuffer imgBuffer(&imgArray);
    imgBuffer.open(QIODevice::WriteOnly);
    texture.save(&imgBuffer, qPrintable(ui->outFormat->currentText()));
    QString imgBase64(imgArray.toBase64());
    fntStrm << "\t<texture width=\"" << texture.width() << "\" height=\"" << texture.height() << "\" >\n";
    fntStrm << imgBase64 << "\n";
    fntStrm << "\t</texture>\n";
    fntStrm << "</fontList>\n";
}