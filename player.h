#ifndef PLAYER_H
#define PLAYER_H

#include <QList>
#include <QString>
#include <QSqlDatabase>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QTableView>
#include <QtSql>
#include "gamedefs.h"

using namespace Game;
class Player
{
public:
    Player();
    void setPlayerData(PlayerData playerData);
    void setStats();

    bool isActive();
    void setActive(bool active);
    bool isCurrent();
    void setCurrent(bool current);
    bool isPlayer();
    void setPlayer(bool player);
    bool  isButton();
    void setButton(bool button);
    quint8 number();
    void setNumber(quint8 number);
    quint8 seatNumber();
    void setSeatNumber(quint8 seatNumber);
    QString name();
    void setName(QString name);
    qreal stack();
    void setStack(qreal stack);
    qreal bet();
    void setBet(qreal bet);
    Position position();
    void setPosition(Position position);
    QList<Card> cards();
    void addCard(Card card);
    void setCard(Card card, quint8 number);
    void setCards(QList<Card> cards);

    QList<Action> actions(Stage stage);
    //int actionsSize();
    int actionStageSize();
    void setAction(Stage stage, Action action, qreal bet, int circle);
    void addAction(Stage stage, Action action, qreal bet);
    QList<qreal> bets(Stage stage);
    struct Stats {
        int hands;
        qreal vpip;
        qreal pfr;
        qreal ats;
        qreal fsbtos;
        qreal fbbtos;
        qreal fcbetf;
        qreal orEP;
        qreal orMP;
        qreal orCO;
        qreal orBU;
        qreal orSB;
    } m_stats;

private:
//    enum Action
//    {
//        NoAction,
//        Fold,
//        Check,
//        Call,
//        Raise,
//        BlindSteal,
//        BlindDefense
//    };

    struct StageActions {
        QList<Action> actions;
        QList<qreal> bets;
    };

    bool m_active;
    bool m_current;
    bool m_player;
    bool m_button;
    quint8 m_number; // номер игрока, начиная с того, кто открывает торги
    quint8 m_seatNumber; // номер места за столом (отсчитывается по часовой стрелке справа от банка)
    QString m_name;
    qreal m_stack;
    qreal m_bet;
    Position m_position;
    QList<Card> m_cards;
    QList<StageActions> m_stageActions;   
};

#endif // PLAYER_H
