#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWhatsThis>
#include <QTextCodec>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    thread = new FontRender(ui);
    connect(ui->pushButton_2, SIGNAL(clicked()), thread, SLOT(start()));
    qRegisterMetaType<QImage>("QImage");
    connect(thread, SIGNAL(renderedImage(QImage)), ui->widget, SLOT(updatePixmap(QImage)));
    //ui->pushButton_2->click();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
void MainWindow::updateWidth(int w) {
    //ui->widget->setMinimumWidth(w);
    //thread->start();
}
void MainWindow::updateHeight(int h) {
    //ui->widget->setMinimumHeight(h);
    //thread->start();
}
void MainWindow::addFont()
{
    QString s = ui->fontComboBox->currentFont().family() +
                QString(", ") +
                ui->spinFontSize->text() + QString(" ") +
                ui->comboPtPx->currentText() +
                (ui->checkFontSmoothing->isChecked()?QString(" smooth"):"") +
                (ui->checkFontBold->isChecked()?QString(" b"):"") +
                (ui->checkFontItalic->isChecked()?QString(" i"):"") +
                (ui->checkFontPadding->isChecked()?QString(" border"):"");
    ui->listOfFonts->appendPlainText(s);
    QList<QByteArray> list = QTextCodec::availableCodecs();
    int i;
    for(i = 0; i < list.size(); i++)
    {
        qDebug("%d", i);
        ui->encoding->insertItem(i, (QString)list.at(i));
    }
}

void MainWindow::bruteForce()
{
    int i, j, k;
    ui->bruteForce->setText("Please, wait for a minute...");
    for(i = 0; i < ui->comboMethod->count(); i++)
    {
        for(j = 0; j < ui->comboHeuristic->count(); j++)
        {
            for(k = 0; k < ui->sortOrder->count(); k++)
            {
                ui->comboMethod->setCurrentIndex(i);
                ui->sortOrder->setCurrentIndex(k);
                ui->comboHeuristic->setCurrentIndex(j);
                thread->start();
                thread->wait();
                if(thread->done)
                {
                    ui->bruteForce->setText("BRUTE-FORCE");
                    return;
                }
            }
        }
    }
    ui->bruteForce->setText("BRUTE-FORCE");
}
