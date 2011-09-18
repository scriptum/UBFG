#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "fontrender.h"
namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::MainWindow *ui;
    FontRender *thread;
private slots:
    void updateWidth(int w);
    void updateHeight(int h);
    void addFont();
    void bruteForce();
};

#endif // MAINWINDOW_H
