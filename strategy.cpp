#include <QDateTime>
#include "strategy.h"
//#include "gamestatus.h"
#include <QDebug>
#include <math.h>
#include <QSqlDatabase>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QTableView>
#include <QtSql>


QString Strategy::combinationToString(Combination combination)
{
    QString out;

    switch(combination){
    case NoCombination: out="NoCombination";  break;
    case MiddlePair: 	out="MiddlePair";     break;
    case TopPair: 		out="TopPair";        break;
    case OverPair: 		out="OverPair";       break;
    case TwoPair: 		out="TwoPair";        break;
    case Trips: 		out="Trips";          break;
    case Set: 			out="Set";            break;
    case Straight: 		out="Straight";       break;
    case Flush: 		out="Flush";          break;
    case FullHouse: 	out="FullHouse";      break;
    case FourOfKind: 	out="FourOfKind";     break;
    case StraightFlush: out="StraightFlush";  break;
    case RoyalFlush:    out="RoyalFlush";     break;
    default:            out="Error converting combination to QString"; break;
    }

    return out;
}

int     sortCards(QList<Card> *cards, int size)
{
    if( size < 2 )
        return 1;

    Card card;
    for(int i=0;i<size-1;i++)
        for(int j=0;j<size-1;j++)
            if ((*cards)[j].nominal<(*cards)[j+1].nominal)
            {
                card=(*cards)[j];
                (*cards)[j]=(*cards)[j+1];
                (*cards)[j+1]=card;
            }

    return 0;
}

void Strategy::addLog(QString text)
{
    if(m_file->isOpen())
    {
        QTextStream out(m_file);
        out<<QTime::currentTime().toString("hh:mm:ss")<<" "<<text<<"\n";
    }
    else
    {
        qDebug()<<"Error while writting logs";
    }
}

Strategy::Strategy()
{
    m_stackMin = 0;
    m_stackRebuy = 0;
    m_stackMax = 0;


    m_sitOut = false;
    m_sitOutNextBB = false;
    m_rebuy = false;
//    m_closeTable = false;
}

Strategy::~Strategy()
{
}

//Проверка на выход
//если игроков меньше 7ми (по стратегии меньше 8ми)
//если еще не рекомендовалось выходить
bool Strategy::sitOutByPlayers(QList<Player> players, bool alreadySitOut)
{
    if((players.size() < 7) &&
       !alreadySitOut)
    {
        return true;
    }

    return false;
}

bool Strategy::sitOutByStack(qreal myStack, bool alreadySitOut)
{
    if((myStack > m_stackMax) &&
       !alreadySitOut)
    {
        return true;
    }

    return false;
}

// Проверка на докупку (пока не используется)
// !!Проверить инициализацию структуры
Recommendation Strategy::checkRebuy(qreal myStack, bool alreadyRebuy)
{
    Recommendation recommendation = { NoAction, 0};

    if(myStack < m_stackMin &&
       !alreadyRebuy)
    {
        recommendation.recAct = Rebuy;
        recommendation.value = m_stackRebuy - myStack;
    }

    return recommendation;
}


void Strategy::initStackParams(qreal BB)
{
    m_stackMax = 50*BB;
    m_stackMin = 30*BB;
    m_stackRebuy = 40*BB;
}

//------------------------------------------------------------------------
// preflopMSSbasic       - Определение рекомендуемого действия по чарту MSS
//------------------------------------------------------------------------
// принимает:   table - класс стола, содержащий информацию, полученную с картинки
// возвращает:  recommendation - структура рекомендованного действия
// определяет:  numberOfButton - номер диллера
//              iFoldPlayers - количество сбросившихся игроков перед нами
//              numberOfRaisers - количество повысивших перед нами
//              iActivePlayers - количество активных игроков (из lpayer.bEnable)
// использует:  table->bigBlind - размер ББ
//              table->totalPot - размер банка
//              m_starterNumber - номер в списке игроков, открывающий торги
//              m_bet - текущая ставка
//              m_myNumber  - номер нашего игрока в списке игроков
//              m_players[m_myNumber].cards() - список наших карта
//              m_players.size() - количество игроков, вошедших в игру
//              m_circle - круг торгов на данной улице
//
/*Recommendation Strategy::preflopMSSbasic(Table *table)
{
    addLog(QString("preflopMSSbasic() start"));
    Recommendation recommendation;
    QList<Card> myCards = m_players[m_myNumber].cards();
    GameStatus::sortCards(&myCards, myCards.size()); //выставляем старшую карту на первое место (требуется при сравнении)
    int numberOfPlayers = m_players.size();
    int myNumber = m_players[m_myNumber].number()-2;//номер нашего игрока, начиная от m_starterNumber
    if(myNumber<1) //в фунции number() номера отсчитываются от игрока после диллера, но на префлопе нужно от игрока
        myNumber+=m_players.size();  //после BB
    qreal myStack = m_players[m_myNumber].stack();
    qreal myBet = 0;
    //Определяем сколько всего нами уже поставлено в банк
    if(m_players[m_myNumber].bets(PreFlop).size())
    {
        myBet=m_players[m_myNumber].bets(PreFlop).last();
    }
    addLog(QString("my position=%1").arg(m_players[m_myNumber].position()));

    if((m_players[m_myNumber].position() != Pos_BB ) &&
       (myCards.at(0).nominal != myCards.at(1).nominal &&
        ((myCards.at(0).nominal < Card::Jack)||
         (myCards.at(0).nominal != Card::Ace && myCards.at(1).nominal < Card::Ten ))))//если мы не на большом блайнде и наши карты не равны, первая меньше вальта, а вторая меньше десяти, то нам рекомендуется сброс
    {
        recommendation.recAct = Fold; // рекомендуемый сброс
    }
    else //если нам сразу не рекомендуется фолд из-за плохих карт
    {
        switch(m_circle){ // проверяем, на каком кругу производится анализ чарта
        case 1:{
            int iRaisePF=0;                     //количество рэйзов на префлопе при анализе действий игроков
            int iCallRaisePF=0;                 //количество заколировавших рэйз на префлопе
            int iRaisePFNumber=0;               //номер игрока, повысившего на префлопе
            int iAllInPFNumber=0;               //количество игроков, вошедших ALL-IN
            Position iRaisePosition=Pos_None;   //позиция повысившего
            int iFoldPlayers=0;                 //количество сбросившихся игроков
            int iTemp=0;                        //временная переменная для проверки стилов
            recommendation.value=table->bigBlind*3; //по умолчанию размер рэйза = 3 больших блайнда

            int i=m_starterNumber;
            while(i != m_myNumber)              // осуществляем проверку игроков, проходя до нас
            {
                if(m_players[i].isActive())     // если игрок активен
                {
                    addLog(QString("i=%1 action=%2").arg(i).arg(m_players[i].actions(PreFlop).at(0)));
                    if (m_players[i].actions(PreFlop).at(0) >= Raise)        //если действие текущего игрока ставка либо стил
                    {
                        iRaisePF++;                          //подсчет игроков, повысивших ставку
                        iRaisePFNumber=i;                    //определяем номер повысившего игрока
                        iRaisePosition=m_players[i].position(); //определяем из какой позиции игрок повысил
                    }
                    if (m_players[i].actions(PreFlop).at(0) == BlindSteal)
                        iTemp=999; //если был стил, то "флаг" поднимается
                    if (m_players[i].stack()==0)
                        iAllInPFNumber++; //подсчитываем количество игроков, вошедших ALL-IN
                }
                i++;
                if( i >= numberOfPlayers)
                    i -= numberOfPlayers;     // проверка на зацикливаемость
            }
            addLog(QString("raisers=%1").arg(iRaisePF));
            addLog(QString("myNumber=%1").arg(myNumber));
            if(iRaisePF)//если количество повысивших игроков больше ноля, то считаем сколько игроков ответило на ставку (на последнюю ставку)
            {
                i=iRaisePFNumber;
                while(i != m_myNumber)//пока не дошли до нас подсчитываем сколько игроков ответило на ставку
                {
                    if(m_players[i].actions(PreFlop).at(0) == Call)//если игрок принимал решение ответить после повысившего
                    {
                        iCallRaisePF++; //увеличиваем количество игроков ответивших на рэйз
                    }
                    i++;
                    if( i >= numberOfPlayers)
                        i -= numberOfPlayers;     // проверка на зацикливаемость
                }
            }
            else // если никто не повышал подсчитываем количество сбросившихся
            {
                i=m_starterNumber; // начинаем с первого игрока
                while(i != m_myNumber) // пока не дошли до нас
                {
                    if(m_players[i].actions(PreFlop).at(0) == Fold) // если игрок скинул
                    {
                        iFoldPlayers++; // увеличиваем количество игроков, скинувших карты
                    }
                    i++;
                    if( i >= numberOfPlayers)
                        i -= numberOfPlayers;     // проверка на зацикливаемость
                }
                addLog(QString("folders=%1").arg(iFoldPlayers));
            }
            //---Определение рекомендуемого действия------/
            if(iRaisePF>=2) //если повышений было 2 или больше
            {
                if(myCards.at(0).nominal == myCards.at(1).nominal &&
                   myCards.at(0).nominal > Card::Queen)
                {
                    recommendation.recAct = Raise;
                    recommendation.value = myStack;
                }//если у нас AA, или KK, то рекомендует All-In
                else
                    recommendation.recAct = Fold; //если у нас нет тузов или королей, то рекомендуется сброс
            }
            else //если повышений было меньше 2
            {
                if(iRaisePF==1)//если было одно повышение
                {
                    if((iAllInPFNumber!=0) &&
                       (m_bet>table->bigBlind*2))//если кто-то зашел ALL-IN и ставка >2BB, разыгрываем как на второй улице
                        goto SecondCircle;

                    if(myCards.at(0).nominal == myCards.at(1).nominal &&
                       myCards.at(0).nominal > Card::Jack)//если наши карты AA,KK, QQ
                    {
                        recommendation.recAct = Raise; //рекомендуем повышать
                        recommendation.value = (m_bet*3)+(m_bet*iCallRaisePF);//размер ставки в три раза больше текущей ставки плюс 1 размер на каждого ответившего
                    }
                    else //если у нас не тузы, не короли, не дамы
                        {
                            //--Проверка, если повысивший делал all-in то разыгрываем как на втором круге

                            if(myCards.at(0).nominal == Card::Ace &&
                               myCards.at(1).nominal== Card::King)//если у нас AK
                            {
                                if(iRaisePosition > Pos_UTG)//если рейз либо из средней позиции либо из поздней
                                {
                                    recommendation.recAct = Raise; //рекомендуем повышать
                                    recommendation.value = (m_bet*3)+(m_bet*iCallRaisePF);//размер ставки в три раза больше текущей ставки плюс 1 размер на каждого ответившего
                                }
                                else//если рэйз был из ранней позиции
                                    recommendation.recAct = Fold;//то рекомендуется сброс
                            }
                            else //если у нас не AK
                            {
                                if(myCards.at(0).nominal == myCards.at(1).nominal &&
                                   myCards.at(0).nominal == Card::Jack)//если у нас JJ
                                {
                                    if(iRaisePosition > Pos_MP)//если повышали из позиции выше средней
                                    {
                                        recommendation.recAct = Raise; //рекомендуем повышать
                                        recommendation.value = (m_bet*3)+(m_bet*iCallRaisePF);//размер ставки в три раза больше текущей ставки плюс 1 размер на каждого ответившего
                                    }
                                    else //если повышали из ранней или из средней позиции
                                    {
                                        if(iRaisePosition == Pos_MP && iCallRaisePF == 0)//если повышали из средней позиции и никто не ответил на повышение
                                        {
                                            recommendation.recAct = Raise; //рекомендуем повышать
                                            recommendation.value = m_bet*3;//размер ставки в три раза больше текущей ставки
                                        }
                                        else//если у нас вальты, а повышали либо из ранней позиции либо из средней, но кто-то уже ответил
                                            recommendation.recAct = Fold;
                                    }
                                }
                                else //если у нас не JJ
                                {
                                    if((myCards.at(0).nominal == myCards.at(1).nominal &&
                                        myCards.at(0).nominal > Card::Eight) ||
                                       (myCards.at(0).nominal == Card::Ace && myCards.at(1).nominal > Card::Ten))//если наши карты 99,TT,AQ,AJ
                                    {
                                        if(iRaisePosition > Pos_UTG && iCallRaisePF == 0)//если повышали не из ранней позиции и никто не ответил на повышение
                                        {
                                            recommendation.recAct = Raise; //рекомендуем повышать
                                            recommendation.value = m_bet*3;//размер ставки в три раза больше текущей ставки
                                        }
                                        else //если повышали из ранней позиции либо кто-то уже ответил на ставку
                                            recommendation.recAct = Fold; //рекомендуется сброс
                                    }
                                    else //если у нас не TT,99,AQ,AJ
                                    {
                                        if(myCards.at(0).nominal == Card::Ace &&
                                           myCards.at(1).nominal == Card::Ten)//если у нас AT
                                        {   if(iCallRaisePF==0)//если никто не отвечал еще на ставку
                                            {
                                                if(iRaisePosition > Pos_MP)//позиция повысившего выше средней
                                                {
                                                    recommendation.recAct = Raise; //рекомендуем повышать
                                                    recommendation.value = m_bet*3;//размер ставки в три раза больше текущей ставки
                                                }
                                                else
                                                {   if(iRaisePosition==2 && myCards.at(0).suit==myCards.at(1).suit)//повышали из средней позиции и у нас туз десять одномастные
                                                    {
                                                        recommendation.recAct = Raise; //рекомендуем повышать
                                                        recommendation.value = m_bet*3;//размер ставки в три раза больше текущей ставки
                                                    }
                                                    else
                                                        recommendation.recAct = Fold;
                                                }
                                            }

                                            else recommendation.recAct = Fold; //если у нас AT но кто-то ответил на ставку до нас
                                        }
                                        else //если у нас не AT
                                        {   if(((myCards.at(0).nominal == myCards.at(1).nominal &&
                                                 myCards.at(0).nominal == Card::Eight) ||
                                                (myCards.at(0).nominal == Card::Ace &&
                                                 myCards.at(1).nominal == Card::Nine &&
                                                 myCards.at(0).suit == myCards.at(1).suit)) &&
                                                 iCallRaisePF==0 && iRaisePosition > Pos_MP) //если у нас 88,A9s и повышали из позиции выше средней и никто не ответил
                                            {
                                                recommendation.recAct = Raise; //рекомендуем повышать
                                                recommendation.value = m_bet*3;//размер ставки в три раза больше текущей ставки
                                            }
                                            else //если не 88,A9s, либо кто-то ответил, либо повышали из ранних позиций
                                                recommendation.recAct = Fold;
                                        }
                                    }
                                }
                            }
                        }
                }
                else //если никто не повышал
                {
                    if((myCards.at(0).nominal == myCards.at(1).nominal &&
                        myCards.at(0).nominal > Card::Nine) ||
                       (myCards.at(0).nominal == Card::Ace &&
                        myCards.at(1).nominal > Card::Jack)) //если у нас AA,KK,QQ,JJ,TT,AK,AQ
                    {
                        if(iFoldPlayers == (myNumber-1) &&
                           m_players[m_myNumber].position() > Pos_MP) // если перед нами все сбросились и мы в поздней позиции либо на блайндах
                           recommendation.recAct = BlindSteal; // то рекомендуется стилл
                        else // если перед нами вошли игроки, либо мы не в поздней позиции
                            recommendation.recAct = Raise;                  //рекомендуем повышать
                    }
                    else //если у нас не AA,KK,QQ,JJ,TT,AK,AQ
                    {
                        if (myCards.at(0).nominal == Card::Ace &&
                            myCards.at(1).nominal== Card::Jack) //если у нас AJ
                        {
                            if(myCards.at(0).suit != myCards.at(1).suit &&
                               m_players[m_myNumber].position() == Pos_UTG)//если AJo, мы в ранней позиции
                                recommendation.recAct = Fold; //рекомендуется сброс
                            else
                            {
                                if(iFoldPlayers == (myNumber-1) &&
                                   m_players[m_myNumber].position() > Pos_MP)// если перед нами все сбросились и мы в поздней позиции либо на блайндах
                                    recommendation.recAct = BlindSteal; // то рекомендуется стилл
                                else // если переднами вошли игроки, либо мы не в поздней позиции
                                    recommendation.recAct = Raise;                  //рекомендуем повышать
                            }
                        }
                        else //если не AJ
                        {
                            if(myCards.at(0).nominal == myCards.at(1).nominal &&
                               myCards.at(0).nominal > Card::Seven) //если у нас 99, 88
                            {
                                if(m_players[m_myNumber].position() > Pos_UTG)
                                {
                                    if(iFoldPlayers == (myNumber-1) &&
                                       m_players[m_myNumber].position() > Pos_MP) // если перед нами все сбросились и мы в поздней позиции либо на блайндах
                                        recommendation.recAct = BlindSteal; // то рекомендуется стилл
                                    else // если переднами вошли игроки, либо мы не в поздней позиции
                                        recommendation.recAct = Raise;//если наша позиция выше ранней
                                }
                                else recommendation.recAct = Fold; //если мы в ранней позиции то с этими картами сброс
                            }
                            else //у нас не 99,88
                            {
                                if (myCards.at(0).nominal == Card::Ace && myCards.at(1).nominal == Card::Ten) //если у нас AT
                                {
                                    if(m_players[m_myNumber].position() > Pos_UTG)//если мы не в ранней позиции
                                    {
                                        if(myCards.at(0).suit != myCards.at(1).suit &&
                                           m_players[m_myNumber].position() == Pos_MP)//если ATo, мы в средней позиции
                                            recommendation.recAct = Fold; //рекомендуется сброс
                                        else
                                        {
                                            if(iFoldPlayers == (myNumber-1) &&
                                               m_players[m_myNumber].position() > Pos_MP &&
                                               m_players[m_myNumber].position() < Pos_BB)// если перед нами все сбросились и мы в поздней позиции либо на малом блайнде
                                                recommendation.recAct = BlindSteal; // то рекомендуется стилл
                                            else // если переднами вошли игроки, либо мы не в поздней позиции
                                                recommendation.recAct = Raise;                  //рекомендуем повышать
                                        }
                                    }
                                    else
                                        recommendation.recAct = Fold;//если мы в ранней позиции, то с AT рекомендуется сброс
                                }
                                else //если у нас не АТ
                                {
                                    if(myCards.at(0).nominal == Card::Ace && myCards.at(1).nominal == Card::Nine)//если у нас A9
                                    {
                                        if(m_players[m_myNumber].position() < Pos_CO)
                                            recommendation.recAct = Fold;//если мы в ранней или средней позиции, то с А9 рекомендуется сброс
                                        else //если мы в поздней позиции или на блайндах
                                        {
                                            if(myCards.at(0).suit == myCards.at(1).suit)//если A9s
                                            {
                                                if(iFoldPlayers == (myNumber-1) &&
                                                   m_players[m_myNumber].position() > Pos_MP &&
                                                   m_players[m_myNumber].position() < Pos_BB)// если перед нами все сбросились и мы в поздней позиции либо на малом блайнде
                                                    recommendation.recAct = BlindSteal; // то рекомендуется стилл
                                                else // если переднами вошли игроки, либо мы не в поздней позиции
                                                    recommendation.recAct = Raise;
                                            }
                                            else //если А9о
                                            {   if(m_players[m_myNumber].position() > Pos_CO)//наша позиция должна быть BU,SB,BB
                                                {
                                                    if(iFoldPlayers == (myNumber-1) &&
                                                       m_players[m_myNumber].position() > Pos_MP &&
                                                       m_players[m_myNumber].position() < Pos_BB)// если перед нами все сбросились и мы в поздней позиции либо на блайндах
                                                        recommendation.recAct = BlindSteal; // то рекомендуется стилл
                                                    else // если переднами вошли игроки, либо мы не в поздней позиции
                                                        recommendation.recAct = Raise;
                                                }
                                                else
                                                    recommendation.recAct = Fold;
                                            }
                                        }
                                     }
                                     else //если у нас не A9
                                     {
                                         if((myCards.at(0).nominal == Card::King &&
                                             myCards.at(1).nominal == Card::Queen) ||
                                            (myCards.at(0).nominal == myCards.at(1).nominal &&
                                             myCards.at(0).nominal == Card::Seven))//если у нас 77,KQ
                                            if(m_players[m_myNumber].position() > Pos_MP)//если мы в поздней позиции либо на блайндах
                                            {
                                                if(iFoldPlayers == (myNumber-1) &&
                                                   m_players[m_myNumber].position() < Pos_BB)// если перед нами все сбросились и мы в поздней позиции либо на блайндах
                                                    recommendation.recAct = BlindSteal;  // то рекомендуется стилл
                                                else            // если переднами вошли игроки, либо мы не в поздней позиции
                                                    recommendation.recAct = Raise;
                                            }
                                            else recommendation.recAct = Fold;//если мы в ранней либо средней позиции, то с KQ сбрасываем
                                         else//если у нас не 77,KQ
                                         {
                                              if(iFoldPlayers == (myNumber-1) )//если перед нами никто не заколировал, а все скинули карты
                                              {
                                                if(m_players[m_myNumber].position()  == Pos_CO)//мы в поздней позиции либо на блайндах (на большом блайнде когда все скинулись не дойдет до нас)
                                                    if((myCards.at(0).nominal == myCards.at(1).nominal &&
                                                        myCards.at(0).nominal > Card::Four) ||
                                                       (myCards.at(0).nominal > Card::Jack &&
                                                        myCards.at(0).suit == myCards.at(1).suit &&
                                                        myCards.at(1).nominal == Card::Jack))   //если у нас 66,55,KJs,QJs
                                                         recommendation.recAct = BlindSteal;
                                                    else recommendation.recAct = Fold;
                                                else //если мы не СО
                                                    if(m_players[m_myNumber].position() >= Pos_CO)//если мы BU или SB,BB
                                                    {
                                                        if(myCards.at(0).nominal == myCards.at(1).nominal ||
                                                           (myCards.at(0).nominal  ==  Card::Ace &&
                                                            myCards.at(0).suit == myCards.at(1).suit) ||
                                                           (myCards.at(0).nominal > Card::Ten &&
                                                            myCards.at(1).nominal > Card::Nine))//оставшиеся карманки 66-22, A8s-A2s,KJ,KT,QJ,QT,JT
                                                            recommendation.recAct = BlindSteal;
                                                        else recommendation.recAct = Fold;
                                                    } else //если мы не BU,SB,BB
                                                        recommendation.recAct = Fold;
                                              }
                                              else recommendation.recAct = Fold;//если кто-то заколировал, то мы сбрасываем с худшими картами
                                         }//не 77,KQ
                                     } //не А9
                                }//не АТ
                            } //не 99,88
                        }//не AJ
                    }//не AA,KK,QQ,JJ,TT,AK,AQ

                    //-------Подсчет рекомендуемого размера ставки---/
                    if(recommendation.recAct == Raise) //если рекомендуется повышать, то подсчитываем количесто лимперов для размера ставки
                    {
                        recommendation.value = table->bigBlind*3;  // по умолчанию размер ставки 3 больших блайнда
                        int i=m_starterNumber;         // начинаем с первого игрока
                        while(i != m_myNumber)        // пока не дошли до нас
                        {
                            if(m_players[i].isActive()) // если игрок активен увеличиваем размер ставки
                                recommendation.value += table->bigBlind;
                            i++;
                            if(i >= numberOfPlayers)
                                i -= numberOfPlayers;
                        }
                    }

                    if(m_players[m_myNumber].position() == Pos_BB &&
                       m_bet == table->bigBlind &&
                       recommendation.recAct == Fold)//если мы на большом блайнде, текущая ставка равна большому блайнду, и рекомендуется скидывать
                    {
                        addLog(QString("change Fold to Check"));
                        recommendation.recAct = Check; //то рекомендовано делать Check
                    }
                }//никто не повышал
            }//если повышений было меньше 2
            break;
       }//case 1
       case 2:{
SecondCircle:
                if(myCards.at(0).nominal == myCards.at(1).nominal &&
                   myCards.at(0).nominal > Card::Jack) //если у нас AA,KK,QQ
                {
                    recommendation.recAct = Raise;
                    recommendation.value = myStack+myBet;
                }
                else //если у нас не AA,KK,QQ
                {
                    if((myCards.at(0).nominal == Card::Ace &&
                        myCards.at(1).nominal == Card::King) &&
                       m_players[m_myNumber].position() > Pos_UTG) //если у нас AK и мы в средней или поздних позициях
                    {
                        recommendation.recAct = Raise;
                        recommendation.value = myStack+myBet;
                    }
                    else //если у нас не AK либо мы не в средней или поздних позициях
                    {   if((myCards.at(0).nominal == myCards.at(1).nominal &&
                            myCards.at(0).nominal == Card::Jack) &&
                            m_players[m_myNumber].position() > Pos_MP)//если у нас JJ и мы в поздних позициях
                        {
                            recommendation.recAct = Raise;
                            recommendation.value = myStack+myBet;
                        }
                        else //если у нас не JJ либо мы не в поздних позициях
                        {   if((myCards.at(0).nominal == myCards.at(1).nominal &&
                                myCards.at(0).nominal == Card::Ten) &&
                               m_players[m_myNumber].actions(PreFlop).at(0) >= BlindSteal)//если у нас TT и мы делали Steal либо ReSteal на первом круге, либо мы на BB
                            {
                                recommendation.recAct = Raise;
                                recommendation.value = myStack+myBet;
                            }
                            else  recommendation.recAct = Fold;   //если у нас не TT либо мы не делали Steal на первом круге
                        }
                    }
                }
                break;
       }//case 2
        case 3: { // если дошли до третьего круга торгов, то ставим вабанк
                recommendation.recAct = Raise;
                recommendation.value = myStack+myBet;
                break;
            } // case 3
       default: //по умолчанию рекомендуем сброс
       {
               recommendation.recAct = Fold;
               break;
           }
       }//switch

        //--------Проверка на действия после стилов рестилов------/
//        if(iCircle>1) //если до нас дошло после первого круга
//        {   if(player[0].Action[0]==5)//если мы делали steal и круг опять дошел до нас (опонент сделал рестил)
//            {    if((myCards.at(0).nominal==myCards.at(1).nominal&&myCards.at(0).nominal>=9)||(myCards.at(0).nominal == Ace&&myCards.at(1).nominal>=11))
//                 {recommendation.value = player[0].Stack;iRecAct=4;}
//                 else iRecAct=1;
//            }

//        }
//        else //если первый круг, то проверка на стиллы опонентов
//        {
//            if(iTemp==999&&player[0].iPosition>4) //если мы на блайндах и перед нами был steal
//                if ((myCards.at(0).nominal == Ace&&myCards.at(1).nominal>10)||(myCards.at(0).nominal==myCards.at(1).nominal&&myCards.at(1).nominal>7))
//                    {recommendation.value = player[0].Stack;iRecAct=4;}
//        }

        //---Дополнительная проверка на частные случаи для ставки---//
        recommendation.value = floor(recommendation.value * 100 + 0.5) / 100; //округляем ставку до центов
        addLog(QString("recommendation.recAct %1").arg(recommendation.recAct));
        addLog(QString("recommendation.value %1").arg(recommendation.value));
        addLog(QString("myStack %1").arg(myStack));
        addLog(QString("my bet=%1").arg(myBet));
        addLog(QString("m_bet %1").arg(m_bet));

        if((recommendation.recAct >= Raise) &&
          (recommendation.value > myStack))
            recommendation.value = myStack+myBet;      //если размер рекомендуемого повышения больше нашего стэка, то приравниваем ставку нашему стэку
        if(recommendation.recAct >= Raise &&
           recommendation.value <= m_bet-myBet)
        {
            recommendation.recAct = Call;        //если рекомендуется повышать, а размер нашего стэка меньше чем текущая ставка, то рекомендуемое действие - Call
            recommendation.value = m_bet-myBet;
        }

        int iActivePlayers = 0;                  // обнуляем количество активных игроков (не скинувших карты и не поставивших все фишки)
        for(int i=0; i<numberOfPlayers; i++)
        {
            if(m_players[i].isActive() == true &&
                m_players[i].stack() != 0)
            iActivePlayers++;
        }
        addLog(QString("iActivePlayers %1").arg(iActivePlayers));
        if(recommendation.recAct == Raise && iActivePlayers == 1)       //если рекомендуется повышать, а остальные игроки уже скинули либо поставили все свои фишки
        {
            recommendation.value = 0;                      //обнуляем размер повышения
            addLog(QString("m_players[m_myNumber].bets(PreFlop).last() %1").arg(m_players[m_myNumber].bets(PreFlop).last()));
            if(m_bet == m_players[m_myNumber].bets(PreFlop).last())  //то если текущая ставка равна нашей предыдущей ставке на префлопе
                recommendation.recAct = Check;                      //рекомендуем чек
            else                                //если текущая ставка отличается от нашей, то
                recommendation.recAct = Call;                      // рекомендуем колировать
        }
    }//если нам сразу не рекомендуется фолд из-за плохих карт

    addLog(QString("recommendation=%1 size=%2").arg(recommendation.recAct).arg(recommendation.value));
    addLog(QString("preflopMSSbasic() end"));

    return recommendation;
}*/

//Recommendation Strategy::preflopMSSinstaFold(Table *table)
//{
//    Recommendation recommendation;

//    return recommendation;
//}

Recommendation Strategy::preflopMSSadvancedFR(int circle,
                                              Table *table,
                                              QList<Player> players,
                                              qreal bet,
                                              int starterNumber,
                                              int myNumber)
{
//    addLog(QString("preflopMSSadvancedFR() start"));
    Recommendation recommendation;
    QList<Card> myCards = players[myNumber].cards(); // !Важно! карты должны быть предварительно отсортированы по убыванию, если не получится использовать через класс
    sortCards(&myCards, myCards.size()); //выставляем старшую карту на первое место (требуется при сравнении)
    int numberOfPlayers = players.size();
    int myNumStarter = players[myNumber].number()-2;//номер Hero, начиная от starterNumber
    if( myNumStarter<1) //в фунции number() номера отсчитываются от игрока после диллера, но на префлопе
        myNumStarter+=players.size();  //нужно отсчитывать от игрока после BB
    int sbNumber = -1;  //номер игрока на SB в списке игроков
    int bbNumber = -1;  //номер игрока на BB в спписке игроков
    for(int i=0; i<players.size(); i++)
    {
        if(players[i].position() == Pos_BB)
            bbNumber = i;
        else if(players[i].position() == Pos_SB)
            sbNumber = i;
    }
    qreal myStack = players[myNumber].stack();
    qreal myBet = 0;
    //Определяем сколько всего нами уже поставлено в банк
    if(players[myNumber].bets(PreFlop).size())
    {
        myBet=players[myNumber].bets(PreFlop).last();
    }
//    addLog(QString("my position=%1").arg(players[myNumber].position()));

    switch(circle){ // проверяем, на каком кругу производится анализ чарта
    case 1:{
        int iRaisePF=0;                     //количество рэйзов на префлопе при анализе действий игроков
        int iCallRaisePF=0;                 //количество заколировавших рэйз на префлопе
        int iRaisePFNumber=0;               //номер игрока, повысившего на префлопе
        QList<int> raiserNumbers;           //номера игроков в списке, сделавших рэйз
        int iAllInPFNumber=0;               //количество игроков, вошедших ALL-IN
        int iLimpNumber=0;                  //количество игроков, сделавших limp
        Position iRaisePosition=Pos_None;   //позиция повысившего
        int iFoldPlayers=0;                 //количество сбросившихся игроков
        int iTemp=0;                        //временная переменная для проверки стилов
        recommendation.value=table->bigBlind*3; //по умолчанию размер рэйза = 3 больших блайнда

        int i=starterNumber;
        while(i != myNumber)              // осуществляем проверку игроков, проходя до нас
        {
            if(players[i].isActive())     // если игрок активен
            {
//                addLog(QString("i=%1 action=%2").arg(i).arg(players[i].actions(PreFlop).at(0)));
                if (players[i].actions(PreFlop).at(0) >= Raise)        //если действие текущего игрока ставка либо стил
                {
                    iRaisePF++;                          //подсчет игроков, повысивших ставку
                    iRaisePFNumber=i;                    //определяем номер повысившего игрока
                    iRaisePosition=players[i].position(); //определяем из какой позиции игрок повысил
                    raiserNumbers.append(i);
                }
                if (players[i].actions(PreFlop).at(0) == BlindSteal)
                    iTemp=999; //если был стил, то "флаг" поднимается
                if (players[i].stack()==0)
                    iAllInPFNumber++; //подсчитываем количество игроков, вошедших ALL-IN
                if (players[i].actions(PreFlop).at(0) == Call)
                    if(players[i].bets(PreFlop).at(0) == table->bigBlind)
                        iLimpNumber++;//подсчитываем количество залимпившихся игроков
            }
            i++;
            if( i >= numberOfPlayers)
                i -= numberOfPlayers;     // проверка на зацикливаемость
        }
//        addLog(QString("raisers=%1").arg(iRaisePF));
//        addLog(QString(" myNumStarter=%1").arg( myNumStarter));
        if(iRaisePF)//если количество повысивших игроков больше ноля, то считаем сколько игроков ответило на ставку (на последнюю ставку)
        {
            i=iRaisePFNumber;
            while(i != myNumber)//пока не дошли до нас подсчитываем сколько игроков ответило на ставку
            {
                if(players[i].actions(PreFlop).at(0) == Call)//если игрок принимал решение ответить после повысившего
                {
                    iCallRaisePF++; //увеличиваем количество игроков ответивших на рэйз
                }
                i++;
                if( i >= numberOfPlayers)
                    i -= numberOfPlayers;     // проверка на зацикливаемость
            }
        }
        else // если никто не повышал подсчитываем количество сбросившихся
        {
            i=starterNumber; // начинаем с первого игрока
            while(i != myNumber) // пока не дошли до нас
            {
                if(players[i].actions(PreFlop).at(0) == Fold) // если игрок скинул
                {
                    iFoldPlayers++; // увеличиваем количество игроков, скинувших карты
                }
                i++;
                if( i >= numberOfPlayers)
                    i -= numberOfPlayers;     // проверка на зацикливаемость
            }
//            addLog(QString("folders=%1").arg(iFoldPlayers));
        }
        //---Определение рекомендуемого действия------/
        if(iRaisePF>=2) //если повышений было 2 или больше
        {
            if(myCards.at(0).nominal == myCards.at(1).nominal &&
               myCards.at(0).nominal >= Card::Queen)
            {
                recommendation.recAct = Raise;
                recommendation.value = myStack;
            }//если у нас AA, KK, QQ, то рекомендует All-In
            else
                recommendation.recAct = Fold; //если у нас не AA,KK,QQ, то рекомендуется сброс
        }
        else //если повышений было меньше 2
        {
            if(iRaisePF==1)//если было одно повышение
            {
                if((iAllInPFNumber!=0) &&
                   (bet>table->bigBlind*2))//если кто-то зашел ALL-IN и ставка >2BB, разыгрываем как на второй улице
                    goto SecondCircleAdvanced;

                //Определить количество рук на игрока для статистики
                qDebug()<<"raiser hands"<<players[raiserNumbers.first()].m_stats.hands;

                if(players[raiserNumbers.first()].m_stats.hands < 15)//если мало статистики на игрока
                {
                     if(players[raiserNumbers.first()].position() == Pos_UTG)
                     {
                         if((myCards.at(0).nominal == myCards.at(1).nominal) &&
                            (myCards.at(0).nominal >= Card::Queen))
                         {
                            recommendation.recAct = Raise;
                                recommendation.value = (bet*3)+(bet*iCallRaisePF);//размер ставки в три раза больше текущей ставки плюс 1 размер на каждого ответившего
                         }
                         else
                             recommendation.recAct = Fold;
                     }
                     else if(players[raiserNumbers.first()].position() == Pos_MP)
                     {
                            if(((myCards.at(0).nominal == myCards.at(1).nominal) &&
                                (myCards.at(0).nominal >= Card::Nine)) ||
                               ((myCards.at(0).nominal == Card::Ace) &&
                                ((myCards.at(1).nominal >= Card::Jack) ||
                                 ((myCards.at(1).nominal == Card::Ten) &&
                                  (myCards.at(0).suit == myCards.at(1).suit)))))
                            {
                                recommendation.recAct = Raise;
                                        recommendation.value = (bet*3)+(bet*iCallRaisePF);//размер ставки в три раза больше текущей ставки плюс 1 размер на каждого ответившего
                            }
                            else
                                recommendation.recAct = Fold;
                     }
                     else if(players[raiserNumbers.first()].position() >= Pos_CO)
                     {
                            if(((myCards.at(0).nominal == myCards.at(1).nominal) &&
                                (myCards.at(0).nominal >= Card::Eight)) ||
                               ((myCards.at(0).nominal == Card::Ace) &&
                                ((myCards.at(1).nominal >= Card::Ten) ||
                                 ((myCards.at(1).nominal == Card::Nine) &&
                                  (myCards.at(0).suit == myCards.at(1).suit)))))
                            {
                                recommendation.recAct = Raise;
                                recommendation.value = (bet*3)+(bet*iCallRaisePF);//размер ставки в три раза больше текущей ставки плюс 1 размер на каждого ответившего
                            }
                            else
                                recommendation.recAct = Fold;
                     }
                     else
                            recommendation.recAct = Fold;
                }
                else //если статистики достаточно
                {
                    qDebug()<<"pfr"<<players[raiserNumbers.first()].m_stats.pfr;
                    if((myCards.at(0).nominal == myCards.at(1).nominal) &&
                        (myCards.at(0).nominal >= Card::King))
                    {
                        recommendation.recAct = Raise;
                        recommendation.value = (bet*3)+(bet*iCallRaisePF);//размер ставки в три раза больше текущей ставки плюс 1 размер на каждого ответившего
                    }
                        else if((players[raiserNumbers.first()].m_stats.pfr>=7) &&
                            (players[raiserNumbers.first()].m_stats.pfr<10))
                    {
                    	if((myCards.at(0).nominal == myCards.at(1).nominal) &&
                        	(myCards.at(0).nominal >= Card::Queen))
                    	{
                        	recommendation.recAct = Raise;
                                recommendation.value = (bet*3)+(bet*iCallRaisePF);//размер ставки в три раза больше текущей ставки плюс 1 размер на каждого ответившего
                        }
                        else
                            recommendation.recAct = Fold;
                    }
                        else if((players[raiserNumbers.first()].m_stats.pfr>=10) &&
                            (players[raiserNumbers.first()].m_stats.pfr<15))
                    {
                    	if(((myCards.at(0).nominal == myCards.at(1).nominal) &&
                        	(myCards.at(0).nominal >= Card::Jack)) ||
                           ((myCards.at(0).nominal == Card::Ace) &&
                            (myCards.at(1).nominal == Card::King)))
                    	{
                        	recommendation.recAct = Raise;
                                recommendation.value = (bet*3)+(bet*iCallRaisePF);//размер ставки в три раза больше текущей ставки плюс 1 размер на каждого ответившего
                        }
                        else
                            recommendation.recAct = Fold;
                    }
                        else if((players[raiserNumbers.first()].m_stats.pfr>=15) &&
                            (players[raiserNumbers.first()].m_stats.pfr<20))
                    {
                    	if(((myCards.at(0).nominal == myCards.at(1).nominal) &&
                        	(myCards.at(0).nominal >= Card::Ten)) ||
                           ((myCards.at(0).nominal == Card::Ace) &&
                            (myCards.at(1).nominal == Card::King)))
                    	{
                        	recommendation.recAct = Raise;
                                recommendation.value = (bet*3)+(bet*iCallRaisePF);//размер ставки в три раза больше текущей ставки плюс 1 размер на каждого ответившего
                        }
                        else
                            recommendation.recAct = Fold;
                    }
                        else if((players[raiserNumbers.first()].m_stats.pfr>=20) &&
                            (players[raiserNumbers.first()].m_stats.pfr<25))
                    {
                    	if(((myCards.at(0).nominal == myCards.at(1).nominal) &&
                        	(myCards.at(0).nominal >= Card::Ten)) ||
                           ((myCards.at(0).nominal == Card::Ace) &&
                            (myCards.at(1).nominal >= Card::Queen)))
                    	{
                        	recommendation.recAct = Raise;
                                recommendation.value = (bet*3)+(bet*iCallRaisePF);//размер ставки в три раза больше текущей ставки плюс 1 размер на каждого ответившего
                        }
                        else
                            recommendation.recAct = Fold;
                    }
                        else if((players[raiserNumbers.first()].m_stats.pfr>=25) &&
                            (players[raiserNumbers.first()].m_stats.pfr<30))
                    {
                    	if(((myCards.at(0).nominal == myCards.at(1).nominal) &&
                        	(myCards.at(0).nominal >= Card::Seven)) ||
                           ((myCards.at(0).nominal == Card::Ace) &&
                            (myCards.at(1).nominal >= Card::Queen)) ||
                           ((myCards.at(0).nominal == Card::Ace) &&
                            (myCards.at(1).nominal == Card::Jack) &&
                            (myCards.at(0).suit == myCards.at(1).suit)) ||
                           ((myCards.at(0).nominal == Card::King) &&
                            (myCards.at(1).nominal == Card::Queen) &&
                            (myCards.at(0).suit == myCards.at(1).suit))) 
                    	{
                        	recommendation.recAct = Raise;
                                recommendation.value = (bet*3)+(bet*iCallRaisePF);//размер ставки в три раза больше текущей ставки плюс 1 размер на каждого ответившего
                        }
                        else
                            recommendation.recAct = Fold;
                    }
                        else if((players[raiserNumbers.first()].m_stats.pfr>=30) &&
                            (players[raiserNumbers.first()].m_stats.pfr<35))
                    {
                    	if(((myCards.at(0).nominal == myCards.at(1).nominal) &&
                        	(myCards.at(0).nominal >= Card::Six)) ||
                           ((myCards.at(0).nominal == Card::Ace) &&
                            (myCards.at(1).nominal >= Card::Jack)) ||
                           ((myCards.at(0).nominal == Card::Ace) &&
                            (myCards.at(1).nominal == Card::Ten) &&
                            (myCards.at(0).suit == myCards.at(1).suit)) ||
                           ((myCards.at(0).nominal == Card::King) &&
                            (myCards.at(1).nominal == Card::Queen) &&
                            (myCards.at(0).suit == myCards.at(1).suit))) 
                    	{
                        	recommendation.recAct = Raise;
                                recommendation.value = (bet*3)+(bet*iCallRaisePF);//размер ставки в три раза больше текущей ставки плюс 1 размер на каждого ответившего
                        }
                        else
                            recommendation.recAct = Fold;
                    }
                        else if(players[raiserNumbers.first()].m_stats.pfr>=35)
                    {
                    	if(((myCards.at(0).nominal == myCards.at(1).nominal) &&
                        	(myCards.at(0).nominal >= Card::Four)) ||
                           ((myCards.at(0).nominal == Card::Ace) &&
                            (myCards.at(1).nominal >= Card::Jack)) ||
                           ((myCards.at(0).nominal == Card::Ace) &&
                            (myCards.at(1).nominal == Card::Ten) &&
                            (myCards.at(0).suit == myCards.at(1).suit)) ||
                           ((myCards.at(0).nominal == Card::King) &&
                            (myCards.at(1).nominal >= Card::Jack) &&
                            (myCards.at(0).suit == myCards.at(1).suit)) ||
                           ((myCards.at(0).nominal == Card::Queen) &&
                            (myCards.at(1).nominal == Card::Jack) &&
                            (myCards.at(0).suit == myCards.at(1).suit))) 
                    	{
                        	recommendation.recAct = Raise;
                                recommendation.value = (bet*3)+(bet*iCallRaisePF);//размер ставки в три раза больше текущей ставки плюс 1 размер на каждого ответившего
                        }
                        else
                            recommendation.recAct = Fold;
                    }
                    else
                        recommendation.recAct = Fold;
                }
            }
            else //если не было повышений
            {
                if(iFoldPlayers == (myNumStarter-1) &&
                   players[myNumber].position() > Pos_MP) // если перед нами все сбросились и мы в поздней позиции либо на блайндах
                {
                    qDebug()<<"fbbtos"<<players[bbNumber].m_stats.fbbtos;
                    qDebug()<<"fsbtos"<<players[sbNumber].m_stats.fbbtos;
                    if((players[myNumber].position() == Pos_CO ||
                        players[myNumber].position() == Pos_BU) &&
                       (players[bbNumber].m_stats.fbbtos>80 ||
                        players[sbNumber].m_stats.fsbtos>80))
                    {
                        recommendation.recAct = BlindSteal;
                    }
                    else if(players[myNumber].position() == Pos_SB &&
                            players[bbNumber].m_stats.fbbtos>80)
                        recommendation.recAct = BlindSteal;
                    else if(players[myNumber].position() == Pos_CO)
                    {
                       if((myCards.at(0).nominal == myCards.at(1).nominal &&
                           myCards.at(0).nominal > Card::Four) ||
                          (myCards.at(0).nominal == Card::Ace &&
                           myCards.at(1).nominal > Card::Nine) ||
                          (myCards.at(0).nominal == Card::Ace &&
                           myCards.at(1).nominal == Card::Nine &&
                           myCards.at(0).suit == myCards.at(1).suit) ||
                          (myCards.at(0).nominal == Card::King &&
                           myCards.at(1).nominal == Card::Queen) ||
                          (myCards.at(0).nominal == Card::King &&
                           myCards.at(1).nominal == Card::Jack &&
                           myCards.at(0).suit == myCards.at(1).suit) ||
                          (myCards.at(0).nominal == Card::Queen &&
                           myCards.at(1).nominal == Card::Jack &&
                           myCards.at(0).suit == myCards.at(1).suit))
                       {
                           recommendation.recAct = BlindSteal;
                       }
                       else
                           recommendation.recAct = Fold;
                    }
                    else if((players[myNumber].position() == Pos_BU) ||
                            (players[myNumber].position() == Pos_SB))
                    {
                       if((myCards.at(0).nominal == myCards.at(1).nominal) ||
                          (myCards.at(0).nominal == Card::Ace &&
                           myCards.at(1).nominal > Card::Eight) ||
                          (myCards.at(0).nominal == Card::Ace &&
                           myCards.at(0).suit == myCards.at(1).suit) ||
                          (myCards.at(0).nominal == Card::King &&
                           myCards.at(1).nominal > Card::Nine) ||
                          (myCards.at(0).nominal == Card::Queen &&
                           myCards.at(1).nominal > Card::Nine) ||
                          (myCards.at(0).nominal == Card::Jack &&
                           myCards.at(1).nominal == Card::Ten))
                       {
                           recommendation.recAct = BlindSteal;
                       }
                       else
                           recommendation.recAct = Fold;
                    }
                }
                else //если перед нами не все сбросились
                {
                   if(players[myNumber].position() == Pos_UTG)
                   {
                       if((myCards.at(0).nominal == myCards.at(1).nominal &&
                           myCards.at(0).nominal > Card::Eight) ||
                          (myCards.at(0).nominal == Card::Ace &&
                           myCards.at(1).nominal > Card::Ten))
                       {
                           recommendation.recAct = Raise;
                       }
                       else
                           recommendation.recAct = Fold;
                   }
                   else if((myNumber == players.size() - 2) ||
                           (players[myNumber].position() == Pos_CO) ||
                           (players[myNumber].position() == Pos_BU)) //позиция MP3, CO, BU
                   {
                       if((myCards.at(0).nominal == myCards.at(1).nominal &&
                           myCards.at(0).nominal >= Card::Seven) ||
                          (myCards.at(0).nominal == Card::Ace &&
                           myCards.at(1).nominal >= Card::Ten) ||
                          (myCards.at(0).nominal == Card::Ace &&
                           myCards.at(1).nominal >= Card::Seven&&
                           myCards.at(0).suit == myCards.at(1).suit) ||
                          (myCards.at(0).nominal == Card::King &&
                           myCards.at(1).nominal >= Card::Jack) ||
                          (myCards.at(0).nominal == Card::King &&
                           myCards.at(1).nominal == Card::Ten&&
                           myCards.at(0).suit == myCards.at(1).suit) ||
                          (myCards.at(0).nominal == Card::Queen &&
                           myCards.at(1).nominal == Card::Jack) ||
                          (myCards.at(0).nominal == myCards.at(1).nominal+1 &&
                           myCards.at(0).nominal >= Card::Eight &&
                           myCards.at(0).suit == myCards.at(1).suit))
                       {
                           recommendation.recAct = Raise;
                       }
                       else
                           recommendation.recAct = Fold;
                   }
                   else if(players[myNumber].position() == Pos_MP)
                   {
                       if((myCards.at(0).nominal == myCards.at(1).nominal &&
                           myCards.at(0).nominal >= Card::Eight) ||
                          (myCards.at(0).nominal == Card::Ace &&
                           myCards.at(1).nominal >= Card::Ten))
                       {
                           recommendation.recAct = Raise;
                       }
                       else
                           recommendation.recAct = Fold;
                   }
                   else if((players[myNumber].position() == Pos_BB) &&
                           (iFoldPlayers == (myNumStarter-2)) &&
                           (players[sbNumber].actions(PreFlop).at(0) == Call))
                   {
                       if((myCards.at(0).nominal == myCards.at(1).nominal &&
                           myCards.at(0).nominal >= Card::Seven) ||
                          (myCards.at(0).nominal == Card::Ace &&
                           myCards.at(1).nominal >= Card::Ten) ||
                          (myCards.at(0).nominal == Card::Ace &&
                           myCards.at(1).nominal >= Card::Seven&&
                           myCards.at(0).suit == myCards.at(1).suit) ||
                          (myCards.at(0).nominal == Card::King &&
                           myCards.at(1).nominal >= Card::Jack) ||
                          (myCards.at(0).nominal == Card::King &&
                           myCards.at(1).nominal == Card::Ten &&
                           myCards.at(0).suit == myCards.at(1).suit) ||
                          (myCards.at(0).nominal == Card::Queen &&
                           myCards.at(1).nominal == Card::Jack) ||
                          (myCards.at(0).nominal == myCards.at(1).nominal+1 &&
                           myCards.at(0).nominal >= Card::Eight &&
                           myCards.at(0).suit == myCards.at(1).suit))
                       {
                           recommendation.recAct = Raise;
                       }
                       else
                           recommendation.recAct = Fold;
                   }
                   else if((players[myNumber].position() == Pos_BB) ||
                           (players[myNumber].position() == Pos_SB))
                   {
                       if(iLimpNumber == 1)
                       {
                           if((myCards.at(0).nominal == myCards.at(1).nominal &&
                               myCards.at(0).nominal >= Card::Seven) ||
                              (myCards.at(0).nominal == Card::Ace &&
                               myCards.at(1).nominal >= Card::Ten) ||
                              (myCards.at(0).nominal == Card::King &&
                               myCards.at(1).nominal >= Card::Jack) ||
                              (myCards.at(0).nominal == Card::King &&
                               myCards.at(1).nominal == Card::Ten &&
                               myCards.at(0).suit == myCards.at(1).suit) ||
                              (myCards.at(0).nominal == Card::Queen &&
                               myCards.at(1).nominal == Card::Jack))
                           {
                               recommendation.recAct = Raise;
                           }
                           else
                               recommendation.recAct = Fold;
                       }
                       else if(iLimpNumber >=2)
                       {
                           if((myCards.at(0).nominal == myCards.at(1).nominal &&
                               myCards.at(0).nominal >= Card::Nine) ||
                              (myCards.at(0).nominal == Card::Ace &&
                               myCards.at(1).nominal >= Card::Jack) ||
                              (myCards.at(0).nominal == Card::King &&
                               myCards.at(1).nominal >= Card::Queen))
                           {
                               recommendation.recAct = Raise;
                           }
                           else
                               recommendation.recAct = Fold;

                       }
                       else
                           recommendation.recAct = Fold;
                   }
                }

                //-------Подсчет рекомендуемого размера ставки---/
                if(recommendation.recAct >= Raise) //если рекомендуется повышать, то подсчитываем количесто лимперов для размера ставки
                {
                    recommendation.value = table->bigBlind*3;  // по умолчанию размер ставки 3 больших блайнда
                    int i=starterNumber;         // начинаем с первого игрока
                    while(i != myNumber)        // пока не дошли до нас
                    {
                        if(players[i].isActive()) // если игрок активен увеличиваем размер ставки
                            recommendation.value += table->bigBlind;
                        i++;
                        if(i >= numberOfPlayers)
                            i -= numberOfPlayers;
                    }
                }

                if(players[myNumber].position() == Pos_BB &&
                   bet == table->bigBlind &&
                   recommendation.recAct == Fold)//если мы на большом блайнде, текущая ставка равна большому блайнду, и рекомендуется скидывать
                {
                    addLog(QString("change Fold to Check"));
                    recommendation.recAct = Check; //то рекомендовано делать Check
                }
            }
        }
    break;
    }
    case 2: {
SecondCircleAdvanced:
        QList<int> raisePlayersBeforeHero;
        QList<int> raisePlayers;
        QList<int> activePlayers;
        int i=starterNumber;
        while(i != myNumber)              // осуществляем проверку игроков, проходя до нас
        {
//            addLog(QString("i=%1 first action=%2").arg(i).arg(players[i].actions(PreFlop).first()));
            if (players[i].actions(PreFlop).first() >= Raise)        //если действие текущего игрока ставка либо стил
                raisePlayersBeforeHero.append(i);
            i++;
            if( i >= numberOfPlayers)
            i -= numberOfPlayers;
        }
        i=myNumber+1;
        if( i >= numberOfPlayers)
        i -= numberOfPlayers;
        while(i != myNumber)
        {
            if (players[i].isActive())
                activePlayers.append(i);
            if (players[i].actions(PreFlop).last() >= Raise)
                raisePlayers.append(i);
            i++;
            if( i >= numberOfPlayers)
            i -= numberOfPlayers;
        }

        if(raisePlayersBeforeHero.size()) //если кто-то ставил перед нами на первом круге
        {
            if(activePlayers.size()==1 &&
               players[activePlayers.first()].actions(PreFlop).size() == 2) //если остался один оппонент и он нас переставил (проверить, если он лузовый)
            {
                recommendation.recAct = Raise;
                recommendation.value = myStack+myBet;
            }
            else
            {
                if(players[raisePlayers.last()].position() == Pos_UTG)
                {
                    if((myCards.at(0).nominal == myCards.at(1).nominal) &&
                       (myCards.at(0).nominal >= Card::Queen))
                    {
                                recommendation.recAct = Raise;
                                recommendation.value = myStack+myBet;
                    }
                    else
                        recommendation.recAct = Fold;
                }
                else if(players[raisePlayers.last()].position() == Pos_MP)
                {
                    if(((myCards.at(0).nominal == myCards.at(1).nominal) &&
                        (myCards.at(0).nominal >= Card::Queen)) ||
                       ((myCards.at(0).nominal == Card::Ace) &&
                        (myCards.at(1).nominal == Card::King)))
                    {
                                recommendation.recAct = Raise;
                                recommendation.value = myStack+myBet;
                    }
                    else
                        recommendation.recAct = Fold;

                }
                else if(players[raisePlayers.last()].position() >= Pos_CO)
                {
                    if(((myCards.at(0).nominal == myCards.at(1).nominal) &&
                        (myCards.at(0).nominal >= Card::Jack)) ||
                       ((myCards.at(0).nominal == Card::Ace) &&
                        (myCards.at(1).nominal == Card::King)))
                    {
                                recommendation.recAct = Raise;
                                recommendation.value = myStack+myBet;
                    }
                    else
                    {
                        if(players[myNumber].actions(PreFlop).last() == BlindDefense)
                        {
                            if(((myCards.at(0).nominal == myCards.at(1).nominal) &&
                                (myCards.at(0).nominal >= Card::Ten)) ||
                               ((myCards.at(0).nominal == Card::Ace) &&
                                (myCards.at(1).nominal == Card::King)))
                            {
                                        recommendation.recAct = Raise;
                                        recommendation.value = myStack+myBet;
                            }
                            else
                                recommendation.recAct = Fold;
                        }
                        else
                            recommendation.recAct = Fold;
                    }
                }
                else
                            recommendation.recAct = Fold;
            }

        }
        else //если никто перед нами не повышал на первом круге
        {
            if((myCards.at(0).nominal == myCards.at(1).nominal &&
                myCards.at(0).nominal >= Card::Queen) ||
               (myCards.at(0).nominal == Card::Ace &&
                myCards.at(1).nominal == Card::King))//если у нас QQ+,AK
            {
                recommendation.recAct = Raise;
                recommendation.value = myStack+myBet;
            }
            else //если у нас не QQ+,AK
            {
                if((myNumber == players.size() - 2) ||
                (players[myNumber].position() == Pos_CO) ||
                (players[myNumber].position() == Pos_BU)) //позиция MP3, CO, BU
                {
                    if((myCards.at(0).nominal == myCards.at(1).nominal &&
                     myCards.at(0).nominal >= Card::Ten))
                    {
                     recommendation.recAct = Raise;
                     recommendation.value = myStack+myBet;
                 }
                else
                    recommendation.recAct = Fold;
            }
            else if((players[myNumber].position() == Pos_SB) ||
                    (players[myNumber].position() == Pos_BB))
            {
               //определяем позицию игроков сделавших 3Bet
               QList<Position> raisePositions;
               int i=myNumber+1;
               if( i >= numberOfPlayers)
               i -= numberOfPlayers;     // проверка на зацикливаемость
               while(i != myNumber)              // осуществляем проверку игроков, проходя до нас
               {
                   if(players[i].isActive())     // если игрок активен
                   {
//                       addLog(QString("i=%1 last action=%2").arg(i).arg(players[i].actions(PreFlop).last()));
                       if (players[i].actions(PreFlop).last() >= Raise)        //если действие текущего игрока ставка либо стил
                       {
                           raisePositions.append(players[i].position());
                       }
                       i++;
                       if( i >= numberOfPlayers)
                       i -= numberOfPlayers;     // проверка на зацикливаемость'
                   }
               }
//                   addLog(QString("raisers number=%1").arg(raisePositions.size()));
//                   for(int j=0;j<raisePositions.size();j++)
//                       addLog(QString("%1 position %2").arg(j).arg(raisePositions.at(j)));
                   if(raisePositions.size())
                   {
                       if(raisePositions.first() >= Pos_CO)
                       {
                           if((myCards.at(0).nominal == myCards.at(1).nominal &&
                               myCards.at(0).nominal >= Card::Ten))
                           {
                               recommendation.recAct = Raise;
                               recommendation.value = myStack+myBet;
                           }
                           else
                            recommendation.recAct = Fold;
                       }
                       else
                           recommendation.recAct = Fold;
                   }
                   else
                       recommendation.recAct = Fold;
                }
                else
                {
                    recommendation.recAct = Fold;
                }
            }
        }
    break;
    }
    case 3: { // если дошли до третьего круга торгов, то ставим вабанк
            recommendation.recAct = Raise;
            recommendation.value = myStack+myBet;
            break;
    } // case 3
    default:
    {
        recommendation.recAct = Fold;
        break;
    }}
    
    //---Дополнительная проверка на частные случаи для ставки---//
    recommendation.value = floor(recommendation.value * 100 + 0.5) / 100; //округляем ставку до центов
//    addLog(QString("recommendation.recAct %1").arg(recommendation.recAct));
//    addLog(QString("recommendation.value %1").arg(recommendation.value));
//    addLog(QString("myStack %1").arg(myStack));
//    addLog(QString("my bet=%1").arg(myBet));
//    addLog(QString("bet %1").arg(bet));

    if((recommendation.recAct >= Raise) &&
      (recommendation.value > myStack))
        recommendation.value = myStack+myBet;      //если размер рекомендуемого повышения больше нашего стэка, то приравниваем ставку нашему стэку
    if(recommendation.recAct >= Raise &&
       recommendation.value <= bet-myBet)
    {
        recommendation.recAct = Call;        //если рекомендуется повышать, а размер нашего стэка меньше чем текущая ставка, то рекомендуемое действие - Call
        recommendation.value = bet-myBet;
    }

    int iActivePlayers = 0;                  // обнуляем количество активных игроков (не скинувших карты и не поставивших все фишки)
    for(int i=0; i<numberOfPlayers; i++)
    {
        if(players[i].isActive() == true &&
            players[i].stack() != 0)
        iActivePlayers++;
    }
//    addLog(QString("iActivePlayers %1").arg(iActivePlayers));
    if(recommendation.recAct == Raise && iActivePlayers == 1)       //если рекомендуется повышать, а остальные игроки уже скинули либо поставили все свои фишки
    {
        recommendation.value = 0;                      //обнуляем размер повышения
//        addLog(QString("players[myNumber].bets(PreFlop).last() %1").arg(players[myNumber].bets(PreFlop).last()));
        if(bet == players[myNumber].bets(PreFlop).last())  //то если текущая ставка равна нашей предыдущей ставке на префлопе
            recommendation.recAct = Check;                      //рекомендуем чек
        else                                //если текущая ставка отличается от нашей, то
            recommendation.recAct = Call;                      // рекомендуем колировать
    }
//    addLog(QString("recommendation=%1 size=%2").arg(recommendation.recAct).arg(recommendation.value));
//    addLog(QString("preflopMSSadvancedFR() end"));

    return recommendation;
}

bool Strategy::checkFlopDraw(QList<Card> tableCards)
{
    sortCards(&tableCards, tableCards.size());
    bool bDraw=false;

    if((tableCards[0].nominal == tableCards[1].nominal+1) &&
       (tableCards[1].nominal == tableCards[2].nominal+1))
        bDraw=true;//если три карты подряд
    else {
        if(tableCards[0].suit == tableCards[1].suit &&
           tableCards[1].suit == tableCards[2].suit)
            bDraw=true; //если три карты одинаковой масти
        else {
            if(tableCards[0].nominal == tableCards[1].nominal+1)//если первые две карты подряд
            {
                if(tableCards[0].suit == tableCards[1].suit ||
                   tableCards[1].suit == tableCards[2].suit ||
                   tableCards[0].suit == tableCards[2].suit)
                    bDraw=true;}//одинаковые рубашки у первой и второй карти или второй и третьей или у первой и третьей
            else {    //если первые две карты не подряд
                if(tableCards[1].nominal == tableCards[2].nominal+1)//если вторая и третья карты подряд
                {
                    if(tableCards[0].suit == tableCards[1].suit ||
                       tableCards[1].suit == tableCards[2].suit ||
                       tableCards[0].suit == tableCards[2].suit)
                        bDraw=true;
                }//одинаковые рубашки у первой и второй карты или второй и третьей или у первой и третьей
            }
         }
    }

    return bDraw;
}

/*bool Strategy::checkFlopToTurnDraw(QList<Card> tableCards)
{
    bool bFlopToTurnDraw = false;
    Card drawCard;
    drawCard.suit = Card::NoSuit;
    drawCard.nominal = Card::NoNominal;
    Card turnCard = tableCards.last();
    int iClubs = 0;
    int iHearts = 0;
    int iSpades = 0;
    int iDiamonds = 0;

    for(int i=0; i<tableCards.size(); i++)
    {
        switch (tableCards[i].suit) {
        case Card::Clubs: {
            iClubs++;
            break;
        }
        case Card::Hearts: {
            iHearts++;
            break;
        }
        case Card::Spades: {
            iSpades++;
            break;
        }
        case Card::Diamonds: {
            iDiamonds++;
            break;
        }
        default:
            break;
        }
    }

    if((iClubs > 2 && turnCard.suit == Card::Clubs) ||
       (iHearts > 2 && turnCard.suit == Card::Hearts) ||
       (iSpades > 2 && turnCard.suit == Card::Spades) ||
       (iDiamonds > 2 && turnCard.suit == Card::Diamonds))
        bFlopToTurnDraw = true;
    else
    {
        QList<Card> cards = tableCards;
        sortCards(&cards, cards.size());
//        Удаление карт одинакового номинала из списка
        for(int j=0;j<cards.size()-1;j++)
        for(int i=0;i<cards.size()-1;i++)
            if(cards.at(i).nominal == cards.at(i+1).nominal)    // если номинал следующей карты равен номиналу текущей карты
            {   if(i!=0)
                {   if(cards.at(i).suit==cards.at(i-1).suit)    // если рубашка текущей карты равна рубашке предыдущей карты,
                    {
                        cards.removeAt(i+1);                    // то происходит удаление следующей карты
                    }
                    else                                        // если рубашки не равны
                    {
                        cards.removeAt(i);                      // в противном случае происходит удаление текущей карты
                    }
                }
                else{
                    cards.removeAt(i);
                }
            }

//    Проверка на стрит
    for(int i=0;i<cards.size()-1;i++)               // проверка на последовательность из пяти карт подрят (если 4 карты подрят и пятая не вписывается в стрит, то выходим из цикла)
    {
        if(cards.at(i).nominal == cards.at(i+1).nominal+1)  // если следующая карта отличается от текущей на 1
        {
            if(!bStraight) //если флаг стрита еще не поднят (определение первой последовательности из двух подряд карт
            {
                bStraight=true;                     // поднимаем флаг наличия стрита
                highCard = cards.at(i);         //приравниваем старшую карту стрита
            }
            count++;                                // увеличиваем на единицу количество подряд идущих карт
        }
        else                                       //если следующая карта отличается больше, чем на единицу
        {
            count=1;                           // сбрасываем поличество подряд идущих карт
            bStraight=false;                   // опускаем флаг наличия предполагаемого стрита
            highCard.nominal = Card::NoNominal;                       // убираем старшую карту
        }
        if(count==4 && highCard.nominal == Card::Five && cards.at(0).nominal == Card::Ace)
            count++;//частный случай стрита, если он начинается с туза
        if(count==5)break;                         // выход из цикла, если собрали стрит
        if((i==cards.size()-2) && (count<5))         // если дошли до последнего члена и не собрали 5 карт подряд то
        {
            highCard.nominal = Card::NoNominal;                           // сбрасываем старшую карту стрита и
            bStraight=false;                       // сбрасываем флаг стрита
        }
    }

        for(int i=0; i<)
        QList<Card> straightDraw = tableCards.first();
        for(int i=0; i<tableCards.size()-1; i++)
        {
            if(tableCards[i].nominal == tableCards[i+1].nominal+1)
            {
                straightDraw.append();

            }
        }
    }

    return bFlopToTurnDraw;
}*/


//------------------------------------------------------------------------
// postflopMSSadvanced - продвинутая стратегия на постфлопе
//------------------------------------------------------------------------
// возвращает:  recommendation - структура рекомендованного действия
// принимает:  table->cards  - карты стола
//              table->totalPot - текущий размер банка
//              starterNumber - номер в списке игроков, открывающий торги
//              bet - текущая ставка
//              players - список игроков
//              myNumber  - наш номер в списке игроков
//              players[m_myNumber].cards - список наших карт
//              players.size() - количество игроков, вошедших в игру
//              stage  - текущая стадия игры
//              combination - комбинация
//              circle - круг торгов на данной улице
//              outs   - количество аутсов
//              kickPair - кикер дл пары
//
Recommendation Strategy::postflopMSSadvanced(Stage stage,
                                             int circle,
                                             Table *table,
                                             QList<Player> players,
                                             qreal bet,
                                             int starterNumber,
                                             int myNumber,
                                             Combination combination,
                                             QList<Card> outs,
                                             Card kickPair )
{
    addLog(QString("postflopMSSadvanced() start"));
    Recommendation recommendation;
    QList<Card> myCards = players[myNumber].cards();
    sortCards(&myCards, myCards.size());
    QList<Card> tableCards = table->cards;
    sortCards(&tableCards, tableCards.size());
    int numberOfPlayers = players.size();
    int iActivePlayers = 0; 	// количество активных игроков
    int numberOfRaisers = 0;  	//количество повысивших игроков перед нами
    int iRaiserNumber = 0; 		//номер последнего в списке игроков, кто сделал ставку
    qreal myStack = players[myNumber].stack();
    qreal myBet = 0;
    if(players[myNumber].bets(stage).size())
        myBet=players[myNumber].bets(stage).last();
    qreal odds = 0;
    qreal minOdds = 0;

    //Определяем сколько всего нами уже поставлено в банк на текущей стадии
    if(players[myNumber].bets(stage).size())
        myBet=players[myNumber].bets(stage).last();

    //Подсчитываем количество активных игроков и определяем диллера
    addLog(QString("numberOfPlayers=%1").arg(numberOfPlayers));
    for(int i=0; i < numberOfPlayers; i++)
    {
        if(players[i].isActive())
            iActivePlayers++;
    }

    //Подсчитываем количество повысивших перед нами плюс после нас на предыдущем круге
    addLog(QString("starterNumber=%1 myNumber=%2").arg(starterNumber).arg(myNumber));
    addLog(QString("circle=%1").arg(circle));
    int i = starterNumber;                  // начинаем с игрока открывающего торги
    while (i != myNumber)                   // производим анализ до момента, пока не дошли до нас
    {
        addLog(QString("i=%1").arg(i));
        if(players[i].isActive())
        {
            addLog(QString("last action=%1").arg(players[i].actions(stage).last()));
            if(players[i].actions(stage).last() == Raise )
            {
                addLog(QString("was raised"));
                numberOfRaisers++;//подсчитываем сколько человек повышало перед нами на текущем круге
                iRaiserNumber = i;
            }
        }
        i++;
        addLog(QString("i++=%1").arg(i));
        if(i >= numberOfPlayers)
            i -= numberOfPlayers;
    }
    addLog(QString("numberOfRaisers=%1").arg(numberOfRaisers));

    //---Основной анализ-------------------------------//
    addLog(QString("my first act on preFlop=%1").arg(players[myNumber].actions(PreFlop).at(0)));
    addLog(QString("combination"));
    addLog(combinationToString(combination));
    addLog(QString("kiker card %1 %2").arg(kickPair.nominal).arg(kickPair.suit));
    addLog(QString("outs=%1").arg(outs.size()));
    for(int i=0; i<outs.size(); i++)
    {
        addLog(QString("outs=%1 %2").arg(outs.at(i).nominal).arg(outs.at(i).suit));
    }
    addLog(QString("table->totalPot %1").arg(table->totalPot));
    addLog(QString("stage %1").arg(stage));

    //Определяем одну из линий: с инициативой, без инициативы, фриплей
    if(players[myNumber].actions(PreFlop).last() == Raise)//у нас инициатива
    {
        addLog(QString("Hero aggressor"));
        switch(stage){
        case Flop: {
            bool bDraw=checkFlopDraw(tableCards);       //проверяем стол на дрова
            addLog(QString("table draw=%1").arg(bDraw));

            if(circle == 1)                 //на первом круге
            {
                if(numberOfRaisers == 0)      //если никто не ставил до нас
                {
                    if((combination >= OverPair) ||
                       (combination == TopPair &&
                        kickPair.nominal >= Card::Jack))
                    {
                        recommendation.recAct = Raise;
                        if((iActivePlayers > 2) && (bDraw == true))
                        {
                            recommendation.value = table->totalPot;
                        }
                        else if((iActivePlayers == 2) && (bDraw == false))
                        {
                            recommendation.value = table->totalPot/2;
                        }
                        else
                        {
                            recommendation.value = 2*table->totalPot/3;
                        }
                    }
                    else if(outs.size() >= 12)
                    {
                        recommendation.recAct = Raise;
                        recommendation.value = 2*table->totalPot/3;
                    }
                    else if(outs.size() >= 8)
                    {
                        if(iActivePlayers > 3)
                        {
                            recommendation.recAct = Check;
                        }
                        else
                        {
                            recommendation.recAct = Raise;
                            recommendation.value = 2*table->totalPot/3;
                        }
                    }
                    else
                    {
                        if(iActivePlayers == 2)
                        {
                            recommendation.recAct = Raise;
                            recommendation.value = 2*table->totalPot/3;
                        }
                        else if(iActivePlayers == 3)
                        {
                            if((bDraw == false) &&
                               ((tableCards.at(0).nominal == Card::Ace) ||
                                (tableCards.at(0).nominal == Card::King)))
                            {
                                recommendation.recAct = Raise;
                                recommendation.value = table->totalPot/2;
                            }
                            else
                            {
                                recommendation.recAct = Check;
                            }
                        }
                        else
                            recommendation.recAct = Check;
                    }
                }
                else if(numberOfRaisers == 1) //если кто-то поставил
                {
                    odds = (qreal)(outs.size())/((qreal)47);
                    minOdds = (bet-myBet)/(table->totalPot+bet-myBet);
                    addLog(QString("odds %1 minOdds %2").arg(odds).arg(minOdds));
                    if(iActivePlayers > 2)    //если несколько оппонентов
                    {
                        if((combination >= TwoPair) ||
                           (outs.size() > 9))     //сильные готовые руки и сильные дро
                        {
                            recommendation.recAct = Raise;
                            recommendation.value = 3*bet;
                        }
                        else if(((combination >= MiddlePair)&&
                                 (combination <= OverPair)) ||
                                 (outs.size() > 3)) //слабые дро и готовые руки средней силы
                        {
                            recommendation.recAct = Call;
                        }
                        else //мусор
                            recommendation.recAct = Fold;
                    }
                    else //если 1 оппонент
                    {
                        if(bet > 3*table->bigBlind) //если оппонент много ставит
                        {
                            if((combination >= TopPair) ||
                               (outs.size() > 9))
                            {
                                recommendation.recAct = Raise;
                                recommendation.value = 3*bet;
                            }
                            else
                                recommendation.recAct = Fold;
                        }
                        else //если маленькая ставка (меньше 4BB)
                        {
                            if((players[iRaiserNumber].stack() < 2.5*table->totalPot) ||
                               (players[myNumber].stack() < 2.5*table->totalPot)) //если эффективный стэк меньше, чем 2.5 банка
                            {
                                if((combination >= TopPair) ||
                                   (outs.size() > 9))
                                {
                                    recommendation.recAct = Raise;
                                    recommendation.value = players[iRaiserNumber].stack();
                                }
                                else if(outs.size() > 3)
                                {
                                    if(odds > minOdds) //ставка по шансам
                                    {
                                        recommendation.recAct = Call;
                                    }
                                    else
                                        recommendation.recAct = Fold;
                                }
                                else
                                    recommendation.recAct = Fold;
                            }
                            else
                            {
                                if((combination >= TopPair) ||
                                   (outs.size() > 9))
                                {
                                    recommendation.recAct = Raise;
                                    recommendation.value = (2*table->totalPot)/3;
                                }
                                else if(outs.size() > 3) // слабые дро
                                {
                                    if(!bDraw)
                                    {
                                        recommendation.recAct = Raise;
                                        recommendation.value = 3*bet;
                                    }
                                    else if(odds > minOdds)
                                    {
                                        recommendation.recAct = Call;
                                    }
                                    else
                                        recommendation.recAct = Fold;
                                }
                                else
                                    recommendation.recAct = Fold;
                            }
                        }
                    }
                }
                else //если было более двух повышений
                {
                    if((combination >= TwoPair) ||
                       (combination == OverPair)||
                       (outs.size() > 9))     //комбинация начиная от оверпары либо монстр-дро
                    {
                        recommendation.recAct = Raise;                  //рекомендуем ставку
                        recommendation.value = 3*bet;         //размер повышения - три ставки оппонента
                    }
                    else
                    {
                        recommendation.recAct = Fold;
                    }
                }
            }
            else //если не первый круг
            {
                if((combination >= TwoPair) ||
                   (outs.size() >= 12))
                {
                    recommendation.recAct = Raise;
                    recommendation.value = 3*bet+myBet;
                }
                else if((combination == OverPair) ||
                        ((combination == TopPair) &&
                         (kickPair.nominal >= Card::Jack)))
                {
                    if(!bDraw)
                    {
                        recommendation.recAct = Call;
                    }
                    else if(((tableCards.at(0).suit == tableCards.at(1).suit) &&
                             (tableCards.at(1).suit == tableCards.at(2).suit)) ||
                            ((tableCards.at(0).nominal == tableCards.at(1).nominal+1) ||
                             (tableCards.at(1).nominal == tableCards.at(2).nominal+1)))
                    {
                        recommendation.recAct = Fold;
                    }
                    else
                    {
                        recommendation.recAct = Raise;
                        recommendation.value = myStack+myBet;
                    }
                }
                else if(outs.size() > 3)
                {
                    odds = (qreal)(outs.size())/((qreal)47);
                    minOdds = (bet-myBet)/(table->totalPot+bet-myBet);
                    addLog(QString("odds %1 minOdds %2").arg(odds).arg(minOdds));
                    if(odds > minOdds)
                        recommendation.recAct = Call;
                    else
                        recommendation.recAct = Fold;
                }
                else
                    recommendation.recAct = Fold;
            }
            break;
        }
        case Turn: {
            odds = (qreal)(outs.size())/((qreal)46);
            minOdds = (bet-myBet)/(table->totalPot+bet-myBet);
            addLog(QString("odds %1 potOdds %2").arg(odds).arg(minOdds));
            if(circle == 1)
            {
                if(numberOfRaisers == 0)
                    recommendation.value = table->totalPot/2;
                else
                    recommendation.value = 3*bet;

                if(combination >= TwoPair)
                {
                    recommendation.recAct = Raise;
                }
                else if(outs.size() > 8)
                {
                    if(numberOfRaisers == 0)
                    {
                        if(iActivePlayers == 1)
                            recommendation.recAct = Raise;
                        else
                            recommendation.recAct = Check;
                    }
                    else
                    {
                        if((combination >= MiddlePair) ||
                           (odds > minOdds))
                            recommendation.recAct = Call;
                        else
                            recommendation.recAct = Fold;
                    }
                }
                else if(combination >= TopPair)
                {

                }
            }
            else //если не первый круг
            {

            }
        break;
        }
        case River: {
        break;
        }
        default:
        break;
        }

    }
    else if(players[myNumber].bets(PreFlop).last() == table->bigBlind)//фриплей (наша ставка равна BB)
    {
        addLog(QString("Freeplay"));

    }
    else //инициатива у оппонента
    {
        addLog(QString("Opp aggressor"));

    }

    //----Дополнительная проверка на размер ставки---------//
    if(recommendation.recAct == Raise)                              // если рекомендуется ставка
    {
        addLog(QString("check bet size"));

        recommendation.value = floor(recommendation.value * 100 + 0.5) / 100; //округляем ставку до центов
        addLog(QString("bet %1 mystack %2 recommendation.value %3").arg(bet).arg(myStack).arg(recommendation.value));
        addLog(QString("myBet %1").arg(myBet));
        if(recommendation.value > myStack/2)        //если нужно вложить больше половины своих фишек, либо рекомендовано больше стэка
            recommendation.value = myStack+myBet;         //то рекомендуем All-In

        if(recommendation.value <= bet-myBet)     //если рекомендуется ставить меньше текущей ставки за вычетом наших поставленных денег
            recommendation.recAct = Call;           //то рекомендуемое действие - Call

        iActivePlayers=0;                           //обнуляем количество активных игроков (не скинувших карты и не поставивших все фишки)
        for(int i=0;i< numberOfPlayers;i++)         //подсчитываем количество игроков не скинувших карты и не вабанк
        {   if(players[i].isActive() == true &&
               players[i].stack() != 0)
            iActivePlayers++;
        }
        addLog(QString("iActivePlayers %1").arg(iActivePlayers));
        if(iActivePlayers==1)                       //если рекомендуется повышать, а остальные игроки уже скинули либо поставили все свои фишки
        {
            recommendation.value = 0;               //обнуляем размер повышения
            recommendation.recAct = Call;           //рекомендуем колировать
        }
    }
    addLog(QString("recommendation=%1 size=%2").arg(recommendation.recAct).arg(recommendation.value));
    addLog(QString("postflopMSSadvanced() end"));

    return recommendation;
}


//------------------------------------------------------------------------
// postflopMSSbasic - Определение рекомендуемого действия по чарту MSS после префлопа
//------------------------------------------------------------------------
// возвращает:  recommendation - структура рекомендованного действия
// принимает:   table->cards  - карты стола
//              table->totalPot - текущий размер банка
//              starterNumber - номер в списке игроков, открывающий торги
//              bet - текущая ставка
//              players - список игроков
//              myNumber  - наш номер в списке игроков
//              players[m_myNumber].cards - список наших карт
//              players.size() - количество игроков, вошедших в игру
//              stage  - текущая стадия игры
//              combination - комбинация
//              circle - круг торгов на данной улице
//              outs   - количество аутсов
//              kickPair - кикер дл пары
//
Recommendation Strategy::postflopMSSbasic(Stage stage,
                                          int circle,
                                          Table *table,
                                          QList<Player> players,
                                          qreal bet,
                                          int starterNumber,
                                          int myNumber,
                                          Combination combination,
                                          QList<Card> outs,
                                          Card pairCard,
                                          Card kickPair )
{
    addLog(QString("postflopMSSbasic() start"));
    Recommendation recommendation;
    QList<Card> myCards = players[myNumber].cards();//при анализе требуется чтобы карты были отсортированы по убыванию
    sortCards(&myCards, myCards.size());
    QList<Card> tableCards = table->cards;//при анализе требуется чтобы карты были отсортированы по убыванию
    sortCards(&tableCards, tableCards.size());
    int numberOfPlayers = players.size();
    int iActivePlayers = 0; // количество активных игроков
    int numberOfRaisers = 0;  //количество повысивших игроков перед нами
    qreal myStack = players[myNumber].stack();
    qreal myBet = 0; //наша ставка на текущей стадии
    //Определяем сколько всего нами уже поставлено в банк на текущей стадии
    if(players[myNumber].bets(stage).size())
    {
        myBet=players[myNumber].bets(stage).last();
    }

    //----Подсчитываем количество активных игроков----------------------------------------//
    iActivePlayers=0;                               // обнуляем количество активных игроков
    addLog(QString("numberOfPlayers=%1").arg(numberOfPlayers));
    for(int i=0; i < numberOfPlayers; i++)                       // проходим по всем игрокам стола
    {
        if(players[i].isActive())
        {
            iActivePlayers++;                       // считаем, если текущий игрок активен
        }
    }

    //--Подсчитываем количество повысивших перед нами, плюс после нас на предыдущем круге---//
    addLog(QString("starterNumber=%1 myNumber=%2").arg(starterNumber).arg(myNumber));
    addLog(QString("circle=%1").arg(circle));
    int i = starterNumber;                  // начинаем с игрока открывающего торги
    while (i != myNumber)                   // производим анализ до момента, пока не дошли до нас
    {
        addLog(QString("i=%1").arg(i));
        if(players[i].isActive())
        {
            addLog(QString("last action=%1").arg(players[i].actions(stage).last()));
            if(players[i].actions(stage).last() == Raise )
            {
                addLog(QString("was raised"));
                numberOfRaisers++;//подсчитываем сколько человек повышало перед нами на текущем круге
            }
        }
        i++;
        if(i >= numberOfPlayers)
            i -= numberOfPlayers;
    }
    addLog(QString("numberOfRaisers=%1").arg(numberOfRaisers));
    //--На данный момент по стратегии количество повысившых не учитывается, если круг > 1---//
//    if(circle != 1)                         //если не первый круг, необходимо подсчитать игроков после нас на предыдущем
//    {
//        qDebug()<<"circle != 1";
//        i = myNumber;                       // начинаем с игрока за нами
//        do {
//            i++;
//            if(i >= numberOfPlayers)
//                i -= numberOfPlayers;
//        } while(!players[i].isActive());
//        //while (i != starterNumber)          // производим анализ до момента, пока не дошли до нас
//        while (i != myNumber)          // производим анализ до момента, пока не дошли до нас
//        {
//            if(players[i].actions(stage).last() == Raise )//проверяем предыдущий круг
//                numberOfRaisers++;//подсчитываем сколько человек повышало перед нами
//            i++;
//            if(i >= numberOfPlayers) i -= numberOfPlayers;
//        }

//    }

    //---Основной анализ-------------------------------//
    addLog(QString("my first act on preFlop=%1").arg(players[myNumber].actions(PreFlop).at(0)));
    addLog(QString("combination"));
    addLog(combinationToString(combination));
    addLog(QString("kicker card %1 %2").arg(kickPair.nominal).arg(kickPair.suit));
    addLog(QString("pair card %1 %2").arg(pairCard.nominal).arg(pairCard.suit));
    addLog(QString("outs=%1").arg(outs.size()));
    for(int i=0; i<outs.size(); i++)
    {
        addLog(QString("outs=%1 %2").arg(outs.at(i).nominal).arg(outs.at(i).suit));
    }
    addLog(QString("table->totalPot %1").arg(table->totalPot));
    addLog(QString("stage %1").arg(stage));

    if(players[myNumber].actions(PreFlop).at(0) >= Raise )                 //если мы повышали на префлопе (ставка, стил, рестил)
    {
        switch(stage) {
        case Flop:
        {
            bool bDraw=checkFlopDraw(tableCards);       //проверяем стол на дрова
            addLog(QString("table draw=%1").arg(bDraw));

            //------Определяем рекомендуемое действие-----------//
            if(bDraw == false && iActivePlayers == 2)             //если борд не дровяной (сухой) и у нас 1 оппонент
            {
                if(circle == 1)                              //на первом круге
                {
                    if(numberOfRaisers == 0)                          //если игрок перед нами не повышал либо мы ходим первыми, то
                    {
                        recommendation.recAct = Raise;                          //рекомендуется повышать
                        recommendation.value = table->totalPot/2;                //размер повышения - пол банка (продолженная ставка против одного оппонента на сухом борде)
                    }
                    else                                    //если перед нами игрок повысил
                    {
                        if((combination >= OverPair) ||
                           (combination == TopPair && pairCard.nominal == Card::Ace && kickPair.nominal > Card::Ten) ||
                           (combination == TopPair && pairCard.nominal == Card::King && kickPair.nominal == Card::Ace) ||
                           (outs.size() > 9))     //комбинация начиная от оверпары либо монстр-дро
                        {
                            if(outs.size() > 9)                 //если дро начиная от фл.дро+топ-пара
                            {
                                recommendation.recAct = Raise;                  //рекомендуем ставку
                                recommendation.value = 3*bet;         //размер повышения - три ставки оппонента
                            }
                            else //если не монстр дро
                            {
                                if(combination == OverPair && myCards.at(0).nominal < Card::Seven)//если у нас оверпара ниже 77
                                    recommendation.recAct = Fold;
                                else //если проверка на оверпару ниже 7
                                {
                                    recommendation.recAct = Raise;          //рекомендуется повышать
                                    recommendation.value = 3*bet; //размер повышения - три ставки оппонента
                                }
                            }
                        }
                        else recommendation.recAct = Fold;
                    }
                }
                else                                        //если не первый круг флопа
                {
                    if((combination >= OverPair) ||
                       (combination == TopPair && pairCard.nominal == Card::Ace && kickPair.nominal > Card::Ten) ||
                       (combination == TopPair && pairCard.nominal == Card::King && kickPair.nominal == Card::Ace) ||
                       (outs.size() > 9))     //комбинация начиная от оверпары либо монстр-дро
                    {
                        if(outs.size() > 9)                 //если дро начиная от фл.дро+топ-пара (монстр дро)
                        {
                            recommendation.recAct = Raise;                  //рекомендуем ставку в размере
                            recommendation.value = 3*bet;         //размер повышения - три ставки оппонента
                        }
                        else                            //если не монстр дро
                        {
                            if(combination == OverPair && myCards.at(0).nominal < Card::Seven)//если у нас оверпара ниже 77
                                recommendation.recAct = Fold;
                            else                    //если проверка на оверпару ниже 7 прошла
                            {
                                recommendation.recAct = Raise;          //рекомендуется повышать
                                recommendation.value = 3*bet; //размер повышения - три ставки оппонента
                            }
                        }
                    }
                    else
                        recommendation.recAct = Fold;
                }
            }
            else    //если несколько оппонентов или борд дровяной
            {
                if(circle == 1)                          //на первом круге
                {
                    if(numberOfRaisers == 0)                      //если игрок перед нами не повышал, то
                    {
                        if(combination >= TopPair || outs.size() > 7) //если наша комбинация от топ пары либо дро от OESD
                        {
                            if(outs.size() > 7)             //если дро начиная от OESD
                            {
                                recommendation.recAct = Raise;              //рекомендуем ставку в размере
                                recommendation.value = (3*table->totalPot)/4;//три четверти банка
                            }
                            else                        //если не OESD, не флеш дро, не двойной гатшот, и не монстр дро
                            {
                                if((combination == OverPair && myCards.at(0).nominal < Card::Seven) ||
                                   (combination == TopPair && (tableCards[0].nominal < Card::Ten || kickPair.nominal < Card::Jack)))//если у нас оверпара ниже 77, либо если у нас топ пара ниже 10 или кикер ниже вальта
                                    recommendation.recAct = Check;
                                else                //если проверка на оверпару ниже 7 и на топ пару ниже 10 (либо с кикером ниже вальта) прошла
                                {
                                    recommendation.recAct = Raise;      //рекомендуем ставку в размере
                                    recommendation.value = (3*table->totalPot)/4;//три четверти банка
                                }
                            }
                        }
                        else recommendation.recAct = Check;                 //если у нас комбинация не для ставки, то рекомендуем чек
                    }
                    else                                //если были ставки до нас
                    {
                        if((combination >= OverPair) ||
                           (combination == TopPair && pairCard.nominal == Card::Ace && kickPair.nominal > Card::Ten) ||
                           (combination == TopPair && pairCard.nominal == Card::King && kickPair.nominal == Card::Ace) ||
                           (outs.size() > 9)) //комбинация начиная от оверпары либо монстр-дро
                        {
                            if(outs.size() > 9)             //если дро начиная от фл.дро+топ-пара (монстр дро)
                            {
                                recommendation.recAct = Raise;              //рекомендуем ставку в размере
                                recommendation.value = 3*bet;     //размер повышения - три ставки оппонента
                            }
                            else                        //если не монстр дро
                            {
                                if(combination == OverPair && myCards.at(0).nominal < Card::Seven)//если у нас оверпара ниже 77
                                    recommendation.recAct = Fold;
                                else                //если проверка на оверпару ниже 7 прошла
                                {
                                    recommendation.recAct = Raise;           //рекомендуется повышать
                                    recommendation.value = 3*bet;  //размер повышения - три ставки оппонента
                                }
                            }
                        }
                        else recommendation.recAct = Fold;                 //если у нас комбинация не для переповышения, то рекомендуем сброс
                    }
                }
                else                                    //если не первый круг флопа (то действия аналогичные если бы мы оставались вдвоем на сухом борде)
                {
                    if((combination >= OverPair) ||
                       (combination == TopPair && pairCard.nominal == Card::Ace && kickPair.nominal > Card::Ten) ||
                       (combination == TopPair && pairCard.nominal == Card::King && kickPair.nominal == Card::Ace) ||
                       (outs.size() > 9))     //комбинация начиная от оверпары либо монстр-дро
                    {
                        if(outs.size() > 9)                 //если дро начиная от фл.дро+топ-пара (монстр дро)
                        {
                            recommendation.recAct = Raise;                  //рекомендуем ставку в размере
                            recommendation.value = 3*bet;         //размер повышения - три ставки оппонента
                        }
                        else                            //если не монстр дро
                        {
                            if(combination == OverPair && myCards.at(0).nominal < Card::Seven)//если у нас оверпара ниже 77
                                recommendation.recAct = Fold;
                            else                    //если проверка на оверпару ниже 7 и на топ пару ниже 10 (либо с кикером ниже вальта) прошла
                            {
                                recommendation.recAct = Raise;          //рекомендуется повышать
                                recommendation.value = 3*bet; //размер повышения - три ставки оппонента
                            }
                        }
                    }
                    else recommendation.recAct = Fold;
                }
            } // else несколько оппонентов или борд дровяной
            break;
        }
        case Turn:
        {
        //------Определяем рекомендуемое действие-----------//
            if(circle == 1 && numberOfRaisers == 0)                //если первый круг и никто не повышал
            {
                if(combination >= TopPair || outs.size() > 9)             //комбинация начиная от топ-пары либо монстр-дро
                {
                    if(outs.size() > 9)                         //если дро начиная от фл.дро+топ-пара
                    {
                        recommendation.recAct = Raise;                          //рекомендуем ставку в размере
                        recommendation.value = (3*table->totalPot)/4;            //размер повышения - три четверти банка
                    }
                    else                                    //если не монстр дро
                    {
                        if(combination == TwoPair &&
                           myCards.at(0).nominal == myCards.at(1).nominal &&
                           myCards.at(0).nominal < tableCards.at(0).nominal)//если комбинация 2 пары, но одна из этих пар у нас, другая на столе и наша пара ниже старшей карты стола, то разыгрываем как среднюю пару
                            recommendation.recAct = Check;                      //рекомендуем чек
                        else                                //если проверка на две пары но отыгрыш как со средней парой пройдена
                        {
                            if((combination == OverPair &&
                                myCards.at(0).nominal < Card::Seven) ||
                               (combination == TopPair && (tableCards[0].nominal < Card::Ten ||  kickPair.nominal < Card::Jack)))//если у нас оверпара ниже 77, либо если у нас топ пара ниже 10 или кикер ниже вальта
                                recommendation.recAct = Check;
                            else                            //если проверка на оверпару ниже 7 и на топ пару ниже 10 (либо с кикером ниже вальта) прошла
                            {
                                recommendation.recAct = Raise;                  //рекомендуется повышать
                                recommendation.value = (3*table->totalPot)/4;    //размер повышения - три четверти банка
                            }
                        }
                    }
                }
                else recommendation.recAct = Check;                             //если у нас ниже топ пары и не монстр-дро
            }
            else                                            //если не первый круг, либо кто-то уже поставил
            {
                if((combination >= OverPair) ||
                   (combination == TopPair && pairCard.nominal == Card::Ace && kickPair.nominal > Card::Ten) ||
                   (combination == TopPair && pairCard.nominal == Card::King && kickPair.nominal == Card::Ace) ||
                   (outs.size() > 9))             //комбинация начиная от оверпары либо монстр-дро
                {
                    if(outs.size() > 9)                         //если дро начиная от фл.дро+топ-пара (монстр дро)
                    {
                        recommendation.recAct = Raise;                      //рекомендуем ставку в размере
                        recommendation.value = 3*bet;             //размер повышения - три ставки оппонента
                    }
                    else                                //если не монстр дро
                    {
                        if(combination == TwoPair && myCards.at(0).nominal == myCards.at(1).nominal && myCards.at(0).nominal < tableCards.at(0).nominal)//если комбинация 2 пары, но одна из этих пар у нас, другая на столе и наша пара ниже старшей карты стола, то разыгрываем как среднюю пару
                            recommendation.recAct = Fold;                  //рекомендуем сброс
                        else                            //если проверка на две пары на спаренном борде прошла
                        {
                            if(combination == OverPair && myCards.at(0).nominal < Card::Seven)//если у нас оверпара ниже 77, либо если у нас топ пара ниже 10 или кикер ниже вальта
                                recommendation.recAct = Fold;
                            else                        //если проверка на оверпару ниже 7 прошла
                            {
                                recommendation.recAct = Raise;              //рекомендуется повышать
                                recommendation.value = 3*bet;     //размер повышения - три ставки оппонента
                            }
                        }
                    }
                }
                else recommendation.recAct = Fold;                         //если у нас комбинация не для переповышения, то рекомендуем сброс
            }
            break;
        }
        case River:
        {
            if( players[myNumber].actions(Turn).at(0) == Raise)        //если мы на терне делали ставку
            {
                if(circle==1 && numberOfRaisers==0)    //если первый круг и до нас никто не ставил
                {
                    if(combination >= TopPair)              //комбинация начиная от топ-пары
                    {
                        if(combination == TwoPair && myCards.at(0).nominal==myCards.at(1).nominal && myCards.at(0).nominal < tableCards.at(0).nominal)//если комбинация 2 пары, но одна из этих пар у нас, другая на столе и наша пара ниже старшей карты стола, то разыгрываем как среднюю пару
                            recommendation.recAct = Check;              //рекомендуем чек
                        else                        //если проверка на две пары но отыгрыш как со средней парой пройдена
                        {
                            if((combination == OverPair && myCards.at(0).nominal < Card::Seven)||(combination == TopPair && (tableCards[0].nominal < Card::Ten ||  kickPair.nominal < Card::Jack)))//если у нас оверпара ниже 77, либо если у нас топ пара ниже 10 или кикер ниже вальта
                                recommendation.recAct = Check;
                            else                    //если проверка на оверпару ниже 7 и на топ пару ниже 10 (либо с кикером ниже вальта) прошла
                            {
                                recommendation.recAct = Raise;          //рекомендуется повышать
                                recommendation.value = (3*table->totalPot)/4;//размер повышения - три четверти банка
                            }
                        }
                    }
                    else recommendation.recAct = Check;                 //если у нас ниже топ пары и не монстр-дро
                }
                else                                //если на ривере либо второй круг, либо уже кто-то поставил
                {
                    if(combination >= TopPair)              //комбинация начиная от топ-пары
                    {
                        if(combination == TwoPair && myCards.at(0).nominal==myCards.at(1).nominal && myCards.at(0).nominal < tableCards.at(0).nominal)//если комбинация 2 пары, но одна из этих пар у нас, другая на столе и наша пара ниже старшей карты стола, то разыгрываем как среднюю пару
                            recommendation.recAct = Fold;              //рекомендуем сброс
                        else                        //если проверка на две пары но отыгрыш как со средней парой пройдена
                        {
                            if((combination == OverPair && myCards.at(0).nominal < Card::Seven)||(combination == TopPair && (tableCards[0].nominal < Card::Ten || kickPair.nominal < Card::Jack)))//если у нас оверпара ниже 77, либо если у нас топ пара ниже 10 или кикер ниже вальта
                                recommendation.recAct = Fold;
                            else                    //если проверка на оверпару ниже 7 и на топ пару ниже 10 (либо с кикером ниже вальта) прошла
                            {
                                recommendation.recAct = Raise;          //рекомендуется повышать
                                recommendation.value = 3*bet; //размер повышения - три поставленные ставки
                            }
                        }
                    }
                    else recommendation.recAct = Fold;                 //если у нас ниже топ пары и не монстр-дро
                }
            }
            else                                    //если мы не делали ставку на терне (значит делали чек и дошли до ривера)
            {
                    if(circle==1 && numberOfRaisers==0)//если первый круг и до нас никто не ставил
                    {
                        if(combination >= TopPair)          //комбинация начиная от топ-пары
                        {
                            if(combination == TwoPair && myCards.at(0).nominal==myCards.at(1).nominal && myCards.at(0).nominal < tableCards.at(0).nominal)//если комбинация 2 пары, но одна из этих пар у нас, другая на столе и наша пара ниже старшей карты стола, то разыгрываем как среднюю пару
                                recommendation.recAct = Check;          //рекомендуем чек
                            else                    //если проверка на две пары но отыгрыш как со средней парой пройдена
                            {
                                if((combination == OverPair && myCards.at(0).nominal < Card::Seven)||(combination == TopPair && (tableCards[0].nominal < Card::Ten || kickPair.nominal < Card::Jack)))//если у нас оверпара ниже 77, либо если у нас топ пара ниже 10 или кикер ниже вальта
                                    recommendation.recAct = Check;
                                else                //если проверка на оверпару ниже 7 и на топ пару ниже 10 (либо с кикером ниже вальта) прошла
                                {
                                    recommendation.recAct = Raise;      //рекомендуется повышать
                                    recommendation.value = (3*table->totalPot)/4;//размер повышения - три четверти банка
                                }
                            }
                        }
                        else recommendation.recAct = Check;             //если у нас ниже топ пары и не монстр-дро
                    }
                    else                            //если на ривере либо второй круг, либо уже кто-то поставил
                    {
                        if(combination >= TopPair)          //комбинация начиная от топ-пары
                        {
                            if(combination == TwoPair && myCards.at(0).nominal==myCards.at(1).nominal && myCards.at(0).nominal < tableCards.at(0).nominal)//если комбинация 2 пары, но одна из этих пар у нас, другая на столе и наша пара ниже старшей карты стола, то разыгрываем как среднюю пару
                                recommendation.recAct = Fold;          //рекомендуем чек
                            else                    //если проверка на две пары но отыгрыш как со средней парой пройдена
                            {
                                if((combination == OverPair && myCards.at(0).nominal < Card::Seven)||(combination == TopPair && (tableCards[0].nominal < Card::Ten || kickPair.nominal < Card::Jack)))//если у нас оверпара ниже 77, либо если у нас топ пара ниже 10 или кикер ниже вальта
                                    recommendation.recAct = Fold;
                                else                //если проверка на оверпару ниже 7 и на топ пару ниже 10 (либо с кикером ниже вальта) прошла
                                {
                                    if(combination == TopPair){recommendation.recAct = Call;recommendation.value =  bet;}//если кто-то поставил на первом круге ривера до нас а у нас топпара то мы отвечаем
                                    else
                                    {
                                        recommendation.recAct = Raise;           //рекомендуется повышать
                                        recommendation.value = 3*bet;  //размер повышения - три поставленные ставки
                                    }
                                }
                            }
                        }
                        else recommendation.recAct = Fold;             //если у нас ниже топ пары и не монстр-дро
                    }
                }
                break;
        }
        default:
            {
                recommendation.recAct = Fold;
                break;
            }
        } //switch(stage)
    }
    else                                            //если мы не повышали на префлопе (только чек на большом блайнде)
    {
        addLog(QString("no raise on PreFlop"));
        if(stage < River)                                //если уровень ниже ривера (на префлопе функция postflopMSSbasic не вызывается поэтому уровни Флоп и Тёрн
        {
            if(circle==1 && numberOfRaisers==0)        //если никто не ставил перед нами
            {
                if(combination >= TwoPair || outs.size() > 9) // комбинация начиная от двух пар либо монстр дро
                {
                    if(outs.size() > 9)                 //если дро начиная от фл.дро+топ-пара (монстр дро)
                    {
                        recommendation.recAct = Raise;                  //рекомендуем ставку в размере
                        recommendation.value = (3*table->totalPot)/4;    //размер повышения - три четверти банка
                    }
                    else                            //если не монстр дро
                    {
                        if(combination == TwoPair && myCards.at(0).nominal == myCards.at(1).nominal && myCards.at(0).nominal < tableCards.at(0).nominal)//если комбинация 2 пары, но одна из этих пар у нас, другая на столе и наша пара ниже старшей карты стола, то разыгрываем как среднюю пару
                            recommendation.recAct = Check;              //рекомендуем чек
                        else                        //если проверка на две пары на спаренном борде прошла
                        {
                            recommendation.recAct = Raise;              //рекомендуем ставку в размере
                            recommendation.value = (3*table->totalPot)/4;//размер повышения - три четверти банка
                        }
                    }
                }
                else
                    recommendation.recAct = Check;                      //рекомендуем чек
            }
            else                                    //если перед нами была ставка или уже не первый круг
            {
                if(combination >= TwoPair || outs.size() > 9)     //если у нас рука монстр или монстр дро
                {
                    if(outs.size() > 9)                 //если дро начиная от фл.дро+топ-пара (монстр дро)
                    {
                        recommendation.recAct = Raise;                  //рекомендуем ставку в размере
                        recommendation.value = 3*bet;         //размер повышения - три ставки оппонента
                    }
                    else                            //если не монстр дро
                    {
                        if(combination == TwoPair && myCards.at(0).nominal == myCards.at(1).nominal && myCards.at(0).nominal < tableCards.at(0).nominal)//если комбинация 2 пары, но одна из этих пар у нас, другая на столе и наша пара ниже старшей карты стола, то разыгрываем как среднюю пару
                            recommendation.recAct = Fold;              //рекомендуем чек
                        else                        //если проверка на две пары на спаренном борде прошла
                        {
                            recommendation.recAct = Raise;              //рекомендуем ставку в размере
                            recommendation.value = 3*bet;     //размер повышения - три ставки оппонента
                        }
                    }
                }
                else                                //если наша рука не достаточно сильна для ответа на ставку
                    recommendation.recAct = Fold;                      //рекомендуем сброс
            }
        }
        else                                        //если уровень ривер
        {
            if(players[myNumber].actions(Turn).at(0) == Raise)         //если мы ставили на терне
            {
                if(circle==1 && numberOfRaisers==0)    //если перед нами никто не ставил и это первый круг торгов
                {
                    if(combination >= TopPair)              //если наша рука начиная от топ-пары
                    {
                        if(combination == TwoPair && myCards.at(0).nominal==myCards.at(1).nominal && myCards.at(0).nominal < tableCards.at(0).nominal)//если комбинация 2 пары, но одна из этих пар у нас, другая на столе и наша пара ниже старшей карты стола, то разыгрываем как среднюю пару
                            recommendation.recAct = Check;              //рекомендуем чек
                        else                        //если проверка на две пары но отыгрыш как со средней парой пройдена
                        {
                            if((combination == OverPair && myCards.at(0).nominal < Card::Seven)||(combination == TopPair && (tableCards[0].nominal < Card::Ten || kickPair.nominal < Card::Jack)))//если у нас оверпара ниже 77, либо если у нас топ пара ниже 10 или кикер ниже вальта
                                recommendation.recAct = Check;
                            else                    //если проверка на оверпару ниже 7 и на топ пару ниже 10 (либо с кикером ниже вальта) прошла
                            {
                                recommendation.recAct = Raise;
                                recommendation.value = (3*table->totalPot)/4; //рекомендуемый размер рейза три четверти банка
                            }
                        }
                    }
                    else recommendation.recAct = Check;                 //если силы руки не достаточно для ставки, то рекомендуется чек
                }
                else                                //если перед нами кто-то ставил либо уже второй круг торгов
                {
                    if(combination >= TopPair)              //если наша рука начиная от топ-пары
                    {
                        if(combination == TwoPair && myCards.at(0).nominal==myCards.at(1).nominal && myCards.at(0).nominal < tableCards.at(0).nominal)//если комбинация 2 пары, но одна из этих пар у нас, другая на столе и наша пара ниже старшей карты стола, то разыгрываем как среднюю пару
                            recommendation.recAct = Fold;              //рекомендуем чек
                        else                        //если проверка на две пары но отыгрыш как со средней парой пройдена
                        {
                            if((combination == OverPair && myCards.at(0).nominal < Card::Seven)||(combination == TopPair && (tableCards[0].nominal < Card::Ten || kickPair.nominal < Card::Jack)))//если у нас оверпара ниже 77, либо если у нас топ пара ниже 10 или кикер ниже вальта
                                recommendation.recAct = Fold;
                            else                    //если проверка на оверпару ниже 7 и на топ пару ниже 10 (либо с кикером ниже вальта) прошла
                            {
                                recommendation.recAct = Raise;
                                recommendation.value = 3*bet; //рекомендуемый размер рейза три ставки оппонента
                            }
                        }
                    }
                    else recommendation.recAct = Fold;                 //если силы руки не достаточно для ставки, то рекомендуется сброс
                }
            }
            else                                    //если мы не ставили на терне (делали чек)
            {
                if(circle==1 && numberOfRaisers==0)    //если перед нами никто не ставил и это первый круг торгов
                {
                    if(combination >= TopPair)              //если наша рука начиная от топ-пары
                    {
                        if(combination == TwoPair && myCards.at(0).nominal==myCards.at(1).nominal && myCards.at(0).nominal < tableCards.at(0).nominal)//если комбинация 2 пары, но одна из этих пар у нас, другая на столе и наша пара ниже старшей карты стола, то разыгрываем как среднюю пару
                            recommendation.recAct = Check;              //рекомендуем чек
                        else                        //если проверка на две пары но отыгрыш как со средней парой пройдена
                        {
                            if((combination == OverPair && myCards.at(0).nominal < Card::Seven)||(combination == TopPair && (tableCards[0].nominal < Card::Ten || kickPair.nominal < Card::Jack)))//если у нас оверпара ниже 77, либо если у нас топ пара ниже 10 или кикер ниже вальта
                                recommendation.recAct = Check;
                            else                    //если проверка на оверпару ниже 7 и на топ пару ниже 10 (либо с кикером ниже вальта) прошла
                            {
                                recommendation.recAct = Raise;
                                recommendation.value = (3*table->totalPot)/4; //рекомендуемый размер рейза три четверти банка
                            }
                        }
                    }
                    else recommendation.recAct = Check;                 //если силы руки не достаточно для ставки, то рекомендуется чек
                }
                else                                //если перед нами кто-то ставил либо уже второй круг торгов
                {
                    if(combination >= TopPair)              //если наша рука начиная от топ-пары
                    {
                        if(combination == TwoPair && myCards.at(0).nominal==myCards.at(1).nominal && myCards.at(0).nominal < tableCards.at(0).nominal)//если комбинация 2 пары, но одна из этих пар у нас, другая на столе и наша пара ниже старшей карты стола, то разыгрываем как среднюю пару
                            recommendation.recAct = Fold;              //рекомендуем чек
                        else                        //если проверка на две пары но отыгрыш как со средней парой пройдена
                        {
                            if((combination == OverPair && myCards.at(0).nominal < Card::Seven)||
                               (combination == TopPair && (tableCards[0].nominal < Card::Ten ||
                                                             kickPair.nominal < Card::Jack)))//если у нас оверпара ниже 77, либо если у нас топ пара ниже 10 или кикер ниже вальта
                                recommendation.recAct = Fold;
                            else                    //если проверка на оверпару ниже 7 и на топ пару ниже 10 (либо с кикером ниже вальта) прошла
                            {
                                if(combination == TopPair)
                                {
                                    recommendation.recAct = Call; //если топ пара рекомендуем Call
                                    recommendation.value = bet;
                                }
                                else
                                {
                                    recommendation.recAct = Raise;
                                    recommendation.value = 3*bet; //рекомендуемый размер рейза три ставки оппонента
                                }
                            }
                        }
                    }
                    else recommendation.recAct = Fold;                 //если силы руки не достаточно для ставки, то рекомендуется сброс
                }
            }
        }
    }

    //----Дополнительная проверка на размер ставки---------//
    if(recommendation.recAct == Raise)                              // если рекомендуется ставка
    {
        addLog(QString("check bet size"));

        recommendation.value = floor(recommendation.value * 100 + 0.5) / 100; //округляем ставку до центов
        addLog(QString("bet %1 mystack %2 recommendation.value %3").arg(bet).arg(myStack).arg(recommendation.value));
        addLog(QString("myBet %1").arg(myBet));
        if(recommendation.value > myStack/2)        //если нужно вложить больше половины своих фишек, либо рекомендовано больше стэка
            recommendation.value = myStack;         //то рекомендуем All-In

        if(recommendation.value <= bet-myBet)     //если рекомендуется ставить меньше текущей ставки за вычетом наших поставленных денег
            recommendation.recAct = Call;           //то рекомендуемое действие - Call

        iActivePlayers=0;                           // обнуляем количество активных игроков (не скинувших карты и не поставивших все фишки)
        for(int i=0;i< numberOfPlayers;i++)                   //подсчитываем количество игроков не скинувших карты и не вабанк
        {   if(players[i].isActive() == true &&
               players[i].stack() != 0)
            iActivePlayers++;
        }
        addLog(QString("iActivePlayers %1").arg(iActivePlayers));
        if(iActivePlayers==1)                       //если рекомендуется повышать, а остальные игроки уже скинули либо поставили все свои фишки
        {
            recommendation.value = 0;               //обнуляем размер повышения
            recommendation.recAct = Call;           //рекомендуем колировать
        }
    }
    addLog(QString("recommendation=%1 size=%2").arg(recommendation.recAct).arg(recommendation.value));
    addLog(QString("postflopMSSbasic() end"));

    return recommendation;
}

