#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include <QMap>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QDir>
#include <QFile>
#include "gamedefs.h"
#include "imageprocess.h"
#include "systemApi.h"
#include "strategy.h"
#include "gamestatus.h"

class GameEngine : public QThread
{
    Q_OBJECT

public:
    GameEngine(QObject *parent = 0);
    ~GameEngine();

    bool setGameWindow(QPoint point);
    bool startEngine(quint8 playerSeatNumber, qreal bigBlind);
    void stopEngine();
    void checkImage(QImage, int);

public slots:
    void godModeStateChanged(bool state);

signals:
    void gameDataReady(QMap<QString, QString> data);

protected:
    void run();

private:
    QMutex mutex;
    QWaitCondition condition;

    QDir m_dir;
    QFile m_file;

    bool abort;
    bool godMode;
    bool moveFlag;

    QImage gameImage;
    Table *table;
    ImageProcess *imageProcess;
    SystemApi *api;
//    Strategy *strategy;
    GameStatus *gameStatus;
    Recommendation recommend;
    Recommendation prevRecom;
    QMap<QString, QString> strategyData;

    void sendData();
    void initFileds();
    void destroyFields();
};

#endif // GAMEENGINE_H
