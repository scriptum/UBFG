#include "fontrender.h"
#include "imagepacker.h"
#include <QPainter>
#include <QTextCodec>
#include <stdio.h>
FontRender::FontRender(Ui_MainWindow *_ui)
{
    
    ui = _ui;
   // texture = new QImage(512, 512, QImage::Format_ARGB32_Premultiplied);
}

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
    qDebug("%d", packer.sortOrder);
    QList<packedImage> list;
    QList<packedImage*> listptr;
    int k;
    QStringList fontslist = ui->listOfFonts->toPlainText().split('\n', QString::SkipEmptyParts);
    QList<QString> fontsnames;
    bool border;
    for(k = 0; k < fontslist.size(); k++)
    {
        QString fontsname = "";
        QStringList flist = fontslist.at(k).split(QString(", "), QString::SkipEmptyParts);
        if(flist.size() != 2) continue;
        QStringList flist2 = flist.at(1).split(' ', QString::SkipEmptyParts);
        if(flist2.size() < 2) continue;
        fontsname += flist.at(0);
        fontsname += QString(" ") + flist2.at(0) + flist2.at(1);
        QFont font = QFont(flist.at(0));
        if(flist2.at(1) == "pt") font.setPointSize(flist2.at(0).toInt());
        else font.setPixelSize(flist2.at(0).toInt());
        font.setStyleStrategy(QFont::NoAntialias);
        packer.border = border = false;
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
            else if(flist2.at(i) == "border")
            {
                border = true;
                packer.border = true;
            }

        }
        fontsnames << fontsname;
        QFontMetrics fm(font);
        //rendering glyphs
        for (i = 0; i < s.size(); i++)
        {
            //rc = fm.tightBoundingRect(s.at(i));
            w = fm.size(Qt::TextSingleLine, s.at(i)).width();
            h = fm.height();
            base = fm.ascent();
            packedImage pi;
            pi.img = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
            pi.crop = QRect(0,0,w,h);
            pi.border = true;
            pi.ch = s.at(i);
            list << pi;
            listptr << &list.last();
            //qDebug("%d %d", w,h);
            QPainter p(&list.last().img);
            p.setFont(font);
            p.fillRect(0,0,w,h,Qt::black);
            p.setPen(Qt::white);
            p.drawText(0,base,s.at(i));
            //list.last().save(s.at(i) + QString(".bmp"),"bmp");
            //emit renderedImage(list.last());
            //this->msleep(100);
        }
    }
    QList<QPoint> points;
    uint width = ui->textureW->value(), height = ui->textureH->value();
    packer.ltr = ui->checkLTR->isChecked();
    qDebug("bol %d", packer.ltr);
    points = packer.pack(&list, ui->comboMethod->currentIndex(), ui->comboHeuristic->currentIndex(), width, height);
    QImage texture(width, height, QImage::Format_ARGB32_Premultiplied);
    QPainter p(&texture);
    p.fillRect(QRect(0,0,width,height),Qt::black);
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("Windows-1251"));
    FILE *file = fopen("font.fnt", "wb");
    fprintf(file, "textures: %s\n", "font.png");
    for (i = 0, j = 0; i < list.size(); i++)
    {
        if(i % s.size() == 0)
        {
            fprintf(file, "\n%s\n", qPrintable(fontsnames.at(j)));
            j++;
        }
        p.drawImage(QPoint(list.at(i).rc.x(), list.at(i).rc.y()), list.at(i).img);
        //             char    x    y   cw   ch   cx   cy    w    h
        fprintf(file, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
                        (unsigned char)list.at(i).ch.toAscii(),
                        list.at(i).rc.x(),
                        list.at(i).rc.y(),
                        list.at(i).crop.width(),
                        list.at(i).crop.height(),
                        list.at(i).crop.x(),
                        list.at(i).crop.y(),
                        list.at(i).rc.width(),
                        list.at(i).rc.height()
                        );
    }
    fclose(file);
    int percent = (int)(((float)packer.area / (float)width / (float)height) * 100.0f + 0.5f);
    float percent2 = (float)(((float)packer.neededArea / (float)width / (float)height) * 100.0f );
    ui->preview->setText(QString("Preview: ") +
         QString::number(percent) + QString("% filled, ") +
         QString::number(packer.missingChars) + QString(" chars missed, ") +
         QString::number(packer.mergedChars) + QString(" chars merged, needed area: ") +
         QString::number(percent2) + QString("%."));
    if(packer.missingChars == 0) done = true;
    texture.save("font.png", "PNG");

    emit renderedImage(texture);
}
FontRender::~FontRender()
{

}
