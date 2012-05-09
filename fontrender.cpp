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
FontRender::FontRender(Ui_MainWindow *_ui) : ui(_ui)
{ }

FontRender::~FontRender()
{ }

void FontRender::run()
{
    done = false;
    QList<FontRec> fontLst;
    QList<packedImage> glyphLst;
    int i, k, w, h, base;
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
    for(k = 0; k < ui->listOfFonts->count(); k++)
    {
        // extract font paramaters
        QStringList flist = ui->listOfFonts->item(k)->text().split(QString(", "), QString::SkipEmptyParts);
        if(flist.size() != 2) continue;
        QStringList flist2 = flist.at(1).split(' ', QString::SkipEmptyParts);
        if(flist2.size() < 2) continue;
        // make font record and qfont
        FontRec fontRec(flist.at(0), flist2.at(0).toInt(), FontRec::GetMetric(flist2.at(1)), FontRec::GetStyle(flist.mid(2)));
        QFont   font(fontRec.m_font);
        // set fonst size
        if (FontRec::POINTS == fontRec.m_metric)
            font.setPointSize(fontRec.m_size);
        else
            font.setPixelSize(fontRec.m_size);
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
            packedImage pi;
            if(s.indexOf(s.at(i), i+1) > 0) continue;
            w = fm.size(Qt::TextSingleLine, s.at(i)).width();
            h = fm.height();
            base = fm.ascent();
            pi.img = QImage(w, h, baseTxtrFormat);
            pi.img.fill(Qt::transparent);
            pi.crop = QRect(0,0,w,h);
            pi.ch = s.at(i);
            QPainter p(&pi.img);
            p.setFont(font);
            if(exporting)
                p.setCompositionMode(QPainter::CompositionMode_SourceOver);
            else
                p.fillRect(0,0,w,h, ui->transparent->isEnabled() && ui->transparent->isChecked() ? Qt::black : bkgColor);
            p.setPen(fontColor);
            p.drawText(0,base,s.at(i));
            // add rendered glyph
            glyphLst << pi;
            fontRec.m_glyphLst << &glyphLst.last();
        }
        fontLst << fontRec;
    }
    QList<QPoint> points;
    uint width = ui->textureW->value(), height = ui->textureH->value();
    points = packer.pack(&glyphLst, ui->comboMethod->currentIndex(), ui->comboHeuristic->currentIndex(), width, height);
    QImage texture(width, height, baseTxtrFormat);
    texture.fill(bkgColor);
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
    QString texFileName = fileName + "." + ui->outFormat->currentText().toLower();
    // attempt to make output font file
    QFile fntFile(fntFileName);
    if (!fntFile.open(QIODevice::WriteOnly | QIODevice::Text))
        QMessageBox::critical(0, "Error", "Cannot create file " + fntFileName);
    QTextStream fntStrm(&fntFile);
    // output fnt file
    QList<FontRec>::const_iterator itr;
    for (itr = fontLst.begin(); itr != fontLst.end(); ++itr)
    {
        // output font record
        fntStrm << itr->m_font << " "
                << itr->m_size << FontRec::GetMetricStr(itr->m_metric);
        if (itr->m_style & FontRec::BOLD)
            fntStrm << " bold";
        if (itr->m_style & FontRec::ITALIC)
            fntStrm << " italic";
        fntStrm << "\n";
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
            fntStrm << 
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
    texture.save(texFileName, qPrintable(ui->outFormat->currentText()));
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
