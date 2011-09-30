#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWhatsThis>
#include <QTextCodec>
#include <QFileDialog>
#include <QSettings>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    thread = new FontRender(ui);
    thread->exporting = false;
    connect(ui->pushButton_2, SIGNAL(clicked()), thread, SLOT(run()));
    qRegisterMetaType<QImage>("QImage");
    connect(thread, SIGNAL(renderedImage(QImage)), ui->widget, SLOT(updatePixmap(QImage)));
    //ui->pushButton_2->click();
    QList<QByteArray> avaiableCodecs = QTextCodec::availableCodecs ();
    int i;
    for(i = 0; i < avaiableCodecs.count(); i++)
    {
        ui->encoding->addItem(avaiableCodecs.at(i));
    }
    ui->outDir->setText(QDir::homePath());
    readSettings();
    thread->run();
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

void MainWindow::addFont()
{
    QString s = ui->fontComboBox->currentFont().family() +
                QString(", ") +
                ui->spinFontSize->text() + QString(" ") +
                ui->comboPtPx->currentText() +
                (ui->checkFontSmoothing->isChecked()?QString(" smooth"):"") +
                (ui->checkFontBold->isChecked()?QString(" b"):"") +
                (ui->checkFontItalic->isChecked()?QString(" i"):"");
    QListWidgetItem *item = new QListWidgetItem(ui->listOfFonts);
    item->setText(s);
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
}

void MainWindow::removeFont()
{
    ui->listOfFonts->takeItem(ui->listOfFonts->row(ui->listOfFonts->currentItem()));
}

void MainWindow::getFolder()
{
    ui->outDir->setText(QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                 ui->outDir->text(),
                                                 QFileDialog::ShowDirsOnly));
}

void MainWindow::exportFont()
{
    thread->exporting = true;
    homeDir = ui->outDir->text();
    outFile = ui->outFile->text();
    thread->run();
}
void MainWindow::bruteForce()
{
    int i, j, k;
    ui->bruteForce->setText("Please, wait...");
    for(i = 0; i < ui->comboMethod->count(); i++)
    {
        for(j = 0; j < ui->comboHeuristic->count(); j++)
        {
            for(k = 0; k < ui->sortOrder->count(); k++)
            {
                ui->comboMethod->setCurrentIndex(i);
                ui->sortOrder->setCurrentIndex(k);
                ui->comboHeuristic->setCurrentIndex(j);
                thread->run();
                //~ thread->wait();
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

void MainWindow::readSettings()
{
    QSettings settings("Sciprum Plus", "UBFG");

    settings.beginGroup("MainWindow");
    resize(settings.value("size", QSize(800, 600)).toSize());
    move(settings.value("pos", QPoint(200, 200)).toPoint());
    projectDir = settings.value("projectDir", QDir::homePath()).toString();
    homeDir = settings.value("homeDir", QDir::homePath()).toString();
    outFile = settings.value("outFile", ui->outFile->text()).toString();
    project = settings.value("project", "project.bfg").toString();
    settings.endGroup();
}

void MainWindow::writeSettings()
{
    QSettings settings("Sciprum Plus", "UBFG");

    settings.beginGroup("MainWindow");
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.setValue("projectDir", projectDir);
    settings.setValue("homeDir", homeDir);
    settings.setValue("outFile", outFile);
    settings.endGroup();
}

void MainWindow::loadProject()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Open Project"),
                                                 projectDir,
                                                 tr("Projects (*.bfg)"));
    if(file.length())
    {
        QFileInfo fi(file);
        projectDir = fi.path();
        project = fi.fileName();
        QSettings settings(file, QSettings::IniFormat, this);
        
        ui->plainTextEdit->setPlainText(settings.value("charList", " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-+\\/():;%&`'*#$=[]@^{}_~\"><").toString());
        ui->trim->setChecked(settings.value("trim", true).toBool());
        ui->borderTop->setValue(settings.value("borderTop", 0).toInt());
        ui->borderBottom->setValue(settings.value("borderBottom", 1).toInt());
        ui->borderLeft->setValue(settings.value("borderLeft", 0).toInt());
        ui->borderRight->setValue(settings.value("borderRight", 1).toInt());
        ui->merge->setChecked(settings.value("merge", true).toBool());
        ui->mergeBF->setChecked(settings.value("mergeBF", true).toBool());
        ui->textureW->setValue(settings.value("textureW", 512).toInt());
        ui->textureH->setValue(settings.value("textureH", 512).toInt());
        ui->comboMethod->setCurrentIndex(settings.value("method", 1).toInt());
        ui->comboHeuristic->setCurrentIndex(settings.value("heuristic", 1).toInt());
        ui->sortOrder->setCurrentIndex(settings.value("sortOrder", 2).toInt());
        ui->outFormat->setCurrentIndex(settings.value("outFormat", 0).toInt());
        ui->encoding->setCurrentIndex(settings.value("encoding", 0).toInt());
        ui->transparent->setChecked(settings.value("transparent", true).toBool());
        ui->outDir->setText(settings.value("outDir", homeDir).toString());
        ui->outFile->setText(settings.value("outFile", outFile).toString());
        int size = settings.beginReadArray("fonts");
        ui->listOfFonts->clear();
        for (int i = 0; i < size; ++i) {
            settings.setArrayIndex(i);
            QListWidgetItem *item = new QListWidgetItem(ui->listOfFonts);
            item->setText(settings.value("font").toString());
            item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        }
        settings.endArray();
    }
}

void MainWindow::saveProject()
{
    QString file = QFileDialog::getSaveFileName(this, tr("Save Project"),
                                                 projectDir+"/"+project,
                                                 tr("Projects (*.bfg)"));
    if(file.length())
    {
        QFileInfo fi(file);
        projectDir = fi.path();
        project = fi.fileName();
        QSettings settings(file, QSettings::IniFormat, this);
        
        settings.setValue("charList", ui->plainTextEdit->toPlainText());
        settings.setValue("trim", ui->trim->isChecked());
        settings.setValue("borderTop", ui->borderTop->value());
        settings.setValue("borderBottom", ui->borderBottom->value());
        settings.setValue("borderLeft", ui->borderLeft->value());
        settings.setValue("borderRight", ui->borderRight->value());
        settings.setValue("merge", ui->merge->isChecked());
        settings.setValue("mergeBF", ui->mergeBF->isChecked());
        settings.setValue("textureW", ui->textureW->value());
        settings.setValue("textureH", ui->textureH->value());
        settings.setValue("method", ui->comboMethod->currentIndex());
        settings.setValue("heuristic", ui->comboHeuristic->currentIndex());
        settings.setValue("sortOrder", ui->sortOrder->currentIndex());
        settings.setValue("outFormat", ui->outFormat->currentIndex());
        settings.setValue("encoding", ui->encoding->currentIndex());
        settings.setValue("transparent", ui->transparent->isChecked());
        settings.setValue("outDir", ui->outDir->text());
        settings.setValue("outFile", ui->outFile->text());
        settings.beginWriteArray("fonts");
        for (int i = 0; i < ui->listOfFonts->count(); ++i) {
            settings.setArrayIndex(i);
            settings.setValue("font", ui->listOfFonts->item(i)->text());
        }
        settings.endArray();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    event->accept();
}