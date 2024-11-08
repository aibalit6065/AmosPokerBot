#ifndef STRATEGYMODULE_H
#define STRATEGYMODULE_H

#include <QString>
#include <QList>
#include <QMap>
#include <QDir>
#include <QFile>
#include "player.h"
#include "gamedefs.h"
//#include "gamestatus.h"
#include <QObject>

//class Strategy: public QObject
class Strategy
{
//    Q_OBJECT
public:
    Strategy();
    ~Strategy();
    bool sitOutByPlayers(QList<Player> players, bool alreadySitOut);
    bool sitOutByStack(qreal myStack, bool alreadySitOut);
    Recommendation checkRebuy(qreal myStack, bool alreadyRebuy);
    void initStackParams(qreal BB);
    Recommendation preflopMSSbasic(Table *table);                // анализ действия по чарту MSS
    Recommendation preflopMSSinstaFold();            // анализ по чарту на Fold не дожидаясь очереди
    Recommendation preflopMSSadvancedFR(int circle, Table *table, QList<Player> players, qreal bet, int starterNumber, int myNumber);
    Recommendation postflopMSSbasic(Table *table);
    Recommendation postflopMSSbasic(Stage stage,
                                    int circle,
                                    Table *table,
                                    QList<Player> players,
                                    qreal bet,
                                    int starterNumber,
                                    int myNumber,
                                    Combination combination,
                                    QList<Card> outs,
                                    Card pairCard,
                                    Card kickPair ); // анализ рекомендуемого действия после префлопа (флоп, терн, ривер) MSS
    Recommendation postflopMSSadvanced(Stage stage,
                                       int circle,
                                       Table *table,
                                       QList<Player> players,
                                       qreal bet,
                                       int starterNumber,
                                       int myNumber,
                                       Combination combination,
                                       QList<Card> outs,
                                       Card kickPair );
    QMap<QString,QString> getStrategyData();    //возвращает информацию о текущем состоянии игры и игроков

    qreal       m_stackMin;         //минимальное количество BB в нашем стэке для докупки
    qreal       m_stackRebuy;       //количество BB в стэке для входа в игру и при докупке
    qreal       m_stackMax;         //максимальное количество BB в нашем стэке для выхода из-за стола

//signals:
//    void saveScreenShot();

private:
//    QList<Player> m_players;        //список игроков, составляется в конструкторе
//    Stage       m_stage;            //стадия игры
//    int         m_circle;           //круг на текущей стадии игры
//    int			m_starterNumber;    //номер игрока, открывающего торги на текущей стадии в списке игроков
//    int 	    m_currentNumber;    //номер текущего игрока в списке игроков
//    int         m_myNumber;         //номер нашего игрока в списке игроков
//    int         m_buttonSeatNumber; //номер места за столом, на котором находится диллер
//    qreal	    m_bet;				//текущая ставка

    bool        m_sitOut;           //флаг выхода из-за стола
    bool        m_sitOutNextBB;     //флаг выхода из-за стола на следующем большом блайнде
    bool        m_rebuy;            //флаг докупки

    QFile       *m_file;             //имя файла с историей
    void    addLog(QString text);    //добавить запись в файл с логами

    bool    checkFlopDraw(QList<Card>); // проверка дро на флопе
    bool    checkFlopToTurnDraw(QList<Card>); // закрылось ли дро от флопа к терну

    QString combinationToString(Combination combination);
};

#endif // STRATEGYMODULE_H
