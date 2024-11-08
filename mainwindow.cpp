#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->lbTarget->installEventFilter(this);
    mousePressed= false;

    engine = new GameEngine(this);
    qRegisterMetaType< QMap<QString, QString> >();
    connect(engine,SIGNAL(gameDataReady(QMap<QString, QString>)),
            this,SLOT(updateGameData(QMap<QString, QString>)));
    connect(ui->checkBoxGodMode,SIGNAL(toggled(bool)),
            engine,SLOT(godModeStateChanged(bool)));
    emit ui->checkBoxGodMode->toggled(ui->checkBoxGodMode->isChecked());
    emit ui->cbTableSize->currentIndexChanged(ui->cbTableSize->currentText());
}

MainWindow::~MainWindow()
{
    delete engine;
    delete ui;
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    if(event->type()==QEvent::MouseButtonPress) {
        setCursor(QPixmap(":/target.png"));
        mousePressed = true;
        return false;
    }
    else
        return QObject::eventFilter(obj, event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (mousePressed) {
        mousePressed = false;
        unsetCursor();
        ui->pbStartStop->setEnabled(engine->setGameWindow(event->globalPos()));
    }
}

void MainWindow::on_pbStartStop_toggled(bool checked)
{
    if (checked) {
        if (engine->startEngine(ui->sbPosition->value(), ui->dsbBigBlind->value()))
        {
            ui->checkBoxGodMode->toggled(ui->checkBoxGodMode->isChecked());
            ui->cbTableSize->currentIndexChanged(ui->cbTableSize->currentText());
            ui->pbStartStop->setText("Stop");
            ui->pbOpenScreenshot->setDisabled(false);
        }
        else
        {
            ui->pbStartStop->setChecked(false);
        }
    } else {
        engine->stopEngine();
        ui->cbTableSize->currentIndexChanged(ui->cbTableSize->currentText());
        ui->pbStartStop->setText("Start");
        ui->pbOpenScreenshot->setDisabled(true);
    }
}

void MainWindow::updateGameData(QMap<QString, QString> data)
{
    ui->tbTotalPot->setText(data.take("totalPot"));
    ui->tbTableCards->setText(data.take("tableCards"));
    ui->tbMyCards->setText(data.take("myCards"));
    ui->tbRecommendation->setText(data.take("recommend"));
    ui->tbStage->setText(data.take("stage"));
    ui->tbCircle->setText(data.take("circle"));
    ui->tbButtonNumber->setText(data.take("buttonNumber"));
    ui->tbMyNumber->setText(data.take("myNumber"));

    ui->twPlayers->clear();
    QList<QString> players;
    quint16 size = data.size();
    for(int i=0; i<size; i++)
        players.append(data.take("player"+QString::number(i)));
    ui->twPlayers->setRowCount(players.count());
    ui->twPlayers->setColumnCount(6);
    ui->twPlayers->setHorizontalHeaderItem(0, new QTableWidgetItem("Chair"));
    ui->twPlayers->setHorizontalHeaderItem(1, new QTableWidgetItem("Bet"));
    ui->twPlayers->setHorizontalHeaderItem(2, new QTableWidgetItem("BU"));
    ui->twPlayers->setHorizontalHeaderItem(3, new QTableWidgetItem("Current?"));
    ui->twPlayers->setHorizontalHeaderItem(4, new QTableWidgetItem("Stack"));
    ui->twPlayers->setHorizontalHeaderItem(5, new QTableWidgetItem("Name"));
    for (int i=0; i<players.count(); i++) {
        QStringList playerData = players.at(i).split(" ");
        for (int j=0; j<playerData.count(); j++)
            ui->twPlayers->setItem(i, j, new QTableWidgetItem(playerData.at(j)));
    }

    QList<QString> strategyPlayers;
    size = data.size();
    for(int i=0; i<size; i++)
        strategyPlayers.append(data.take("strategyPlayer"+QString::number(i)));
    ui->twStrategyPlayers->setRowCount(strategyPlayers.size());
    ui->twStrategyPlayers->setColumnCount(5);
    ui->twStrategyPlayers->setHorizontalHeaderItem(0, new QTableWidgetItem("Position"));
    ui->twStrategyPlayers->setHorizontalHeaderItem(1, new QTableWidgetItem("Bet"));
    ui->twStrategyPlayers->setHorizontalHeaderItem(2, new QTableWidgetItem("Stack"));
    ui->twStrategyPlayers->setHorizontalHeaderItem(3, new QTableWidgetItem("Chair"));
    ui->twStrategyPlayers->setHorizontalHeaderItem(4, new QTableWidgetItem("Actions"));
    for(int i=0; i<strategyPlayers.size(); i++) {
        QStringList strategyPlayerData = strategyPlayers.at(i).split(";");
        QStringList strategyPlayerMainData = strategyPlayerData.at(0).split(" ");
        for (int j=0; j<strategyPlayerMainData.count(); j++)
            ui->twStrategyPlayers->setItem(i, j, new QTableWidgetItem(strategyPlayerMainData.at(j)));

        if(strategyPlayerData.size()>1)
             ui->twStrategyPlayers->setItem(i, 4, new QTableWidgetItem(strategyPlayerData.at(1)));
    }
}

void MainWindow::on_pbSaveScreenshot_clicked()
{
    emit saveScreenShot();
}

void MainWindow::on_cbTableSize_currentIndexChanged(const QString &arg1)
{
    emit changeTableSize(arg1);
}

void MainWindow::on_pbOpenScreenshot_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open Image"),"",
                                                    tr("Images (*.png)"));
    if (!fileName.isEmpty())
    {
        QImage image;
        if (!image.load(fileName, "PNG"))
        {
            QMessageBox::warning(this, tr("Error"),
                                 tr("Cannot read file %1:\n")
                                 .arg(fileName)
                                 );
            return;
        }
        engine->checkImage(image, ui->sbPosition->value());
    }
}
