#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>
#include <QMap>
#include "gameengine.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    bool eventFilter(QObject* obj, QEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);

private slots:
    void on_pbStartStop_toggled(bool checked);
    void updateGameData(QMap<QString, QString> data);
    void on_pbSaveScreenshot_clicked();
    void on_cbTableSize_currentIndexChanged(const QString &arg1);
    void on_pbOpenScreenshot_clicked();

signals:
    void saveScreenShot();
    void changeTableSize(QString tableSize);

private:
    Ui::MainWindow *ui;
    GameEngine *engine;
    bool mousePressed;
};

#endif // MAINWINDOW_H
