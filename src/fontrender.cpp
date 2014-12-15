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
#include <QTime>
#include <stdio.h>
#include <math.h>
#include <limits>

FontRender::FontRender(Ui_MainWindow *_ui) : ui(_ui)
{}

FontRender::~FontRender()
{}

#define WIDTH  1024
#define HEIGHT 1024

struct Point
{
    short dx, dy;
    int f;
};

struct Grid
{
    int w, h;
    Point *grid;
};

Point pointInside = { 0, 0, 0 };
Point pointEmpty = { 9999, 9999, 9999*9999 };
Grid grid[2];

static inline Point Get(Grid &g, int x, int y)
{
    return g.grid[y * (g.w + 2) + x];
}

static inline void Put(Grid &g, int x, int y, const Point &p)
{
    g.grid[y * (g.w + 2) + x] = p;
}

/* macro is a way faster than inline */
#define Compare(offsetx, offsety)                                              \
do {                                                                           \
    int add;                                                                   \
    Point other = Get(g, x + offsetx, y + offsety);                            \
    if(offsety == 0) {                                                         \
        add = 2 * other.dx + 1;                                                \
    }                                                                          \
    else if(offsetx == 0) {                                                    \
        add = 2 * other.dy + 1;                                                \
    }                                                                          \
    else {                                                                     \
        add = 2 * (other.dy + other.dx + 1);                                   \
    }                                                                          \
    other.f += add;                                                            \
    if (other.f < p.f)                                                         \
    {                                                                          \
        p.f = other.f;                                                         \
        if(offsety == 0) {                                                     \
            p.dx = other.dx + 1;                                               \
            p.dy = other.dy;                                                   \
        }                                                                      \
        else if(offsetx == 0) {                                                \
            p.dy = other.dy + 1;                                               \
            p.dx = other.dx;                                                   \
        }                                                                      \
        else {                                                                 \
            p.dy = other.dy + 1;                                               \
            p.dx = other.dx + 1;                                               \
        }                                                                      \
    }                                                                          \
} while(0)

static void GenerateSDF(Grid &g)
{
    for (int y = 1; y <= g.h; y++)
    {
        for (int x = 1; x <= g.w; x++)
        {
            Point p = Get(g, x, y);
            Compare(-1,  0);
            Compare( 0, -1);
            Compare(-1, -1);
            Compare( 1, -1);
            Put(g, x, y, p);
        }
    }

    for(int y = g.h; y > 0; y--)
    {
        for(int x = g.w; x > 0; x--)
        {
            Point p = Get(g, x, y);
            Compare( 1,  0);
            Compare( 0,  1);
            Compare(-1,  1);
            Compare( 1,  1);
            Put(g, x, y, p);
        }
    }
}

static void dfcalculate(QImage *img, int distanceFieldScale, bool transparent)
{
    int x, y;
    int w = img->width(), h = img->height();
    grid[0].w = grid[1].w = w;
    grid[0].h = grid[1].h = h;
    grid[0].grid = (Point*)malloc(sizeof(Point) * (w + 2) * (h + 2));
    grid[1].grid = (Point*)malloc(sizeof(Point) * (w + 2) * (h + 2));
    /* create 1-pixel gap */
    for(x = 0; x < w + 2; x++)
    {
        Put(grid[0], x, 0, pointInside);
        Put(grid[1], x, 0, pointEmpty);
    }
    for(y = 1; y <= h; y++)
    {
        Put(grid[0], 0, y, pointInside);
        Put(grid[1], 0, y, pointEmpty);
        for(x = 1; x <= w; x++)
        {
            if(qGreen(img->pixel(x - 1, y - 1)) > 128)
            {
                Put(grid[0], x, y, pointEmpty);
                Put(grid[1], x, y, pointInside);
            }
            else
            {
                Put(grid[0], x, y, pointInside);
                Put(grid[1], x, y, pointEmpty);
            }
        }
        Put(grid[0], w + 1, y, pointInside);
        Put(grid[1], w + 1, y, pointEmpty);
    }
    for(x = 0; x < w + 2; x++)
    {
        Put(grid[0], x, h + 1, pointInside);
        Put(grid[1], x, h + 1, pointEmpty);
    }
    GenerateSDF(grid[0]);
    GenerateSDF(grid[1]);
    for(y = 1; y <= h; y++)
        for(x = 1; x <= w; x++)
        {
            double dist1 = sqrt((double)(Get(grid[0], x, y).f + 1));
            double dist2 = sqrt((double)(Get(grid[1], x, y).f + 1));
            double dist = dist1 - dist2;
            // Clamp and scale
            int c = dist + 128;
            if(c < 0) c = 0;
            if(c > 255) c = 255;
            if(transparent)
                img->setPixel(x - 1, y - 1, qRgba(255,255,255,c));
            else
                img->setPixel(x - 1, y - 1, qRgb(c,c,c));
        }
    free(grid[0].grid);
    free(grid[1].grid);
}

void FontRender::run()
{
    QTime myTimer;
    myTimer.start();
    done = false;
    QList<FontRec> fontLst;
    QList<packedImage> glyphLst;
    int i, k, base;
    uint width, height;
    QImage::Format baseTxtrFormat;
    QImage::Format glyphTxtrFormat;
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
        glyphTxtrFormat= QImage::Format_ARGB32;
    }
    else if (Qt::Checked == ui->transparent->checkState())
    {
        distanceField = false;
        baseTxtrFormat = QImage::Format_ARGB32_Premultiplied;
        glyphTxtrFormat= QImage::Format_ARGB32_Premultiplied;
    }
    else
    {
        distanceField = false;
        baseTxtrFormat = QImage::Format_RGB32;
        glyphTxtrFormat= QImage::Format_ARGB32_Premultiplied;
    }
    int distanceFieldScale = 4;
    if(exporting)
        distanceFieldScale *= 4;
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
        fontRec.m_qfont = font;
        //rendering glyphs
        QFontMetrics fontMetrics(font);
        base = fontMetrics.ascent() + fontMetrics.leading();
        for (i = 0; i < charList.size(); i++)
        {
            packedImage packed_image;
            if(charList.indexOf(charList.at(i), i + 1) > 0)
                continue;
            QChar charFirst = charList.at(i);
            QSize charSize = fontMetrics.size(0, charFirst);
            packed_image.charWidth = fontMetrics.width(charFirst) / distanceFieldScale;
            int firstBearing = fontMetrics.leftBearing(charFirst);
            packed_image.bearing = firstBearing / distanceFieldScale;
            width = charSize.width() - firstBearing;
            if(exporting && ui->exportKerning->isChecked())
            {
                for (int j = 0; j < charList.size(); ++j)
                {
                    QChar charSecond = charList.at(j);
                    int widthAll = fontMetrics.width(charFirst) + fontMetrics.width(charSecond);
                    QString kernPair(QString(charFirst) + QString(charSecond));
                    float kerning = (float)(fontMetrics.width(kernPair) - widthAll) / (float)distanceFieldScale;
                    if(kerning != 0)
                    {
                        kerningPair kp = {charFirst, charSecond, kerning};
                        fontRec.m_kerningList << kp;
                    }
                }
            }

            height = charSize.height() + fontMetrics.leading();
            QImage buffer;
            if(distanceField)
            {
                buffer = QImage(width, height, glyphTxtrFormat);
                buffer.fill(Qt::transparent);
            }
            else
            {
                packed_image.img = QImage(width, height, glyphTxtrFormat);
                packed_image.img.fill(Qt::transparent);
            }

            packed_image.ch = charFirst;
            QPainter painter(distanceField ? &buffer : &packed_image.img);
            painter.setFont(font);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            //painter.setCompositionMode(QPainter::CompositionMode_Source);
            painter.setPen(fontColor);
            painter.drawText(-firstBearing, base, charFirst);
            if(distanceField)
            {
                dfcalculate(&buffer, distanceFieldScale, exporting && ui->transparent->isEnabled() && ui->transparent->isChecked());
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
    QPainter p;
    if(exporting)
    {
        // Some sort of unicode hack...
        if(ui->encoding->currentText() == "UNICODE")
            pCodec = NULL;
        else
            pCodec = QTextCodec::codecForName(ui->encoding->currentText().toLatin1());
        // draw glyphs
        p.begin(&texture);
        if(!ui->transparent->isChecked() || !ui->transparent->isEnabled())
            p.fillRect(0,0,texture.width(),texture.height(), bkgColor);
        for (i = 0; i < glyphLst.size(); ++i)
            if(glyphLst.at(i).merged == false)
                    p.drawImage(QPoint(glyphLst.at(i).rc.x(), glyphLst.at(i).rc.y()), glyphLst.at(i).img);                   
        p.end();
        // apply distance field calculations if selected
        if(distanceField)
        {
            QImage scaled = texture.scaled(texture.size() * 8, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            dfcalculate(&scaled, 8, exporting && ui->transparent->isEnabled() && ui->transparent->isChecked());
            QImage texture1 = (scaled.scaled(texture.size()));
            texture = texture1;
        }
        if (ui->transparent->isEnabled() && ui->transparent->isChecked())
        {
            if (0 == ui->bitDepth->currentIndex())
                texture = texture.convertToFormat(QImage::Format_Indexed8, Qt::ThresholdAlphaDither | Qt::PreferDither);
        }
        else
        {
            if (0 == ui->bitDepth->currentIndex()) // 8 bit alpha image
                texture = texture.convertToFormat(QImage::Format_Indexed8, Qt::ThresholdAlphaDither | Qt::ThresholdDither);
            else // 24 bit image
                texture = texture.convertToFormat(QImage::Format_RGB888, Qt::ThresholdAlphaDither | Qt::PreferDither);
        }
        bool result;
        // output files
        fileName = ui->outDir->text() + QDir::separator() + ui->outFile->text();
        imageExtension = ui->outFormat->currentText().toLower();
        imageFileName = fileName + "." + imageExtension;
        if (ui->outputFormat->currentText().toLower() == QString("xml"))
            result = outputXML(fontLst, texture);
        else if (ui->outputFormat->currentText().toLower() == QString("bmfont"))
            result = outputBMFont(fontLst, texture);
        else
            result = outputFNT(fontLst, texture);
        // notify user
        if(result)
            QMessageBox::information(0, "Done", "Your font successfully saved in " + ui->outDir->text());
        exporting = false; // reset flag
    }
    else
    {
        // draw glyhps
        p.begin(&texture);
        for (i = 0; i < glyphLst.size(); i++)
            p.drawImage(QPoint(glyphLst.at(i).rc.x(), glyphLst.at(i).rc.y()), glyphLst.at(i).img);

        p.end();  // end of drawing glyphs
        int percent = (int)(((float)packer.area / (float)width / (float)height) * 100.0f + 0.5f);
        float percent2 = (float)(((float)packer.neededArea / (float)width / (float)height) * 100.0f );
        ui->preview->setText(QString("Preview: ") +
                             QString::number(percent) + QString("% filled, ") +
                             QString::number(packer.missingChars) + QString(" chars missed, ") +
                             QString::number(packer.mergedChars) + QString(" chars merged, needed area: ") +
                             QString::number(percent2) + QString("%."));
        if(packer.missingChars == 0) done = true;
        // apply distance field calculations if selected
        if(distanceField)
        {
            QImage scaled = texture.scaled(texture.size()*8, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            dfcalculate(&scaled, 8, exporting && ui->transparent->isEnabled() && ui->transparent->isChecked());
            emit renderedImage(scaled.scaled(texture.size()));
        } else {
            emit renderedImage(texture);
        }
    }
    int nMilliseconds = myTimer.elapsed();
    qDebug() << nMilliseconds;
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

bool FontRender::outputBMFont(const QList<FontRec>& fontLst, const QImage& texture)
{
    int index = 0;
    QList<FontRec>::const_iterator fontRecIt;
    for (fontRecIt = fontLst.begin(); fontRecIt != fontLst.end(); ++fontRecIt)
    {
        // create output file names
        QString fntFileName = fontLst.size() > 1 ? fileName + "_" + QString::number(index++) + ".fnt" : fileName + ".fnt";
        // attempt to make output font file
        QFile fntFile(fntFileName);
        if (!fntFile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QMessageBox::critical(0, "Error", "Cannot create file " + fntFileName);
            return false;
        }
        QTextStream fontStream(&fntFile);
        fontStream.setFieldAlignment(QTextStream::AlignLeft);
        // output "info" tag
        fontStream << "info " <<
                      "face=\"" << fontRecIt->m_font << "\" " <<
                      "size=" << fontRecIt->m_size << " " <<
                      "bold=" << (fontRecIt->m_style & FontRec::BOLD ? 1 : 0) << " " <<
                      "italic=" << (fontRecIt->m_style & FontRec::ITALIC ? 1 : 0) << " " <<
                      "charset=\"\" " <<
                      "unicode=1 " <<
                      "stretchH=100 " <<
                      "smooth=" << (fontRecIt->m_style & FontRec::SMOOTH ? 1 : 0) << " " <<
                      "aa=1 " <<
                      "padding=" << packer.borderTop << "," << packer.borderRight << "," << packer.borderBottom << "," << packer.borderLeft << " " <<
                      "spacing=0,0 " <<
                      "outline=0" << endl;
        // output "common" tag
        QFontMetrics fontMetrics(fontRecIt->m_qfont);
        bool transparent = ui->transparent->isEnabled() && ui->transparent->isChecked();
        fontStream << "common " <<
                      "lineHeight=" << fontMetrics.height() << " " <<
                      "base=" << fontMetrics.ascent() << " " <<
                      "scaleW=" << texture.width() << " " <<
                      "scaleH=" << texture.height() << " " <<
                      "pages=1 " <<
                      "packed=0 " <<
                      "alphaChnl=" << (transparent ? 0 : 4) << " " <<
                      "redChnl=" << (transparent ? 4 : 0) << " " <<
                      "greenChnl=" << (transparent ? 4 : 0) << " " <<
                      "blueChnl=" << (transparent ? 4 : 0) << endl;
        // output "page" tag
        fontStream << "page " <<
                      "id=0 " <<
                      "file=\"" << ui->outFile->text() + "." + imageExtension << "\"" << endl;
        // output "chars" tag
        fontStream << "chars " <<
                      "count=" << fontRecIt->m_glyphLst.size() << endl;
        // output each glyph record
        QList<const packedImage*>::const_iterator chrItr;
        for (chrItr = fontRecIt->m_glyphLst.begin(); chrItr != fontRecIt->m_glyphLst.end(); ++chrItr)
        {
            const packedImage* pGlyph = *chrItr;
            // output glyph metrics
            fontStream << "char " <<
                          "id=" << qSetFieldWidth(4) << pGlyph->ch.unicode() << qSetFieldWidth(0) << " " <<
                          "x=" << qSetFieldWidth(5) << pGlyph->rc.x() << qSetFieldWidth(0) << " " <<
                          "y=" << qSetFieldWidth(5) << pGlyph->rc.y() << qSetFieldWidth(0) << " " <<
                          "width=" << qSetFieldWidth(5) << pGlyph->img.width() << qSetFieldWidth(0) << " " <<
                          "height=" << qSetFieldWidth(5) << pGlyph->img.height() << qSetFieldWidth(0) << " " <<
                          "xoffset=" << qSetFieldWidth(5) << pGlyph->crop.x() + pGlyph->bearing - (int)packer.borderLeft << qSetFieldWidth(0) << " " <<
                          "yoffset=" << qSetFieldWidth(5) << pGlyph->crop.y() - (int)packer.borderTop << qSetFieldWidth(0) << " " <<
                          "xadvance=" << qSetFieldWidth(5) << pGlyph->charWidth << qSetFieldWidth(0) << " " <<
                          "page=0  " <<
                          "chnl=15" << endl;
        }
        const QList<kerningPair> *kerningList = &fontRecIt->m_kerningList;
        if(kerningList->length() > 0)
        {
            // output "kernings" tag
            fontStream << "kernings " <<
                          "count=" << kerningList->size() << endl;
            // output each kerning pair
            for (int i = 0; i < kerningList->length(); ++i) {
                fontStream << "kerning " <<
                              "first=" << qSetFieldWidth(3) << kerningList->at(i).first.unicode() << qSetFieldWidth(0) << " " <<
                              "second=" << qSetFieldWidth(3) << kerningList->at(i).second.unicode() << qSetFieldWidth(0) << " " <<
                              "amount=" << kerningList->at(i).kerning << endl;
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
