#include "fontrender.h"
#include "imagepacker.h"
#include <QPainter>
#include <QTextCodec>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <stdio.h>
FontRender::FontRender(Ui_MainWindow *_ui)
{
    
    ui = _ui;
   // texture = new QImage(512, 512, QImage::Format_ARGB32_Premultiplied);
}

//~ void UpdateAlphaChannel(QImage *image)
//~ {
    //~ unsigned int a;
    //~ for(int x = 0; x < image->width(); x++)
    //~ {
        //~ for(int y = 0; y < image->height(); y++)
        //~ {
            //~ QColor color(image->pixel(x,y));
            //~ 
            //~ color.setRgb(255,255,255,color.alpha());
            //~ image->setPixel(x,y,color.rgba());
        //~ }
    //~ }
//~ }

void FontRender::run()
{
    //QImage texture(512, 512, QImage::Format_ARGB32_Premultiplied);
    //QPainter p(&texture);
    //QFont font = ui->fontComboBox->currentFont();
    //font.setPointSize(ui->spinFontSize->value());
    //font.setStyleStrategy(ui->checkFontSmoothing->isChecked()?QFont::PreferAntialias:QFont::NoAntialias);
    //ui->listWidget->addItem(font.family());
    //p.setFont(font);
    //p.setPen(Qt::white);
    //p.drawText(0,10,ui->plainTextEdit->toPlainText());
    //emit renderedImage(texture);
    done = false;
    QString s = ui->plainTextEdit->toPlainText();
    int i, j, w, h, base;
    //QRect rc;
    packer.sortOrder = ui->sortOrder->currentIndex();
    packer.borderTop = ui->borderTop->value();
    packer.borderLeft = ui->borderLeft->value();
    packer.borderRight = ui->borderRight->value();
    packer.borderBottom = ui->borderBottom->value();
    packer.trim = ui->trim->isChecked();
    packer.merge = ui->merge->isChecked();
    packer.mergeBF = ui->mergeBF->isChecked();
    //~ qDebug("%d", packer.sortOrder);
    QList<packedImage> list;
    QList<packedImage*> listptr;
    int k;
    //~ QStringList fontslist = ui->listOfFonts->toPlainText().split('\n', QString::SkipEmptyParts);
    QList<QString> fontsnames;
    //~ bool border;
    for(k = 0; k < ui->listOfFonts->count(); k++)
    {
        QString fontsname = "";
        QStringList flist = ui->listOfFonts->item(k)->text().split(QString(", "), QString::SkipEmptyParts);
        if(flist.size() != 2) continue;
        QStringList flist2 = flist.at(1).split(' ', QString::SkipEmptyParts);
        if(flist2.size() < 2) continue;
        fontsname += flist.at(0);
        fontsname += QString(" ") + flist2.at(0) + flist2.at(1);
        QFont font = QFont(flist.at(0));
        if(flist2.at(1) == "pt") font.setPointSize(flist2.at(0).toInt());
        else font.setPixelSize(flist2.at(0).toInt());
        font.setStyleStrategy(QFont::NoAntialias);
        for(i = 2; i < flist2.size(); i++)
        {
            if(flist2.at(i) == "smooth")
                font.setStyleStrategy((QFont::StyleStrategy)(QFont::PreferDevice|QFont::PreferMatch));
            else if(flist2.at(i) == "b")
            {
                font.setWeight(QFont::Bold);
                fontsname += " bold";
            }
            else if(flist2.at(i) == "i")
            {
                font.setItalic(true);
                fontsname += " italic";
            }
        }
        fontsnames << fontsname;
        QFontMetrics fm(font);
        //rendering glyphs
        for (i = 0; i < s.size(); i++)
        {
            if(s.indexOf(s.at(i), i+1) > 0) continue;
            //rc = fm.tightBoundingRect(s.at(i));
            w = fm.size(Qt::TextSingleLine, s.at(i)).width();
            h = fm.height();
            base = fm.ascent();
            packedImage pi;
            pi.img = QImage(w, h, QImage::Format_ARGB32);
            pi.img.fill(Qt::transparent);
            pi.crop = QRect(0,0,w,h);
            //~ pi.border = true;
            pi.ch = s.at(i);
            list << pi;
            listptr << &list.last();
            //qDebug("%d %d", w,h);
            QPainter p(&list.last().img);
            p.setFont(font);
            if(exporting)
                //грязный хак. Хрен знает почему, но QT делает прозрачный шрифт жёлтым
                p.setCompositionMode(QPainter::CompositionMode_Multiply);
            else
                p.fillRect(0,0,w,h,Qt::black);
            p.setPen(Qt::white);
            p.drawText(0,base,s.at(i));
            //~ list.last().img.save(s.at(i) + QString(".png"),"png");
            //emit renderedImage(list.last());
            //this->msleep(100);
        }
    }
    QList<QPoint> points;
    uint width = ui->textureW->value(), height = ui->textureH->value();
    //~ qDebug("bol %d", packer.ltr);
    points = packer.pack(&list, ui->comboMethod->currentIndex(), ui->comboHeuristic->currentIndex(), width, height);
    QImage texture(width, height, QImage::Format_ARGB32);
    texture.fill(Qt::transparent);
    QPainter p(&texture);
    if(exporting)
        if(!ui->transparent->isChecked())
            p.fillRect(0,0,texture.width(),texture.height(),Qt::black);

    if(exporting)
    {
        QTextCodec *codec = QTextCodec::codecForName(ui->encoding->currentText().toAscii());
        QString fntFile = ui->outDir->text();
        fntFile += QDir::separator();
        fntFile += ui->outFile->text();
        fntFile += ".fnt";
        QString imgFile = ui->outFile->text();
        imgFile += ".";
        imgFile += ui->outFormat->currentText().toLower();
        QString imgdirFile = ui->outDir->text();
        imgdirFile += QDir::separator();
        imgdirFile += imgFile;
        QFile outFntFile(fntFile);
        if (!outFntFile.open(QIODevice::WriteOnly | QIODevice::Text))
            QMessageBox::critical(0, "Error", "Cannot create file " + fntFile);
        else
        {
            QTextStream out(&outFntFile);
            out << "textures: " << imgFile << "\n";
            unsigned int chr;
            for (i = 0, j = 0; i < list.size(); i++)
            {
                if(i % s.size() == 0)
                {
                    out << "\n" << fontsnames.at(j) << "\n";
                    j++;
                }
                p.drawImage(QPoint(list.at(i).rc.x(), list.at(i).rc.y()), list.at(i).img);
                QByteArray encodedString = codec->fromUnicode((QString)list.at(i).ch);
                chr = (unsigned char)encodedString.data()[0];
                for(k = 1; k < encodedString.size(); k++)
                    chr = (chr << 8) +(unsigned char)encodedString.data()[k];
                out << 
                                chr << "\t" <<
                                list.at(i).rc.x() << "\t" <<
                                list.at(i).rc.y() << "\t" <<
                                list.at(i).crop.width() << "\t" <<
                                list.at(i).crop.height() << "\t" <<
                                list.at(i).crop.x() << "\t" <<
                                list.at(i).crop.y() << "\t" <<
                                list.at(i).rc.width() << "\t" <<
                                list.at(i).rc.height() << "\t" << "\n";
            }
            texture.save(imgdirFile, qPrintable(ui->outFormat->currentText()));
            QMessageBox::information(0, "Done", "Your font successfully saved in " + ui->outDir->text());
        }
        exporting = false;
    }
    else
    {
        for (i = 0, j = 0; i < list.size(); i++)
            p.drawImage(QPoint(list.at(i).rc.x(), list.at(i).rc.y()), list.at(i).img);
    
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
FontRender::~FontRender()
{

}
