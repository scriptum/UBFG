#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWhatsThis>
#include <QTextCodec>
#include <QFileDialog>
#include <QSettings>
#include <QColorDialog>
#include <QFontDatabase>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QFontDatabase fdb;
    QDir currentDir;
    QStringList filters;
    filters << "*.ttf" << "*.TTF";
    currentDir.setNameFilters(filters);
    QStringList entryList = currentDir.entryList();
    for(int i = 0; i < entryList.size(); ++i)
    {
        fdb.addApplicationFont(entryList.at(i));
    }
    ui->setupUi(this);
    ui->bruteForce->hide();
    thread = new FontRender(ui);
    thread->exporting = false;
    connect(ui->updateButton, SIGNAL(clicked()), thread, SLOT(run()));
    qRegisterMetaType<QImage>("QImage");
    connect(thread, SIGNAL(renderedImage(QImage)), ui->widget, SLOT(updatePixmap(QImage)));
    ui->encoding->addItem("UNICODE");
    QList<QByteArray> avaiableCodecs = QTextCodec::availableCodecs();
    qSort(avaiableCodecs);
    for(int i = 0; i < avaiableCodecs.count(); i++)
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
        e->accept();
        break;
    default:
        QWidget::changeEvent(e);
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
    ui->bruteForce->setText("Please, wait...");

    thread->run();
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
        ui->distanceField->setChecked(settings.value("distanceField", false).toBool());
        ui->comboHeuristic->setCurrentIndex(settings.value("heuristic", 1).toInt());
        ui->sortOrder->setCurrentIndex(settings.value("sortOrder", 2).toInt());
        ui->outputFormat->setCurrentIndex(settings.value("outFormat", 0).toInt());
        //compatible with old format without UNICODE and with export indexes instead of text
        int encodingInt = settings.value("encoding", 0).toInt();
        QString encodingStr = settings.value("encoding", 0).toString();
        if(QString::number(encodingInt) == encodingStr)
            ui->encoding->setCurrentIndex(encodingInt + 1);
        else
            ui->encoding->setCurrentIndex(ui->encoding->findText(encodingStr));
        ui->transparent->setChecked(settings.value("transparent", true).toBool());
        ui->outDir->setText(settings.value("outDir", homeDir).toString());
        ui->outFile->setText(settings.value("outFile", outFile).toString());
        ui->exportKerning->setChecked(settings.value("kerning", true).toBool());
        ui->saveImageInsideXML->setChecked(settings.value("imageInXML", false).toBool());
        int size = settings.beginReadArray("fonts");
        ui->listOfFonts->clear();
        for (int i = 0; i < size; ++i) {
            settings.setArrayIndex(i);
            QListWidgetItem *item = new QListWidgetItem(ui->listOfFonts);
            item->setText(settings.value("font").toString());
            item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        }
        settings.endArray();
        thread->run();
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
        settings.setValue("distanceField", ui->distanceField->isChecked());
        settings.setValue("heuristic", ui->comboHeuristic->currentIndex());
        settings.setValue("sortOrder", ui->sortOrder->currentIndex());
        settings.setValue("outFormat", ui->outputFormat->currentIndex());
        settings.setValue("encoding", ui->encoding->currentText());
        settings.setValue("transparent", ui->transparent->isChecked());
        settings.setValue("outDir", ui->outDir->text());
        settings.setValue("outFile", ui->outFile->text());
        settings.setValue("kerning", ui->exportKerning->isChecked());
        settings.setValue("imageInXML", ui->saveImageInsideXML->isChecked());
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

void MainWindow::bitDepthChanged(int index) {
    if (index == 1)
        ui->transparent->setDisabled(true);
    else
        ui->transparent->setDisabled(false);
}

void MainWindow::changeFontColor() {
    QPalette buttonPal = ui->fontColor->palette();
    QColor selectedColor = QColorDialog::getColor(buttonPal.brush(QPalette::Button).color(), NULL, "Font Color");
    QBrush brush(selectedColor);
    brush.setStyle(Qt::SolidPattern);
    buttonPal.setBrush(QPalette::Active, QPalette::Button, brush);
    buttonPal.setBrush(QPalette::Inactive, QPalette::Button, brush);
    buttonPal.setBrush(QPalette::Disabled, QPalette::Button, brush);
    ui->fontColor->setPalette(buttonPal);
}

void MainWindow::changeBkgColor() {
    QPalette buttonPal = ui->backgroundColor->palette();
    QColor selectedColor = QColorDialog::getColor(buttonPal.brush(QPalette::Button).color(), NULL, "Font Color");
    QBrush brush(selectedColor);
    brush.setStyle(Qt::SolidPattern);
    buttonPal.setBrush(QPalette::Active, QPalette::Button, brush);
    buttonPal.setBrush(QPalette::Inactive, QPalette::Button, brush);
    buttonPal.setBrush(QPalette::Disabled, QPalette::Button, brush);
    ui->backgroundColor->setPalette(buttonPal);
}
