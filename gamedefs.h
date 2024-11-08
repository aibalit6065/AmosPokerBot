#ifndef GAMEDEFS_H
#define GAMEDEFS_H

#include <QList>
#include <QString>
#include <QRect>

namespace Game
{
    enum Action
    {
        NoAction,
        Fold,
        Check,
        Call,
        Raise,
        BlindSteal,
        BlindDefense,
        SitOut,
        SitOutNextBB,
        Rebuy,
        ClosePopupWindow,
        CloseTable,
        WaitForBB,
        PostBlind
    };

    enum GameMode
    {
        NoMode,
        StartMode,                //режим до получения карт
        PlayMode                    //режим после получения карт
    };

    enum Stage
    {
        NoStage = 0,
        PreFlop = 1,
        Flop = 2,
        Turn = 3,
        River = 4
    };

    enum Position
    {
        Pos_None = 0,
        Pos_UTG = 1,
        Pos_MP = 2,
        Pos_CO = 3,
        Pos_BU = 4,
        Pos_SB = 5,
        Pos_BB = 6
    };

    enum Combination
    {
        NoCombination,
        MiddlePair,     //Средняя пара
        TopPair,        //Топ пара
        OverPair,       //Овер пара
        TwoPair,        //Две пары
        Trips,          //Трипс (3 карты) - участвует только одна наша карта
        Set,            //Сет (3 карты) - участвуют обе наши карты
        Straight,       //Стрит
        Flush,          //Флеш
        FullHouse,      //Фул хаус
        FourOfKind,     //Каре
        StraightFlush,  //Стрит флеш
        RoyalFlush      //Роял флеш
    };

    enum ButtonType
    {
        FoldButton  = 0,
        CallButton  = 1,
        RaiseButton = 2
    };

    struct Card
    {
        enum Suits
        {
            NoSuit,
            Spades = 'S',         //Пики
            Hearts = 'H',         //Черви
            Clubs = 'C',          //Крести (трефы)
            Diamonds = 'D'        //Бубны
        };
        enum Nominals
        {
            NoNominal = 0,
            Two   = 2,
            Three = 3,
            Four  = 4,
            Five  = 5,
            Six   = 6,
            Seven = 7,
            Eight = 8,
            Nine  = 9,
            Ten   = 10,
            Jack  = 11,
            Queen = 12,
            King  = 13,
            Ace   = 14
        };
        Nominals nominal;
        Suits    suit;
    };

    struct PlayerData
    {
        bool current;
        bool button;
        quint8 seatNumber;
        QString name;
        qreal stack;
        qreal bet;
        QList<Card> cards;
    };

    struct Table
    {
        bool scaleChanged;
		bool valid;
        qreal totalPot;
        qreal bigBlind;
        qreal bet;
        quint8 playerSeatNumber;
        QList<Card> cards;
        QList<PlayerData> playersData;
        QList<QRect> gameButtons;
        QRect betInput;
        QRect optionsTab;
        QRect chatTab;
        QRect rebuyTab;
        QRect rebuyInput;
        QRect rebuyButton;
        QRect sitOutNextBBcheckbox;
        QRect sitOutCheckbox;
        QRect popupWindowButton;
        QRect popupWindowPart;
    };

    struct Recommendation
    {
        Action recAct;
        qreal value;
    };
}

#endif // GAMEDEFS_H
