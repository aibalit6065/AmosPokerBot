#ifndef GAMESTATUS_H
#define GAMESTATUS_H

#include <QObject>
#include <QDebug>
#include <QString>
#include <QList>
#include <QMap>
#include <QDir>
#include <QFile>
#include "player.h"
#include "gamedefs.h"
#include "strategy.h"

class GameStatus// : public QObject
{
//    Q_OBJECT

public:
    GameStatus();
    Recommendation analyzeGame(Table *table);
    int     sortCards(QList<Card> *, int); // сортировка карт
    ~GameStatus();
    QMap<QString, QString> getStrategyData();


signals:
    void saveScreenShot();

private:
    QList<Player> m_players;        //список игроков, составляется в конструкторе
    Stage       m_stage;            //стадия игры
    int         m_circle;           //круг на текущей стадии игры
    int         m_starterNumber;    //номер игрока, открывающего торги на текущей стадии в списке игроков
    int         m_currentNumber;    //номер текущего игрока в списке игроков
    int         m_myNumber;         //номер нашего игрока в списке игроков
    int         m_buttonSeatNumber; //номер места за столом, на котором находится диллер
    qreal       m_bet;              //текущая ставка
    qreal       m_stackMin;         //минимальное количество BB в нашем стэке для докупки
    qreal       m_stackRebuy;       //количество BB в стэке для входа в игру и при докупке
    qreal       m_stackMax;         //максимальное количество BB в нашем стэке для выхода из-за стола

    QList<Card> m_cards;            //список всех карт, включающий собственные карты и карты, лежащие на столе
    Combination m_combination;      //текущая комбинация
    Card        m_pairCard;         //номинал пары
    Card        m_kickPair;         //кикер для пары
    QList<Card> m_outs;             //карты аутсов

    bool        m_sitOut;           //флаг выхода из-за стола
    bool        m_sitOutNextBB;     //флаг выхода из-за стола на следующем большом блайнде
    bool        m_rebuy;            //флаг докупки

    Strategy   *strategy;
//    QFile       *m_file;             //имя файла с историей
//    void    addLog(QString text);   //добавить запись в файл с логами

    int     getButtonSeatNumber(QList<PlayerData> playersData); //возвращает номер места за столом, где сидит диллер
    int     getMyNumber(Table *table);  //определяет наш номер в списке игроков
    int     getCurrentNumber(QList<Player> players); //определяет номер текущего игрока
    GameMode gameMode(Table *table); //определяет режим, в зависимоть от наличия карт у нас
    Stage   checkStage(Table *table); //проверка стадии игры
    int     setPlayers(Table *table); //составляет список m_players из данных в table->playersData
    void    updatePlayers(Table *table); //обновляет поля в списке m_players согласно table->playersData
    int     assignPlayerPositions(QList<Player> *players); //выставление позиций игрокам (все игроки активны)
    int     assignPlayerActions(Table *table); //присвоение действий игрокам
    int     assignLostPlayerActions(Stage stage, Table *table); //добавление действий игрокам, которые не были обработаны при переходе на новую стадию (начиная с флопа)

    Combination    checkOnePair(QList<Card>, QList<Card>, QList<Card>*);// проверка на пару
    bool    checkTwoPair(QList<Card>,QList<Card>);    // проверка на две пары
    Combination    checkTrips(QList<Card>,QList<Card>);        // проверка на 3 карты (сет либо трипс)
    bool    checkStraight(QList<Card>,QList<Card>*);   // проверка на стрэйт
    bool    checkFlush(QList<Card>,QList<Card>*);      // проверка на флэш
    bool    checkFullHouse(QList<Card>);  // проверка на фул хаус
    bool    checkQuads(QList<Card>);      // проверка на карэ (4 карты)
    bool    checkStraightFlush(QList<Card>);// проверка на стрит флеш
    bool    checkRoyalFlush(QList<Card>); // проверка на флеш рояль
    bool    checkFlopDraw(QList<Card>); // проверка дро на флопе
    bool    checkFlopToTurnDraw(QList<Card>); // закрылось ли дро от флопа к терну

    void    checkCombination(QList<Card>,QList<Card>);// определение комбинации и получение списка аутсов
    void    testCombination();// тестирование функции проверки комбинации
};

#endif // GAMESTATUS_H
