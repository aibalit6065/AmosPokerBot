#include "gameengine.h"
#include <QDateTime>
#include <QTime>
#include <QDebug>

//#define DEBUG_IMAGE

static const quint16 TIMER_PERIOD = 100;
static const QString CLIENT_WINDOW_NAME = "Hold'em";

void GameEngine::initFileds()
{
    abort = false;
    godMode = false;
    moveFlag = false;

    table = new Table();
    api = new SystemApi();
    imageProcess = new ImageProcess();
    gameStatus = new GameStatus();
//    strategy = new Strategy();


//    connect(gameStatus,SIGNAL(saveScreenShot()),
//            api,SLOT(saveLastScreenShot()));
//    connect(strategy,SIGNAL(saveScreenShot()),
//            api,SLOT(saveLastScreenShot()));
    connect(this->parent(),SIGNAL(saveScreenShot()),
            api,SLOT(saveLastScreenShot()));
    connect(this->parent(),SIGNAL(changeTableSize(QString)),
            imageProcess,SLOT(tableSizeChanged(QString)));
}

void GameEngine::destroyFields()
{
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();
    wait();

//    delete strategy;
    delete gameStatus;
    delete imageProcess;
    delete api;
    delete table;
}

GameEngine::GameEngine(QObject *parent)
    : QThread(parent)
{
    initFileds();
}

GameEngine::~GameEngine()
{
    destroyFields();
}

bool GameEngine::setGameWindow(QPoint point)
{
    bool result = false;

    if (!point.isNull() && !isRunning()) {
        SystemApi::WindowHandle handle = api->ancestorHandle(api->handleFromPoint(point));
        if (handle) {
            //QString windowName = api->windowText(handle);
            //if (windowName.contains(CLIENT_WINDOW_NAME)) {
                api->setHandle(handle);
                result = true;
            //}
        }
    }
    return result;
}

void GameEngine::checkImage(QImage image, int heroChair)
{
    table->playerSeatNumber = heroChair;
    imageProcess->processImage(image, table);
    sendData();
}

bool GameEngine::startEngine(quint8 playerSeatNumber, qreal bigBlind)
{
    QMutexLocker locker(&mutex);
    bool result = false;

    if (api->handle()) {
        result = true;
        table->bigBlind = bigBlind;
        table->playerSeatNumber = playerSeatNumber;
        if (!isRunning()) {
            start(LowPriority);

            m_dir = QDir::current();
            if (!m_dir.mkdir("history"))
                qDebug()<<tr("directory \"/history\" already exists");
            m_dir.cd("history");
            QDir::setCurrent(m_dir.path());
            QString filename=QDateTime::currentDateTime().toString("yyyy.MM.dd_hh-mm-ss")+".txt";
            m_file.setFileName(filename);
            if(!m_file.open(QIODevice::WriteOnly|QIODevice::Text))
                qDebug()<<tr("History file could not be open...");
        }
    }

    return result;
}

void GameEngine::stopEngine()
{
    if (m_file.isOpen()) {
        m_file.close();
        m_dir.cdUp();
        QDir::setCurrent(m_dir.path());
    }

    TableSize tableSize = imageProcess->tableSize();
    SystemApi::WindowHandle handle = api->handle();
    destroyFields();
    initFileds();
    imageProcess->setTableSize(tableSize);
    api->setHandle(handle);
}

void GameEngine::run()
{
    mutex.lock();
    if (!api->handle()) {
        abort = true;
    }
    mutex.unlock();

#ifdef DEBUG_IMAGE
    //Цикл по изоражениям в директории
    QDir dir("C:/Projects/images");
    QStringList files = dir.entryList(QDir::Files);
    qint32 k = 0;
#endif

    forever {
        mutex.lock();
        if (abort) {
            abort = false;
            mutex.unlock();
            return;
        }

        recommend.recAct = NoAction;
        QImage image = api->takeScreenShot();
        mutex.unlock();

#ifdef DEBUG_IMAGE
        //Цикл по изоражениям в директории
        if (k >= files.size())
            k = 0;
        qDebug() << dir.path() << files.at(k);
        image.load(dir.path() + "/" + files.at(k));
        k++;
#endif

        if(image!=gameImage)
        {
            gameImage = image;
            if (!gameImage.isNull()) {
                imageProcess->processImage(gameImage, table);
                if(table->valid)
                {
//                    gameStatus->analyzeGame(table);
                    //recommend = strategy->makeDecision(gameStatus, table, &m_file);
                    if (prevRecom.recAct != recommend.recAct) {
                        prevRecom = recommend;
                        moveFlag = false;
                    }
//                    strategyData = strategy->getStrategyData();
                }
                else
                {
                    recommend.recAct = ClosePopupWindow;
                    if (prevRecom.recAct != recommend.recAct) {
                        prevRecom = recommend;
                        moveFlag = false;
                    }
                }
            }

            if (godMode && !moveFlag) {
                mutex.lock();
                switch(recommend.recAct) {
                case Fold:
                    api->randMoveMouse(table->gameButtons[FoldButton]);
                    api->clickMouse();
                    break;
                case Check:
                case Call:
                    api->randMoveMouse(table->gameButtons[CallButton]);
                    api->clickMouse();
                    break;
                case Raise:
                case BlindSteal:
                case BlindDefense:
                    api->randMoveMouse(table->betInput);
                    api->clickMouse();
                    api->typeText(QString("%1").arg(recommend.value));
                    api->randMoveMouse(table->gameButtons[RaiseButton]);
                    api->clickMouse();
                    break;
                case SitOut:
                    api->randMoveMouse(table->optionsTab);
                    api->clickMouse();
                    api->randMoveMouse(table->sitOutCheckbox);
                    api->clickMouse();
                    break;
                case SitOutNextBB:
                    api->randMoveMouse(table->optionsTab);
                    api->clickMouse();
                    api->randMoveMouse(table->sitOutNextBBcheckbox);
                    api->clickMouse();
                    break;
                case Rebuy:
                    api->randMoveMouse(table->rebuyTab);
                    api->clickMouse();
                    api->randMoveMouse(table->rebuyInput);
                    api->clickMouse();
                    api->typeText(QString("%1").arg(recommend.value));
                    api->randMoveMouse(table->rebuyButton);
                    api->clickMouse();
                    break;
                case ClosePopupWindow:
                    api->randMoveMouse(table->popupWindowButton);
                    api->clickMouse();
                    break;
                case NoAction:
                default:
                    break;
                }
                mutex.unlock();
                moveFlag = true;
            }
            sendData();
        }

        mutex.lock();
        condition.wait(&mutex, TIMER_PERIOD);
        mutex.unlock();
    }
}

void GameEngine::sendData()
{
    QMap<QString,QString> exportData;

    exportData.insert("totalPot",QString::number(table->totalPot));

    switch(recommend.recAct) {
    case NoAction:
        exportData.insert("recommend", "Wait");
        break;
    case PostBlind:
        exportData.insert("recommend", "Post blind");
        break;
    case WaitForBB:
        exportData.insert("recommend", "Wait for big blind");
        break;
    case Fold:
        exportData.insert("recommend", "Fold");
        break;
    case Check:
        exportData.insert("recommend", "Check");
        break;
    case Call:
        exportData.insert("recommend", "Call");
        break;
    case Raise:
        exportData.insert("recommend", QString("Raise%1").arg(recommend.value));
        break;
    case BlindSteal:
        exportData.insert("recommend", QString("BlindSteal%1").arg(recommend.value));
        break;
    case BlindDefense:
        exportData.insert("recommend", QString("BlindDefense%1").arg(recommend.value));
        break;
    case SitOut:
        exportData.insert("recommend", "SitOut");
        break;
    case SitOutNextBB:
        exportData.insert("recommend", "SitOutNextBB");
        break;
    case Rebuy:
        exportData.insert("recommend", QString("Rebuy %1").arg(recommend.value));
        break;
    case ClosePopupWindow:
        exportData.insert("recommend", "Close popup window");
        break;
    case CloseTable:
        exportData.insert("recommend", "Close table");
        break;
    }

    QString tableCards;
    for (int i=0; i<table->cards.count();i++) {
        tableCards.append((QChar)table->cards[i].suit);
        tableCards.append(QString::number(table->cards[i].nominal));
        tableCards.append(" ");
    }
    exportData.insert("tableCards", tableCards);

    QString myCards;
    quint8 myPlayerNumber = 255;
    for (int i=0;i<table->playersData.count();i++) {
        if (table->playersData[i].seatNumber == table->playerSeatNumber)
            myPlayerNumber = i;
    }
    if (myPlayerNumber < 250) {
        for (int i=0;i<table->playersData[myPlayerNumber].cards.count();i++) {
            myCards.append((QChar)table->playersData[myPlayerNumber].cards[i].suit);
            myCards.append(QString::number(table->playersData[myPlayerNumber].cards[i].nominal));
            myCards.append(" ");
        }
    }
    exportData.insert("myCards", myCards);

    for (int i=0; i<table->playersData.count();i++) {
            QString player;
            player.append(QString::number(table->playersData[i].seatNumber)+" ");
            player.append(QString::number(table->playersData[i].bet)+" ");
            player.append(QString::number((char)table->playersData[i].button)+" ");
            player.append(QString::number((char)table->playersData[i].current)+" ");
            player.append(QString::number(table->playersData[i].stack)+" ");
            player.append(table->playersData[i].name);
            exportData.insert("player"+QString::number(i), player);
    }

    //qDebug()<<"before export";
    //QList<QString> keys = strategyData.keys();
    //for (int i=0; i<keys.count();i++) {
    //    qDebug()<<"keys["<<i<<"]="<<keys[i]<<"strategyData.value(keys[i])"<<strategyData.value(keys[i]);
    //    exportData.insert(keys[i],strategyData.value(keys[i]));
    //}

    emit gameDataReady(exportData);
}

void GameEngine::godModeStateChanged(bool state)
{
    godMode = state;
}
