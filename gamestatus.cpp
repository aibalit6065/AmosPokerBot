#include "gamestatus.h"

GameStatus::GameStatus()
{
    m_players.clear();
    m_buttonSeatNumber = -1;//номер места за столом, где сидит диллер
    m_myNumber = -1;  //номер нашего игрока в списке игроков
    m_starterNumber = -1; //номер игрока, открывающего торги

    m_stage = NoStage;
    m_circle = 0;

    m_bet = 0;
    m_kickPair.nominal = Card::NoNominal;
    m_kickPair.suit = Card::NoSuit;
    m_pairCard.nominal = Card::NoNominal;
    m_pairCard.suit = Card::NoSuit;
    m_combination = NoCombination;
    m_outs.clear();

    m_sitOut = false;
    m_sitOutNextBB = false;
    m_rebuy = false;

    strategy = new Strategy();
}

GameStatus::~GameStatus()
{
    delete strategy;
}

Recommendation GameStatus::analyzeGame(Table *table)
{
    Recommendation recommendation;// рекомендуемое действие, включая размер рэйза либо докупки
    int buttonSeatNumber = getButtonSeatNumber(table->playersData);// место диллера за столом
    int myNumber = getMyNumber(table); // наш номер в списке игроков из table

    //Инициализация (стадия, круг, список игроков):
    //если не инициализированы место диллера или наш номер
    //если нас нет в списке игроков из table
    //если диллер поменялся (новая раздача)
    if( m_buttonSeatNumber == -1 ||
        m_myNumber == -1 ||
        myNumber == -1 ||
       (m_buttonSeatNumber != buttonSeatNumber &&
        buttonSeatNumber   != -1))
    {
        m_stage = NoStage;
        m_circle = 0;

        //Пересоздаем список игроков только если мы присутствуем
        if(myNumber != -1)
            setPlayers(table);

        strategy->initStackParams(table->bigBlind);

        //Проверка на докупку/выход
        if(m_myNumber != -1 &&
           m_players.size() != 0)
        {
            //Если диллер только что поменялся (новый кон)
            if(m_buttonSeatNumber != buttonSeatNumber && buttonSeatNumber != -1)
            {
                //Проверка на выход
                if(strategy->sitOutByPlayers(m_players, m_sitOut))
                {
                    m_sitOut = true;
                    recommendation.recAct = SitOut;
                    {
                        qDebug()<<"saveScreenShot()";
//                        emit saveScreenShot();
                    }
//                    addLog(QString("recommend SitOut, players %1").arg(m_players.size()));
//                    qDebug()<<"my name"<<m_players[m_myNumber].name();
                }

                //Проверка на докупку
                if(recommendation.recAct != SitOut )
                {
//                    addLog(QString("my stack=%1").arg(m_players[m_myNumber].stack()));
                    if(m_players[m_myNumber].stack() < m_stackMin)
                    {
//                        addLog(QString("Stack < Minimal"));
                        if(!m_rebuy) //если еще не было докупок
                        {
//                            for(int i=0; i<m_players.size(); i++)
//                            {
//                                addLog(QString("%1 stack=%2").arg(i).arg(m_players[i].stack()));
//                            }
                            m_rebuy = true;
                            recommendation.recAct = Rebuy;
                            recommendation.value = m_stackRebuy - m_players[m_myNumber].stack();
//                            addLog(QString("recommend Rebuy %1").arg(recommendation.value));
                        }
                        else //если уже докупались, то рекомендуем выход со стола
                        {
                            m_sitOut = true;
                            recommendation.recAct = SitOut;
//                            addLog(QString("recommend SitOut"));
                        }
                    }
                }
            }
        }

        m_buttonSeatNumber = buttonSeatNumber;
        return recommendation;
    }

    //Рекомендуем выход из-за стола, если размер нашего стэка стал больше верхнего предела и еще не рекомендовалось выходить
    if( strategy->sitOutByStack(m_players[m_myNumber].stack(), m_sitOutNextBB) )
    {
        m_sitOutNextBB = true;
        recommendation.recAct = SitOutNextBB;
        return recommendation;
    }

    //Выбор режима в зависимости от наличия у нас карт
    switch(gameMode(table)){
    case StartMode:
//        addLog("StartMode:");

        //Инициализировать список игроков
        setPlayers(table);

        //Выход, если мы не текущий игрок
        if(!m_players[m_myNumber].isCurrent())
        {
            recommendation.recAct = NoAction;
            break;
        }
//        addLog(QString("m_players[m_myNumber].position()=%1").arg(m_players[m_myNumber].position()));

        //Ставим, только если мы на блайндах
        if(m_players[m_myNumber].position() == Pos_BB ||
           m_players[m_myNumber].position() == Pos_SB)
        {
            if ( m_players[m_myNumber].stack() > strategy->m_stackMax )
            {
                m_sitOut = true;
                recommendation.recAct = SitOut;
            }
            else
                recommendation.recAct = PostBlind;
        }
        else
            recommendation.recAct = WaitForBB;
        break;
    case PlayMode: {
        Stage stage = checkStage(table);
//        addLog(QString("PlayMode m_stage:%1 stage:%2").arg(m_stage).arg(stage));
        if( stage != m_stage )
        {
            //Присваивание действий игрокам, не обработанным на предыдущей стадии
            if( (stage == m_stage+1) &&
                (stage > PreFlop) )
            {
                assignLostPlayerActions(stage, table);
            }

            //Обновляем на новой стадии
            m_stage = stage;
            m_circle = 0;
            m_bet = -1;
            m_starterNumber = -1;
            m_currentNumber = -1;
        }

        switch (m_stage) {
        case PreFlop:
            if(m_circle == 0)           // если еще не было инициализации, составляем список игроков,
            {
                setPlayers(table);      // создаем список игроков из списка в Table (m_myNumber определяется внутри)
            }
            else
            {
                updatePlayers(table);   // если уже была инициализация, то обновляем список игроков
            }

            assignPlayerActions(table);          //присваиваем игрокам действия, согласно текущей ситуации

            if(m_players[m_myNumber].isCurrent())//если мы - текущий игрок, то определяем рекомендуемое действие
                recommendation = strategy->preflopMSSadvancedFR(m_circle,
                                                                table,
                                                                m_players,
                                                                m_bet,
                                                                m_starterNumber,
                                                                m_myNumber);
            else
                recommendation.recAct = NoAction;
            break;
        case Flop:
            updatePlayers(table);                //обновляем список игроков
            assignPlayerActions(table);          //присваиваем игрокам действия, согласно текущей ситуации
            checkCombination(table->cards, m_players[m_myNumber].cards());//определяем текущую комбинацию
            if(m_players[m_myNumber].isCurrent())
                recommendation = strategy->postflopMSSbasic( m_stage,
                                                             m_circle,
                                                             table,
                                                             m_players,
                                                             m_bet,
                                                             m_starterNumber,
                                                             m_myNumber,
                                                             m_combination,
                                                             m_outs,
                                                             m_pairCard,
                                                             m_kickPair );
            else
                recommendation.recAct = NoAction;
            break;
        case Turn:
            updatePlayers(table);
            assignPlayerActions(table);
            checkCombination(table->cards, m_players[m_myNumber].cards());
            if(m_players[m_myNumber].isCurrent())
                recommendation = strategy->postflopMSSbasic( m_stage,
                                                             m_circle,
                                                             table,
                                                             m_players,
                                                             m_bet,
                                                             m_starterNumber,
                                                             m_myNumber,
                                                             m_combination,
                                                             m_outs,
                                                             m_pairCard,
                                                             m_kickPair );
            else
                recommendation.recAct = NoAction;
            break;
        case River:
            updatePlayers(table);
            assignPlayerActions(table);
            checkCombination(table->cards, m_players[m_myNumber].cards());
            if(m_players[m_myNumber].isCurrent())
                recommendation = strategy->postflopMSSbasic( m_stage,
                                                             m_circle,
                                                             table,
                                                             m_players,
                                                             m_bet,
                                                             m_starterNumber,
                                                             m_myNumber,
                                                             m_combination,
                                                             m_outs,
                                                             m_pairCard,
                                                             m_kickPair );
            else
                recommendation.recAct = NoAction;
            break;
        default:
            recommendation.recAct = NoAction;
            recommendation.value = 0;
            break;
        }


        break;
    }
    case NoMode:
        recommendation.recAct = NoAction;
        break;
    }

//    addLog(QString("m_circle=%1 m_stage=%2").arg(m_circle).arg(m_stage));

    return recommendation;
}

int GameStatus::getButtonSeatNumber(QList<PlayerData> playersData)
{
    for(int i=0; i<playersData.size(); i++)
    {
        if(playersData[i].button)
            return playersData[i].seatNumber;
    }

    return -1;
}

int GameStatus::getMyNumber(Table *table)
{
    for(int i=0; i<table->playersData.size(); i++)
    {
        //Если наш игрок присутствует в списке игроков возвращаем его номер
        if(table->playersData[i].seatNumber == table->playerSeatNumber)
            return i;
    }

    //Если не обнаружили нас возвращаем -1
    return -1;
}

GameMode GameStatus::gameMode(Table *table)
{
//    addLog(QString("gameMode"));

    //Если у нас нет карт, то режим входа в игру, если есть карты то - игра
    for(int i=0; i<table->playersData.size(); i++)
    {
       if(table->playersData[i].seatNumber == table->playerSeatNumber)
       {
           //addlog(QString("myCard.size=%1").arg(table->playersData[i].cards.size()));
           //addlog(QString("myCard[1].nominal=%1 myCard[2].nominal=%2").arg(table->playersData[i].cards.at(0).nominal).arg(table->playersData[i].cards.at(1).nominal));
           //addlog(QString("myCard[1].suit=%1 myCard[2].suit=%2").arg(table->playersData[i].cards.at(0).suit).arg(table->playersData[i].cards.at(1).suit));

           int myCardsNumber = 0;
           for (int j=0; j<table->playersData[i].cards.size(); j++)
           {
               if(table->playersData[i].cards.at(j).nominal != Card::NoNominal)
                   myCardsNumber++;
           }
           if(myCardsNumber == 2)
               return PlayMode;
           else
               return StartMode;
       }
    }

    //Если не нашли нас в списке игроков
    return NoMode;
}

Stage GameStatus::checkStage(Table *table)
{
    int tableCardsNumber=0;

    for(int i=0; i<table->cards.size(); i++)
    {
        if(table->cards.at(i).nominal != 0)
        {
            tableCardsNumber++;
//            //addlog(QString("%1 %2").arg(table->cards.at(i).nominal).arg(table->cards.at(i).suit));
        }
    }

    switch(tableCardsNumber) {
    case 0: return PreFlop;
    case 3: return Flop;
    case 4: return Turn;
    case 5: return River;
    default: return NoStage;
    }
}



//------------------------------------------------------------------------
// setPlayers   - составление списка игроков
//------------------------------------------------------------------------
// принимает:   table - класс стола, содержащий информацию, полученную с картинки
// возвращает:  -1 если проблемы с присвоением позиций
//               1 если позиции присвоены успешно
// изменяет:    m_players - список игроков
//              m_myNumber - наш номер в списке игроков
//
int GameStatus::setPlayers(Table *table)
{
    Player player;
    bool myNumberFound = false;
    int status = -1;
    m_players.clear();

    for(int i=0; i<table->playersData.size(); i++)
    {
        player.setPlayerData(table->playersData[i]);
        player.setActive(true);
        player.setStats();
        m_players.append(player);

        //Определяем наш номер в списке игроков
        if(table->playersData[i].seatNumber == table->playerSeatNumber)
        {
            m_myNumber = i;
            myNumberFound = true;
        }
    }

    //Если не нашли нас в списке игроков
    if(!myNumberFound)
        m_myNumber = -1;

    status = assignPlayerPositions(&m_players);//присваиваем игрокам позиции

    return status;
}

int GameStatus::assignPlayerPositions(QList<Player> *players)
{
    int numberOfPlayers = players->size();
    int buttonNumber = -1;                 //номер диллера в списке игроков

    //Выходим, если плохой список с игроками
    if(numberOfPlayers < 2 || numberOfPlayers > 10)
        return -1;

    //Находим номер диллера в списке игроков
    for( int i = 0; i < numberOfPlayers; i++ )
    {
        if((*players)[i].isButton())
        {
            buttonNumber = i;
            break;
        }
    }

    //Если диллер не обнаружен, то выходим
    if(buttonNumber == -1)
        return -1;

    if( numberOfPlayers == 2) //ToDo: неправильно должен выставляться начальный игрок в assignPlayerActions
    {
        (*players)[buttonNumber].setPosition( Pos_SB ); //когда 2 игрока, то диллер имеет позицию малого блайнда
        (*players)[buttonNumber].setNumber(2); //ToDo: проверить, возмножна некорректная работа preflopMMS функции
        int i = buttonNumber+1;
        if(i>=numberOfPlayers)
            i-=numberOfPlayers;
        (*players)[i].setPosition( Pos_BB );
        (*players)[i].setNumber(1); //ToDo: проверить, возмножна некорректная работа preflopMMS функции
    }
    else
    {
        int j = 0;                                 //переменная, для зацикливания перебора игроков
        for( int i = 0; i < numberOfPlayers; i++ ) // проходим по всем игрокам для присвоения каждому игроку номера начиная после диллера
        {
            j=i+buttonNumber+1;                    //отсчет начинается с игрока после диллера
            if(j>=numberOfPlayers)
                j-=numberOfPlayers;                //зацикливание игроков при присвоении номеров
            (*players)[j].setNumber(i+1);

            if( i+1 == numberOfPlayers )
                (*players)[j].setPosition( Pos_BU );
            else if( i+1 == 1 )
                (*players)[j].setPosition( Pos_SB );
            else if( i+1 == 2 )
                (*players)[j].setPosition( Pos_BB );
            else if( i+1 == numberOfPlayers - 1 )
                (*players)[j].setPosition( Pos_CO );
            else if( i+1 > numberOfPlayers - 5 )
                (*players)[j].setPosition( Pos_MP );
            else
                (*players)[j].setPosition( Pos_UTG );
        }
    }

    return 1;
}

void GameStatus::updatePlayers(Table *table)
{
    bool updated = false;

    for(int i=0; i<m_players.size(); i++)
    {
       updated = false;
       for(int j=0; j<table->playersData.size(); j++)
       {
           if(m_players[i].seatNumber() == table->playersData[j].seatNumber)
           {
               m_players[i].setPlayerData(table->playersData[j]);
               updated = true;
           }
       }
       if(updated != true) // игрок не найдет
       {
           m_players[i].setBet(0);
           m_players[i].setCurrent(false);
//           qDebug()<<"not found - m_players["<<i<<"].bet()="<<m_players[i].bet();
       }
    }
}

//------------------------------------------------------------------------
// assignPlayerActions - присвоение действий игрокам, испускает сигнал о скриншоте
//------------------------------------------------------------------------
// принимает:   table - класс стола, содержащий информацию, полученную с картинки
// возвращает:  -1 в случае, если не обнаружен диллер или текущий игрок
//               1 в случае успеха
// изменяет:    m_circle - круг торгов (проверяет на ноль)
//              m_bet - текущая ставка на столе
//              m_starterNumber - номер игрока, открывающего торги
//              m_currentNumber - номер текущего игрока
//              m_players - присваивает действия
//
int GameStatus::assignPlayerActions(Table *table)
{
//    addLog(QString("assignPlayerActions() start"));

    int buttonNumber = -1; // номер диллера в списке игроков
    int numberOfPlayers = m_players.size();
    int currentNumber = getCurrentNumber(m_players);

    if (currentNumber == -1)
    {
//        addLog(QString("currentNumber not found"));
        return -1;
    }

    for( int i = 0; i < numberOfPlayers; i++ )
    {
        if(m_players[i].isButton()) // определяем номер диллера в списке игроков
            buttonNumber = i;
    }
    if (buttonNumber == -1)
    {
//        addLog(QString("buttonNumber not found"));
        return -1;
    }

//    addLog(QString("m_circle=%1 m_stage=%2 buttonNumber=%3 currentNumber=%4").arg(m_circle).arg(m_stage).arg(buttonNumber).arg(currentNumber));
    if(m_circle == 0)  // если еще не было проверок на данной стадии
    {
        m_circle = 1;
        if(m_stage == PreFlop) // если проверка производится на префлопе
        {
//            addLog(QString("PreFlop"));
            m_bet=table->bigBlind;            // ставка
            m_starterNumber = buttonNumber+3; //на префлопе при инициализации предполагается, что в списке только активные
            if(m_starterNumber>=numberOfPlayers)
                m_starterNumber-=numberOfPlayers;  // проверка на зацикливаемость
//            addLog(QString("m_starterNumber=%1 m_bet=%2").arg(m_starterNumber).arg(m_bet));
        }
        else
        {
//            addLog(QString("Not PreFlop"));
            m_bet=0;
            m_starterNumber = buttonNumber;
            do {
                m_starterNumber++;      // переходим к следующему игроку
            if(m_starterNumber>=numberOfPlayers) m_starterNumber-=numberOfPlayers;  // проверка на зацикливаемость
            } while(!m_players[m_starterNumber].isActive());
//            addLog(QString("m_starterNumber=%1 m_bet=%2").arg(m_starterNumber).arg(m_bet));
        }
        m_currentNumber = m_starterNumber;
        if(m_currentNumber == m_myNumber)
        {
            qDebug()<<"saveScreenShot()";
//            emit saveScreenShot();
        }
    }

//    addLog(QString("m_currentNumber=%1").arg(m_currentNumber));
    if(m_currentNumber != currentNumber)
    {
        int i = m_currentNumber;
        while(i != currentNumber)
        {
//            addLog(QString("m_currentNumber=%1 i=%2 currentNumber=%3").arg(m_currentNumber).arg(i).arg(currentNumber));
            if(m_players[i].isActive())
            {
//                addLog(QString("m_players[%1].bet()=%2 m_bet=%3").arg(i).arg(m_players[i].bet()).arg(m_bet));
                if(m_players[i].bet() < m_bet)
                {
                    if(m_players[i].stack()==0)
                     (m_players)[i].addAction(m_stage, Call, m_players[i].bet());
                    else
                    {
                       m_players[i].addAction(m_stage, Fold, 0);
                       m_players[i].setActive(false);
                    }
                }
                else if(m_players[i].bet() == m_bet) // если ставка игрока равна предыдущей ставки, то действие Call (2)
                {
                    if((m_bet==0) ||
                      (m_players[i].position()==Pos_BB && m_stage==PreFlop)) // если ставка равна нулю, либо текущий игрок ББ и никто не повышал
                        m_players[i].addAction(m_stage, Check, m_bet);
                    else
                        m_players[i].addAction(m_stage, Call, m_bet);
                }
                else if(m_players[i].bet() > m_bet)
                {
                    m_bet=m_players[i].bet();     // присваиваем размер рэйза
                    m_players[i].addAction(m_stage, Raise, m_bet);

                    if(m_stage==PreFlop)                        // если префлоп проверка на Стил, Рестил
                    {
                        if(m_circle==1)                  // проверка на стил/рестил, если первый круг
                        {
                            int iActive=0;       // количество активных игроков
                            int j=m_starterNumber; // начало проверки с игрока после BB
                            while(j!=i)                 // считаем количество активных и неактивных игроков до повысившего игрока
                            {
                                if(m_players[j].isActive()) // если статус игрока True
                                 iActive++;    // увеличиваем количество активных игроков
                             j++;                    //  переходим к следующему игроку
                             if(j>=numberOfPlayers)           // зацикливаем по величине стола
                                 j-=numberOfPlayers;
                            }
                            if(m_players[i].position()>Pos_MP &&
                                m_players[i].position()<Pos_BB)  // если повысивший игрок сделал ставку из поздней позиции либо с SB
                            {
                            if(iActive == 0)                // если количество активных игроков перед повысившим равно нулю
                                 m_players[i].setAction(m_stage, BlindSteal, m_bet, m_circle); // присваиваем действие стил
                            }
                            if(m_players[i].position()==Pos_BB)     // если текущий игрок BB производим проверку на рестил
                            {
                             bool bSteal=false;          //переменная флаг, был или не было стила
                             j=m_starterNumber;
                             while(j!=i)                 //проходим по всем игрокам до текущего и если был стил, то поднимаем флаг
                             {
                                 if(m_players[i].actions(m_stage).at(0)==BlindSteal)
                                    bSteal=true;        // поднимаем флаг стила
                                 j++;
                                 if(j>=numberOfPlayers)
                                     j-=numberOfPlayers;
                             }
                             if((iActive==1)&&(bSteal==true)) //если кроме текущего игрока остался только 1 и он делал стил, то
                                 m_players[i].setAction(m_stage, BlindDefense, m_bet, m_circle);  // присваиваем действие Рестил
                            }
                        }
                    }
                }
            }

            i++;
            if(i>=numberOfPlayers) i-=numberOfPlayers;  // проверка на зацикливаемость
        };

        //Проверка на переключение круга (дополнительный цикл, т.к. в предыдущем выход происходит на одного игрока раньше)
        //ToDo: возможно вынести в отдельную функцию
        i = m_currentNumber;
        do {
            i++;
            if(i>=numberOfPlayers) i-=numberOfPlayers;  // проверка на зацикливаемость

            if( i == m_starterNumber &&
                m_currentNumber != m_starterNumber )
            {
//                addLog(QString("m_circle++ m_starterNumber=%1").arg(m_starterNumber));
                m_circle++;
                break;
            }
        } while(i!=currentNumber);

        if(currentNumber == m_myNumber)
        {
            qDebug()<<"saveScreenShot()";
//            emit saveScreenShot();
        }
        m_currentNumber = currentNumber;
    }
//    addLog(QString("assignPlayerActions() end"));
    return 1;
}

//------------------------------------------------------------------------
// assignLostPlayerActions - присвоение действий при переходе на новую стадию
//------------------------------------------------------------------------
// принимает:   table - класс стола, содержащий информацию, полученную с картинки
//              stage - стадия
// возвращает:  -1 в случае неверных
//               1 в случае успеха
// изменяет:    m_circle - круг торгов (проверяет на ноль)
//              m_bet - текущая ставка на столе
//              m_starterNumber - номер игрока, открывающего торги
//              m_currentNumber - номер текущего игрока
//              m_players - присваивает действия
//
int GameStatus::assignLostPlayerActions(Stage stage, Table *table)
{
//    addLog(QString("assignLostPlayerActions() start"));

    if(stage != m_stage+1 ||
       stage < Flop ||
       m_stage < PreFlop ||
       m_starterNumber < 0)
    {
//        addLog("wrong arguments");
        return -1;
    }
    else
    {
        //Определяем номер последнего игрока, сделавшего повышение
//        addLog("adding action");
//        addLog(QString("m_starterNumber=%1").arg(m_starterNumber));
        int raiserNumber = -1;
        for(int j=0, i=m_starterNumber; j<m_players.size(); j++, i++)
        {
//            if(i>=m_players.size())
//                i-=m_players.size();
//            addLog(QString("seatNumber=%1 act size=%2").arg(m_players[i].seatNumber()).arg(m_players[i].actionStageSize()));

            if(m_players[i].actionStageSize() >= m_stage)
            {
                if(m_players[i].actions(m_stage).size())
                {
                    if(m_players[i].actions(m_stage).last() >= Raise)
                        raiserNumber = i;
                }
            }
        }
//        addLog(QString("raiserNumber=%1").arg(raiserNumber));

        if(raiserNumber == -1) //Если не было повышений
        {
            for(int j=0, i=m_starterNumber-1; j<m_players.size(); j++, i--)
            {
                if(i<0)
                    i+=m_players.size();

//                addLog(QString("player=%1 activity=%2").arg(i).arg(m_players[i].isActive()));

                if(m_players[i].isActive())
                {
                    bool activity = false;
                    for(int k=0; k<table->playersData.size(); k++)
                    {
                        if( m_players[i].seatNumber() == table->playersData[k].seatNumber )
                            activity = true;
                    }

//                    addLog(QString("new activity=%1").arg(activity));
                    if(!activity)//Если игрок на новой стадии неактивен
                    {
//                        addLog(QString("player=%1 added action Fold").arg(i));
                        m_players[i].addAction(m_stage, Fold, 0);
                        m_players[i].setActive(false);
                    }
                    else
                    {
//                        addLog(QString("player=%1 actions size=%2").arg(i).arg(m_players[i].actions(m_stage).size()));
                        if(m_players[i].actions(m_stage).size())//Если дошли до игрока, у которого есть действия
                        {
//                            addLog(QString("break"));
                            break;                              //то выходим из цикла
                        }
                        else
                        {
                            if(m_stage == PreFlop)
                            {
//                                addLog(QString("PreFlop"));
                                if(m_players[i].position() == Pos_BB)
                                {
//                                    addLog(QString("player=%1 added action Check").arg(i));
                                    m_players[i].addAction(m_stage, Check, table->bigBlind);
                                }
                                else
                                {
//                                    addLog(QString("player=%1 added action Call").arg(i));
                                    m_players[i].addAction(m_stage, Call, table->bigBlind);
                                }
                            }
                            else
                            {
//                                addLog(QString("player=%1 added action Check").arg(i));
                                m_players[i].addAction(m_stage, Check, 0);
                            }
                        }
                    }
                }
            }
        }
        else //Если были повышения
        {
//            addLog(QString("raiser=%1 action size=%2").arg(raiserNumber).arg(m_players[raiserNumber].actions(m_stage).size()));
            bool afterStarter = false;
            if(raiserNumber == m_starterNumber)
                afterStarter = true;

            for(int j=0, i=raiserNumber-1; j<m_players.size(); j++, i--)
            {
                if(i<0)
                    i+=m_players.size();

                if(m_players[i].isActive())
                {
                    bool activity = false;
                    for(int k=0; k<table->playersData.size(); k++)
                    {
                        if( m_players[i].seatNumber() == table->playersData[k].seatNumber )
                            activity = true;
                    }

                    if(!activity)//Если игрок на новой стадии неактивен
                    {
//                        addLog(QString("player=%1 added action Fold").arg(i));
                        m_players[i].addAction(m_stage, Fold, 0);
                        m_players[i].setActive(false);
                    }
                    else
                    {
//                        addLog(QString("action size=%1").arg(m_players[i].actions(m_stage).size()));
                        if(afterStarter)
                        {
                            if(m_players[i].actions(m_stage).size() < m_players[raiserNumber].actions(m_stage).size())
                            {
//                                addLog(QString("player=%1 added action Call").arg(i));
                                m_players[i].addAction(m_stage, Call, m_players[raiserNumber].bets(m_stage).last());
                            }
                            else
                            {
//                                addLog(QString("break"));
                                break;
                            }
                        }
                        else
                        {
                            if(m_players[i].actions(m_stage).size() < m_players[raiserNumber].actions(m_stage).size()+1)
                            {
//                                addLog(QString("player=%1 added action Call").arg(i));
                                m_players[i].addAction(m_stage, Call, m_players[raiserNumber].bets(m_stage).last());
                            }
                            else
                                break;
                        }
                    }
                }

                if(i == m_starterNumber)
                    afterStarter = true;
            }
        }
    }

//    addLog(QString("assignPlayerActions() end"));
    return 1;
}

int GameStatus::getCurrentNumber(QList<Player> players)
{
    for(int i=0; i<players.size(); i++)
    {
        //Если игрок текущий, то возвращаем его номер
        if(players[i].isCurrent())
            return i;
    }

    //Если не обнаружили текущего игрока возвращаем -1
    return -1;
}

QMap<QString, QString> GameStatus::getStrategyData()
{
    QMap<QString, QString> strategyData;
//    qDebug()<<"getStrategyData: 1"<<"m_stage"<<m_stage<<m_circle<<m_buttonSeatNumber<<m_myNumber;

    strategyData.insert("stage",QString::number(m_stage));
    strategyData.insert("circle",QString::number(m_circle));
    strategyData.insert("buttonSeatNumber",QString::number(m_buttonSeatNumber));
    strategyData.insert("myNumber",QString::number(m_myNumber));

//    qDebug()<<"getStrategyData: 2"<<m_players.size();
    for(int i=0; i<m_players.size(); i++)
    {
        QString player;
//        player.append("position "+QString::number(m_players[i].position())+" ");
        player.append(QString::number(m_players[i].position())+" ");
//        player.append("bet "+QString::number(m_players[i].bet())+" ");
        player.append(QString::number(m_players[i].bet())+" ");
//        player.append(QString::number((char)m_players[i].isButton())+" ");
//        player.append(QString::number((char)m_players[i].isCurrent())+" ");
//        player.append("stack "+QString::number(m_players[i].stack())+" ");
        player.append(QString::number(m_players[i].stack())+" ");
//        player.append("seatNumber "+QString::number(m_players[i].seatNumber())+";");
        player.append(QString::number(m_players[i].seatNumber())+";");
        if(m_players[i].actionStageSize())
        {
            for(int j=1; j<=m_players[i].actionStageSize(); j++)
            {
                switch(j){
                case PreFlop:
                    player.append("PreFlop ");
                    break;
                case Flop:
                    player.append("Flop ");
                    break;
                case Turn:
                    player.append("Turn ");
                    break;
                case River:
                    player.append("River ");
                    break;
                }

                for(int k=0; k<m_players[i].actions((Stage)j).size(); k++)
                {
                    switch(m_players[i].actions((Stage)j).at(k)){
                    case Fold:
                        player.append("Fold ");
                        break;
                    case Check:
                        player.append("Check ");
                        break;
                    case Call:
                        player.append("Call ");
                        break;
                    case Raise:
                        player.append("Raise ");
                        break;
                    case BlindSteal:
                        player.append("BlindSteal ");
                        break;
                    case BlindDefense:
                        player.append("BlindDefense ");
                        break;
                    default:
                        player.append("Unknown"+QString::number(m_players[i].actions((Stage)j).at(k)));
                        break;
                    }
                }
            }
        }

        strategyData.insert("strategyPlayer"+QString::number(i), player);
    }

    return strategyData;
}

// Тестовая отладочная функция
void GameStatus::testCombination()
{
    QList<Card> myCards;
    QList<Card> boardCards;
    Card card;

    card.nominal = Card::Two;
    card.suit = Card::Hearts;
    myCards<<card;
    card.nominal = Card::Three;
    card.suit = Card::Hearts;
    myCards<<card;

    card.nominal = Card::Queen;
    card.suit = Card::Diamonds;
    boardCards<<card;
    card.nominal = Card::Eight;
    card.suit = Card::Hearts;
    boardCards<<card;
    card.nominal = Card::Four;
    card.suit = Card::Hearts;
    boardCards<<card;
//    card.nominal = Card::Queen;
//    card.suit = Card::Clubs;
//    boardCards<<card;

    checkCombination(myCards,boardCards);
    qDebug()<<m_outs.size();
    for(int i=0; i<m_outs.size(); i++)
    {
        qDebug()<<m_outs.at(i).nominal<<m_outs.at(i).suit;
    }
}

int GameStatus::sortCards(QList<Card> *cards, int size)
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

//-----------------------------------------------------------------------------
// Определение комбинации и составление списка аутсов
//-----------------------------------------------------------------------------
void GameStatus::checkCombination(QList<Card>boardCard, QList<Card>myCard) // использует списки известных карт (собственных и на столе) и структуру card
{
    /****Объединяем все карты****/
    QList<Card> allCards;     // список всех известных карт
    QList<Card> lStDrawOuts;  // список аутов на стрит
    QList<Card> lFlDrawOuts;  // список аутов на флеш
    QList<Card> lSetOuts;     // список аутов на сет (из топ-пары)
    QList<Card> lOuts;        // список всех аутов

    m_pairCard.nominal = Card::NoNominal;
    m_pairCard.suit = Card::NoSuit;
    m_kickPair.nominal = Card::NoNominal;
    m_kickPair.suit = Card::NoSuit;

    allCards<<myCard;     // добавляем к общему списку карт список наших карт
    allCards<<boardCard;  // добавляем к общему списку карт список карт стола

    /****Сортируем по убыванию списки с картами****/
    sortCards(&allCards, allCards.size());
    sortCards(&myCard, myCard.size());
    sortCards(&boardCard, boardCard.size());

    /****Проверяем наличие флеш рояля****************************/
    if(checkRoyalFlush(allCards))
        m_combination=RoyalFlush;
    else // если не флеш рояль
    /****Проверяем наличие стрит флеша***************************/
    if(checkStraightFlush(allCards))
        m_combination=StraightFlush;
    else // если не стрит флеш
    /****Проверяем наличие карэ**********************************/
    if(checkQuads(allCards))
        m_combination=FourOfKind;
    else // если не каре
    /****Проверяем наличие фулхауса******************************/
    if(checkFullHouse(allCards))
        m_combination=FullHouse;
    else // если не фулхаус
    {
        /****Проверяем наличие флеша, и составляем список флеш дро*******************/
        if(checkFlush(allCards, &lFlDrawOuts))
            m_combination=Flush;
        else // если не флеш
        {
            /****Проверяем наличие стрита и определяем аутсы на стрит****/
            if(checkStraight(allCards, &lStDrawOuts))
                m_combination=Straight;
            else // если не стрит
            {
                /****Проверяем наличие трех карт одинакового номинала********/
                m_combination = checkTrips(myCard,boardCard);
                if(m_combination!=Set &&
                   m_combination!=Trips)
                {
                    /****Проверяем наличие двух пар******************************/
                    if(checkTwoPair(myCard, boardCard))
                    {
                        m_combination=TwoPair;
                    }
                    else
                    {
                        /****Проверяем наличие у нас пары****************************/
                        m_combination=checkOnePair(myCard,boardCard,&lSetOuts);
                    }
                }
            }
         }
    }
    /*****Составление списка аутсов**********************************/
    for(int i=0;i<lFlDrawOuts.size();i++)   // сначала добавляем аутсы на флеш
        lOuts<<lFlDrawOuts.at(i);
    for(int i=0;i<lStDrawOuts.size();i++)
    {
        bool bFlag=false;                   // флаг наличия совпадений
        for(int j=0;j<lOuts.size();j++)
        {
            if(lStDrawOuts.at(i).nominal == lOuts.at(j).nominal &&
               lStDrawOuts.at(i).suit == lOuts.at(j).suit) // если одинаковые
                bFlag=true;
        }
        if(!bFlag)     // если совпадения не было
            lOuts<<lStDrawOuts.at(i); // добавляем текущую карту в списке аутсов на флеш в общий список аутсов
    }
    for(int i=0;i<lSetOuts.size();i++) // добавляем ауты на сет
    {
        bool bFlag=false;
        for(int j=0;j<lOuts.size();j++)
        {
            if(lSetOuts.at(i).nominal == lOuts.at(j).nominal &&
               lSetOuts.at(i).suit == lOuts.at(j).suit)
                bFlag=true;
        }
        if(!bFlag)
            lOuts<<lSetOuts.at(i);
    }

    m_outs = lOuts;
}


//-----------------------------------------------------------------------------
// checkOnePair - проверка на наличие пары
//-----------------------------------------------------------------------------
// принимает:  QList<Card>tableCards - список карт стола
//             QList<Card>myCards - список карт игрока
// возвращает: Combination - средняя пара, топпара, оверпара (если префлоп, то
//             только оверпара)
// изменяет:   m_kickPair - кикер для пары (но не оверпара)
//
Combination GameStatus::checkOnePair(QList<Card>myCards, QList<Card>tableCards, QList<Card> *lSetOuts)
{
    Combination combination = NoCombination; // тип пары (1 - средняя, 2 - топ-пара, 3 - оверпара)
    Card        kicker;
    kicker.nominal = Card::NoNominal;// номинал кикера (при топ паре либо при составной паре)
    kicker.suit = Card::NoSuit;
    Card        myTopCard;
    Card        myPairCard;
    myPairCard.nominal = Card::NoNominal;
    myPairCard.suit = Card::NoSuit;


    lSetOuts->clear();

    if(tableCards.size() < 3)         // если список карт стола меньше трех, то проверяем только наши карты
    {
        if(myCards.at(0).nominal == myCards.at(1).nominal)// если номиналы наших карт равны
        {
            combination = OverPair;
            myPairCard = myCards.at(0);
        }
    }
    else                        // если размер списка карт стола больше либо равен 3, то не префлоп
    {
        sortCards(&myCards, myCards.size());
        sortCards(&tableCards, tableCards.size());

        if(myCards.at(0).nominal == myCards.at(1).nominal)// если наши номиналы равны
        {
            if(myCards.at(0).nominal > tableCards.at(0).nominal)  // если наш номинал старше самой старшей карты стола (список отсортирван по убыванию), то
                combination=OverPair;
            else
                combination=MiddlePair;
            myPairCard = myCards.at(0);
        }
        else                                    // если наши карты не равны
        {
            if(myCards.at(0).nominal == tableCards.at(0).nominal) // если наша старшая карта равна старшей карте стола
            {
                combination=TopPair;
                kicker = myCards.at(1);
                myPairCard = myCards.at(0);
                myTopCard = myCards.at(0); // для списка аутов на сет
            }
            else                                // если наша старшая карта не равна старшей карте борда
            {
                if(myCards.at(1).nominal == tableCards.at(0).nominal) // если наша младшая карта равна старшей карте стола
                {
                    combination=TopPair;
                    kicker = myCards.at(0);
                    myPairCard = myCards.at(1);
                    myTopCard = myCards.at(1); // для списка аутов на сет
                }
                else                                // если наша младшая карта не равна старшей карте стола
                {
                    for(int i=1;i<tableCards.size();i++)// проходим по списку карт стола
                    {
                        if(myCards.at(0).nominal == tableCards.at(i).nominal)// если номинал нашей старшей карты равен текущей карте борда
                        {
                            combination = MiddlePair;
                            kicker = myCards.at(1);
                            myPairCard = myCards.at(0);
                            break;
                        }
                        else
                            if(myCards.at(1).nominal == tableCards.at(i).nominal)// если номинал нашей младшей карты равен текущей карте борда
                            {
                            combination = MiddlePair;
                            kicker = myCards.at(0);
                            myPairCard = myCards.at(1);
                            break;
                        }
                    }
                }
            }
        }
    }

    if(combination != NoCombination)
    {
        if(myCards.at(0).nominal != myCards.at(1).nominal)
            m_kickPair = kicker;

        m_pairCard = myPairCard;
    }

    if(combination == TopPair)
    {
        QList<Card> cards;
        Card card;
        card.nominal=tableCards.at(0).nominal;
        card.suit=Card::Spades;
        cards<<card;
        card.suit=Card::Hearts;
        cards<<card;
        card.suit=Card::Diamonds;
        cards<<card;
        card.suit=Card::Clubs;
        cards<<card;
        for(int i=0; i<cards.size(); i++)
        {
            if(cards.at(i).suit != tableCards.at(0).suit &&
               cards.at(i).suit != myTopCard.suit)
            {
                lSetOuts->append(cards.at(i));
            }
        }
    }

    return combination;
}

//-----------------------------------------------------------------------------
// checkTwoPair - проверка на наличие двух пар
//-----------------------------------------------------------------------------
bool GameStatus::checkTwoPair(QList<Card> myCards, QList<Card> boardCards)
{
    bool bTwoPair=false;           // флаг наличия двух пар
    QList<Card> allCards;
    Card hightPairCard;            // номинал старшей пары
    Card lowPairCard;              // номинал младшей пары
    allCards.append(myCards);
    allCards.append(boardCards);
    sortCards(&allCards, allCards.size());
    sortCards(&myCards, myCards.size());
    sortCards(&boardCards, boardCards.size());

    hightPairCard.nominal = Card::NoNominal;
    lowPairCard.nominal = Card::NoNominal;

    for(int i=0;i<allCards.size()-1;i++)       // проходим по всем картам списка
    {
        if(allCards.at(i).nominal == allCards.at(i+1).nominal)  // если следующая карта равна текущей
        {
            if(hightPairCard.nominal == Card::NoNominal)         // и старшей пары еще не было, то
                hightPairCard.nominal = allCards.at(i).nominal; // приравниваем старшую пару текущей карте
            else
            {
                lowPairCard.nominal = allCards.at(i).nominal;   // приравниваем младшую пару текущей карте
                break;
            }
            i++;                                // переходим к следующей карте
        }
    }

    if(hightPairCard.nominal == myCards.at(0).nominal &&
       lowPairCard.nominal ==  myCards.at(1).nominal ) // если собралось 2 старших пары и участвуют обе наши карты
    {
        bTwoPair=true;                          // поднимаем флаг наличия комбинации
    }

    return bTwoPair;
}

//-----------------------------------------------------------------------------
// checkTrips - проверка на наличие 3х карт одного номинала (сет или трипс)
//-----------------------------------------------------------------------------
Combination GameStatus::checkTrips(QList<Card>myCards, QList<Card>tableCards)
{
    Combination combination = NoCombination;
    QList<Card> allCards;         // список всех известных карт
    int count = 1;                // счетная переменная для определения количества одинаковых карт
    Card tripsCard;
    tripsCard.nominal = Card::NoNominal;
    tripsCard.suit = Card::NoSuit;

    allCards.append(myCards);     // добавляем список с нашими картами
    allCards.append(tableCards);  // добавляем список с картами на столе
    sortCards(&allCards, allCards.size());

    for(int i=0;i<allCards.size()-1;i++)
    {
        if(allCards.at(i).nominal == allCards.at(i+1).nominal)
        {
            count++;
            tripsCard = allCards.at(i);
        }
        else
        {
            count = 1;
            tripsCard.nominal = Card::NoNominal;
            tripsCard.suit = Card::NoSuit;
        }
        if(count == 3)
            break;
    }
    if(count == 3)
    {
        if(myCards.at(0).nominal == myCards.at(1).nominal)
            combination = Set;
        else
        {
            if(myCards.at(0).nominal == tripsCard.nominal ||
               myCards.at(1).nominal == tripsCard.nominal)
                combination = Trips;
        }
    }

    return combination;
}

//-----------------------------------------------------------------------------
// checkStraight - проверка на наличие стрита и стритдро
//-----------------------------------------------------------------------------
// принимает:  QList<Card>cards - список всех известных карт
//             QList<Card> *straightDrawOuts - список аутсов на стрит
// возвращает: флаг наличия стрита
bool GameStatus::checkStraight(QList<Card>cards, QList<Card> *straightDrawOuts)
{
    bool bStraight = false;                                 // флаг наличия стрита
    Card highCard;
    int count=1;                                            // переменная подсчета количества подряд идущих карт

    sortCards(&cards, cards.size());
    highCard.nominal = Card::NoNominal;
    highCard.suit = Card::NoSuit;

    /***Удаление карт одинакового номинала из списка***/
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

    /*****Проверка на стрит ************************/
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

    if(bStraight && highCard.nominal == Card::Ace) // если собрался
        return bStraight;

    /*****Проверка на стрит дро*********************/
    QList<int> highCardNominals;   //список номиналов старших карт стрит дро
    QList<int> missedCardNominals; //список номиналов пропущенных карт
    straightDrawOuts->clear();    //очищаем список аутсов на стрит
    bool flag=false;        //флаг для одного разрыва на две карты между последовательными картами
    bool bStraightDraw=false;//флаг начала анализа на стрит дро (если true то уже были отличия карт на 1 или 2)
    int iStraightDraw=0;    //количество стрит дро
    int numb=0;             //номер карты, отличающейся от предыдущей больше чем на единицу
    count=1;                //начинаем считать с единицы
    for(int i=0;i<2;i++)    //два раза проходим по циклу на случай если t98_654_2 (двойной пропуск)
    {
        if(cards.size()-numb>2)//начинаем проверку на стрит дро только если осталось для проверки больше чем 3 карты
        {
            for(int j=numb;j<cards.size()-1;j++)//цикл начиная от карты после разрыва (в начальный момент считается от нуля)
            {
                if(cards[j].nominal==cards[j+1].nominal+1)  //если номиал следующей карты отличается от текущей на единицу
                {
                    if(!bStraightDraw)                      //если флаг предполагаемого стрит дро еще не поднят
                    {
                         bStraightDraw=true;             //поднимаем флаг предполагаемого стрит дро
                         highCardNominals<<cards[j].nominal;    //прибавление старшей карты стрита
                    }
                    count++;                            // увеличиваем количество подряд идущих карт на единицу
                }
                else        // если следующая карта отличается от текущей больше, чем на единицу
                    if(cards[j].nominal==cards[j+1].nominal+2)//если следующая карта отличается от текущей на 2
                    {   if(!flag)                           // если флаг разрыва через 2 опущен
                        {   count++;                        // увеличиваем количество подряд идущих на единицу
                            numb=j+1;                       //приравниваем номер карты отличающейся от предыдущей на 2
                            missedCardNominals<<cards[j].nominal-1;//добавляем пропущенную карту
                            if(!bStraightDraw)              //если проверка на стрит дро еще не запускалась
                            {   bStraightDraw=true;         //поднимаем флаг предполагаемого стрит дро
                                highCardNominals<<cards[j].nominal;//прибавление номинала старшей карты в предполагаемом стрит дро
                            }
                            flag=true;  //поднятие флага перескока через 1 карту, если следующая карта отличается от предыдущей на 2 и до этого не было отличий на 2
                        }
                        else            // если уже был один разрыв через 1 карту, то начинаем анализ стрит дро с текущей карты
                        {   count=2;    //две карты (текущая и следующая) участвуют в проверке на стрит дро
                            numb=j+1;   //приравниваем номер карты отличающейся от предыдущей на 2
                            missedCardNominals.pop_back();         //удаляем предыдущую пропущенную карту
                            missedCardNominals<<cards[j].nominal-1;//ставим на ее место карту отличающуюся от текущей на единицу
                            highCardNominals.pop_back();           //удаляем предыдущую старшую карту
                            highCardNominals<<cards[j].nominal;    //ставим на ее место текущую карту, как старшую карту предполагаемого стрит дро
                        }
                    }
                    else                // если следующая карта отличается от текущей не на 2 и не на 1, то разрывается последовательность стрита и начинаем заново считать
                    {   count=1;        // начинаем отсчет подряд идущих карт заново
                        numb=j+1;       // следущий счет начинаем с карты отличающейся от текущей больше чем на 2
                        if(bStraightDraw)//если мы находимся в режиме проверки на стрит дро, т.е. уже было отличие на 1 или 2 карты
                        {
                            highCardNominals.pop_back();           // убираем последнюю старшую карту стрита
                            if(missedCardNominals.size())missedCardNominals.pop_back();//если размер списка пропущенных карт не нулевой (т.е. там присутствуют пропущенные карты), то удаляем поледнюю
                        }
                        bStraightDraw=false;                // опускаем флаг стрит дро
                        flag=false;                         // опускаем флаг перескока через одну карту
                    }
                    //**Проверка частного случая дро с тузом (14,2,3,4,5)
                    if(count==3 && cards[0].nominal == Card::Ace)      // проверка на частный случай стрит дро (если он начинается с туза и между 5 и тузом пропущена 1 карта)
                    {
                        switch(cards[j+1].nominal+cards[j].nominal+cards[j-1].nominal){// сумма карт от 5 до 2ух должна быть меньше 13, иначе старшая карта не 5, а 6
                        case 12: { // 14,_,3,4,5
                                count++;                    // увеличиваем количество карт в стрит дро на единицу
                                if(!flag)                   // если флаг перескока через одну карту опущен (разрывов не было в трех картах подряд)
                                {
                                    missedCardNominals<<Card::Two;         // то нехватает двойки (добавляем ее в список недостающих карт)
                                    highCardNominals.pop_back();   // заменяем старшую карту с 4 на 5
                                    highCardNominals<<Card::Five;
                                    flag=true;              // поднимаем флаг пропуска, чтобы в проверке на 4 подряд уже не участвовать
                                }
                                    break;
                            }
                        case 11: { // 14,2,_,4,5
                                count++;                    // увеличиваем количество карт в стрит дро на единицу
                                if(!flag)                   // если флаг перескока через одну карту опущен (разрывов не было в трех картах подряд)
                                {
                                    missedCardNominals<<Card::Three;         // то нехватает тройки (добавляем ее в список недостающих карт)
                                    highCardNominals.pop_back();   // заменяем старшую карту с 4 на 5
                                    highCardNominals<<Card::Five;
                                    flag=true;              // поднимаем флаг пропуска, чтобы в проверке на 4 подряд уже не участвовать
                                }
                                    break;
                            }
                        case 10: { // 14,2,3,_,5
                                count++;                    // увеличиваем количество карт в стрит дро на единицу
                                if(!flag)                   // если флаг перескока через одну карту опущен (разрывов не было в трех картах подряд)
                                {
                                    missedCardNominals<<Card::Four;         // то нехватает четверки (добавляем ее в список недостающих карт)
                                    highCardNominals.pop_back();   // заменяем старшую карту с 4 на 5
                                    highCardNominals<<Card::Five;
                                    flag=true;              // поднимаем флаг пропуска, чтобы в проверке на 4 подряд уже не участвовать
                                }
                                break;
                            }
                        case 9: { // 14,2,3,4,_
                                count++;                    // увеличиваем количество карт в стрит дро на единицу
                                if(!flag)                   // если флаг перескока через одну карту опущен (разрывов не было в трех картах подряд)
                                {
                                    missedCardNominals<<Card::Five;         // то нехватает пятерки (добавляем ее в список недостающих карт)
                                    highCardNominals.pop_back();   // заменяем старшую карту с 4 на 5
                                    highCardNominals<<Card::Five;
                                    flag=true;              // поднимаем флаг пропуска, чтобы в проверке на 4 подряд уже не участвовать
                                }
                                break;
                            }

                        default:   break;
                        }
                    }
                    if(count==4)                            // если досчитали до 4 карт подряд, либо с одним пропуском то
                    {   count=1;                            // приравниваем 1 переменную счета
                        if(!flag)                           // если пропусков не было, то делаем два стрит дро
                        {   if(cards[j+1].nominal==Card::Jack)      // если старший стрит дро (заканчивается вальтом)
                            {
                                if(bStraight && highCard.nominal == Card::Ace)// если собрался стрит и старшая карта туз
                                {
                                    iStraightDraw--;        // уменьшаем количество дро
                                    highCardNominals.pop_back();   // убираем последнюю старшую карту из общего списка
                                }
                                else
                                    missedCardNominals<<cards[j+1].nominal-1;// иначе прибавляем к списку номинал нехватающих карт
                            }
                            else
                            {   missedCardNominals<<highCardNominals.last()+1;    //добавляем к списку пропущенных карт карту отличающуюся от последней карты списка старших карт на 1
                                highCardNominals.pop_back();               //стираем последнюю карту списка старших карт
                                highCardNominals<<missedCardNominals.last();      //добавляем к списку старших карт последнюю пропущенную карту
                                if((highCardNominals.last()-1)!=highCard.nominal) // если старшая карта не совпадает со старшей картой собранного стрита
                                {   iStraightDraw++;                //увеличиваем количество стрит дро на единицу (из одного прохода по картам получится 2 стрит дро)
                                    missedCardNominals<<highCardNominals.last()-5;//добавляем к списку пропущенных карт нижнюю карту двустороннего стрит дро
                                    if(missedCardNominals.last()==1)missedCardNominals.last()=Card::Ace;//частный случай если двусторонний стрит дро 5432, тогда нижняя карта туз (14)
                                    highCardNominals<<highCardNominals.last()-1;  //добавляем к списку старшую карту для нижней части двустороннего стрит дро
                                }
                            }
                            numb=j+2;   // для того чтобы разница в условии cards.size()-numb>3 и проверка на следующий стрит дро не пошла
                        }
                        flag=false;     // опускаем флаг разрыва через 2 карты
                        break;          // выходим из цикла для второй проверки на стрит дро
                    }
                    if((j==cards.size()-2)&&(count<4))  // если дошли до последнего члена и не собрали 4 карты для стрит дро то
                    {
                        if(bStraightDraw)               //если вошли в проверку на стрит дро (были карты подряд или отличающищиеся на 1)
                        {   highCardNominals.pop_back();       //то убираем последнюю занесенную старшую карту и
                            if(flag)                    //если были пропуски то
                                missedCardNominals.pop_back(); //и последнюю занесенную пропущенную карту
                        }
                    bStraightDraw=false;                //сбрасываем флаг стрит дро
                }
            }
            if(bStraightDraw)                           // если флаг стрит дро поднят
            {   iStraightDraw++;                        // прибавляем 1 к переменной подсчета количества стрит дро
                bStraightDraw=false;                    // опускаем флаг стрит дро
            }
            else break;
        }
    }

    //**удаление повторных стрит дро (выбираем старший стрит дро)
    for(int i=0;i<missedCardNominals.size()-1;i++) // проходим по всем пропущенным картам
    {
        if(missedCardNominals.at(i) == missedCardNominals.at(i+1)) // если один и тот же номинал участвует в нескольких дро
        {
            iStraightDraw--;                // уменьшаем на единицу количество дро
            missedCardNominals.removeAt(i+1);      // удаляем последний номинал
            highCardNominals.removeAt(i+1);        // удаляем последнюю старшую карту
        }
    }
    //**Составление списка аутсов (учитывая масти)
    Card oneCard;                           // одна карта (для заполнения списка аутсов)
    for(int i=0;i<missedCardNominals.size();i++)
    {
        oneCard.nominal=(Card::Nominals)missedCardNominals[i];  // присваиваем текущий номинал
        oneCard.suit=Card::Hearts;
        straightDrawOuts->append(oneCard);
        oneCard.suit=Card::Clubs;
        straightDrawOuts->append(oneCard);
        oneCard.suit=Card::Spades;
        straightDrawOuts->append(oneCard);
        oneCard.suit=Card::Diamonds;
        straightDrawOuts->append(oneCard);
    }

    return bStraight;   // возвращаем флаг наличия стрита
}

//-----------------------------------------------------------------------------
// Определение наличия флэша и составление списка флеш дро
//-----------------------------------------------------------------------------
bool GameStatus::checkFlush(QList<Card>cards, QList<Card> *flushDrawOuts)//проверка на флеш, возвращает старшую карту флеша либо 0, если не собрался (cards - все известные карты, отсортированные по убыванию)
{
    bool bFlush=false;                  //логическая переменная наличия флеша
    bool   bFlushDraw=false;            //логическая переменная флеш-дро
    Card flushHighCard;                 //
    flushDrawOuts->clear();             //очищаем список карт-аутсов на флеш
    int iH=0,iC=0,iS=0,iD=0;            //переменные подсчета количества карт соответствующих мастей

    for(int i=0; i<cards.size(); i++)   //производим подсчет карт каждой масти, если их 5, то выставляем bFlush, если 4 то bFlushDraw.
    {
        if (cards.at(i).suit==Card::Hearts) iH++;
        if (cards.at(i).suit==Card::Clubs) iC++;
        if (cards.at(i).suit==Card::Spades) iS++;
        if (cards.at(i).suit==Card::Diamonds) iD++;
    }

    if(iH>3)      { if(iH>4)bFlush=true; else bFlushDraw=true; flushHighCard.suit=Card::Hearts;} // если присутствуют больше 3ех червей, то если больше 4ех - флеш, если 4ре - то флеш дро
    else if(iC>3) { if(iC>4)bFlush=true; else bFlushDraw=true; flushHighCard.suit=Card::Clubs;} // если присутствуют больше 3ех крестей, то если больше 4ех - флеш, если 4ре - то флеш дро
    else if(iS>3) { if(iS>4)bFlush=true; else bFlushDraw=true; flushHighCard.suit=Card::Spades;} // если присутствуют больше 3ех пик, то если больше 4ех - флеш, если 4ре - то флеш дро
    else if(iD>3) { if(iD>4)bFlush=true; else bFlushDraw=true; flushHighCard.suit=Card::Diamonds;} // если присутствуют больше 3ех бубей, то если больше 4ех - флеш, если 4ре - то флеш дро

    if(bFlush)                          //если комбинация флеш собралась, то определяем старшую карту
    {
        int i=0;                        //обнуление переменной для выявления старшей карты флеша
        do  {
                flushHighCard.nominal=cards.at(i).nominal; //приравниваем переменной старшей карты флеша значение номинала текущей карты в списке выпавших карт
                i++;
        }  while(cards.at(i).suit!=flushHighCard.suit);     //пока не дошли до карты с мастью флеша
    }

    if(bFlushDraw||bFlush)              //если флеш-дро (4 карты одной масти)
    {
        Card oneCard;                   //временная переменная структуры карты
        oneCard.suit=flushHighCard.suit;        //присваиваем масть флеш дро
        for(int i=14;i>1;i--)           //составляем список всех карт масти флеш-дро
        {
            bool bOverlap=false;        //переменная совпадения карты флеш-дро и аутсами
            for(int j=0;j<cards.size();j++) //проходим по всем известным картам
                if((cards.at(j).nominal==i) &&
                   (cards.at(j).suit==flushHighCard.suit)) //если данный номинал данной масти совпал
                {   bOverlap=true;
                    break;              //выходим из цикла проверки на совпадения
                }
            if(!bOverlap)               //если не было совпадений
            {
                oneCard.nominal=(Card::Nominals)i;      //присваиваем номинал
                flushDrawOuts->append(oneCard);    //прибавляем данную карту к списку аутсов на флеш
            }
        }

        if(bFlush)                      // если определяем аутсы когда флеш собрался, то
        {
            for(int i=flushDrawOuts->size();i>0;i--)    // счетная переменная размера списка аутсов на флеш
            {
                if(flushDrawOuts->last().nominal<flushHighCard.nominal)
                flushDrawOuts->pop_back();  // удаляем все карты ниже номинала старшей карты флеша
            }                           // пока не дошли до номинала старшей карты флеша
        }
    }

    return bFlush;                      //возвращаем переменную наличия флэша
}

//------------------------------------------------------------------------------
// Определение фул хауса
//------------------------------------------------------------------------------
bool GameStatus::checkFullHouse(QList<Card>cards)
{
    bool bFullHouse=false;          // флаг наличия комбинации
    Card card3oak;                  // номинал триплета
    Card cardPair;                  // номинал пары в фулхаусе
    card3oak.nominal=Card::NoNominal;
    cardPair.nominal=Card::NoNominal;
    int count=1;                    // счетная переменная

    for(int i=0;i<cards.size()-1;i++)// проходим по списку для определения триплета
    {
        if(cards.at(i).nominal==cards.at(i+1).nominal)// если карты равны
        {
            count++;                    // подсчитываем
            card3oak.nominal=cards.at(i).nominal;  // выставляем номинал триплета
        }
        else                        // если карты не равны
        {
            count=1;                // сбрасываем счет
            card3oak.nominal=Card::NoNominal;                // обнуляем номинал триплета
        }
        if(count==3)                // если дошли до 3ех
            break;                  // выходим из счета
    }
    if(count==3)
    {
        count=1;                    // сбрасываем счетную переменную
        for(int i=0;i<cards.size()-1;i++) // производим второй круг подсчета для обнаружения пары
        {
            if(cards.at(i).nominal==cards.at(i+1).nominal &&
               cards.at(i).nominal!=card3oak.nominal)// если карты равны, и отличаются от номинала тройки
            {
                cardPair.nominal=cards.at(i).nominal; // определяем номинал пары
                break;              // выходим из счета
            }
        }
    }
    if(card3oak.nominal!=Card::NoNominal &&
       cardPair.nominal!=Card::NoNominal)          // если обнаружены тройка и пара
    {
        bFullHouse=true;            // выставляем комбинацию фулхаус
//        addLog(QString("fullhouse x3: %1 x2: %2").arg(card3oak.nominal).arg(cardPair.nominal));
    }

    return bFullHouse;
}


//-----------------------------------------------------------------------------
// Определение карэ (4 карты)
//------------------------------------------------------------------------------
bool GameStatus::checkQuads(QList<Card>cards)
{
    bool bQuads=false;              // флаг наличия каре
    int  count=1;                   // счетная переменная для определения количества одинаковых карт

    for(int i=0;i<cards.size()-1;i++)
    {
        if(cards.at(i).nominal==cards.at(i+1).nominal)// если следующая карта равна текущей
        {
            count++;                // увеличиваем количество одинаковых карт
        }
        else                        // в противном случае сбрасываем счетную переменную
        {
            count=1;
        }
        if(count==4)                // если нашли 4 одинаковые карты
            break;                  // выходим из цикла
    }
    if(count==4)                    // если после прохода по списку не собралось 4 одинаковых карты
        bQuads=true;                // то выставляем флаг карэ

    return bQuads;
}

//-----------------------------------------------------------------------------
// Определение стрит флеш
//------------------------------------------------------------------------------
bool GameStatus::checkStraightFlush(QList<Card>cards)
{
    bool bStraightFlush=false;          // флаг наличия стрит флеш
    Card royalFlushHighCard;                      // масть стрит флеша
    int count=1;                        // счетная переменная
    int iH=0,iC=0, iS=0, iD=0;          // для подсчета количества
    royalFlushHighCard.suit = Card::NoSuit;
    royalFlushHighCard.nominal = Card::NoNominal;

    for(int i=0;i<cards.size();i++)
    {
        if(cards.at(i).suit==Card::Hearts)iH++;  //
        if(cards.at(i).suit==Card::Clubs)iC++;  //
        if(cards.at(i).suit==Card::Spades)iS++;  //
        if(cards.at(i).suit==Card::Diamonds)iD++;
    }
    if(iH>4)royalFlushHighCard.suit=Card::Hearts;
    if(iC>4)royalFlushHighCard.suit=Card::Clubs;
    if(iS>4)royalFlushHighCard.suit=Card::Spades;
    if(iD>4)royalFlushHighCard.suit=Card::Diamonds;

    if(royalFlushHighCard.suit!=Card::NoSuit)                       // если флеш собрался
    {
        for(int i=0;i<cards.size();i++) // проходим по списку карт
        {
            if(cards.at(i).suit!=royalFlushHighCard.suit) // если рубашка текущей карты отличается от масти флеша
            {
                cards.removeAt(i);      // удаляем данную карту из списка
                i--;
            }
        }
        if(cards.size()>4)              // если осталось больше четырех карт в списке, производим проверку на стрит
        {
            for(int i=0;i<cards.size()-1;i++) // проходим по списку
            {
                if(cards.at(i).nominal==cards.at(i+1).nominal+1) // если номинал следующей карты отличается на единицу от текущей
                {
                    count++;
                    if(count==2)        // если первый раз карты подряд
                    {
                        royalFlushHighCard.nominal=cards.at(i).nominal; // присваиваем номинал старшей карты
                    }
                }
                else                    // если карты отличаются не на единицу
                {
                    count=1;            // начинаем отсчет заново
                    royalFlushHighCard.nominal=Card::NoNominal;        // сбрасываем старшую карту
                }
                if(count==5)            // если собралось 5 карт подряд
                    break;              // выходим из цикла проверки
            }
        }
        if(royalFlushHighCard.nominal != Card::NoNominal)                   // если определили старшую карту стрита
            bStraightFlush=true;        // выставляем наличие комбинации
        else                            // если стрит флеш не собрался
        {
            royalFlushHighCard.nominal = Card::NoNominal;              // очищаем масть флеша
            royalFlushHighCard.suit = Card::NoSuit;
        }
    }

    return bStraightFlush;
}

//-----------------------------------------------------------------------------
// Определение флеш рояль
//------------------------------------------------------------------------------
bool GameStatus::checkRoyalFlush(QList<Card>cards)
{
    bool bRoyalFlush=false;           // флаг наличия флеш рояля
    Card royalFlushHighCard;           // масть флеш рояля  и номинал старшей карты
    int count=1;                        // счетная переменная
    int iH=0,iC=0, iS=0, iD=0;          // для подсчета количества
    royalFlushHighCard.suit = Card::NoSuit;
    royalFlushHighCard.nominal = Card::NoNominal;

    for(int i=0;i<cards.size();i++)     // проходим по списку карт
    {
        if(cards.at(i).suit==Card::Hearts)iH++;  // считаем количество червей
        if(cards.at(i).suit==Card::Clubs)iC++;  // крестей
        if(cards.at(i).suit==Card::Spades)iS++;  // пик
        if(cards.at(i).suit==Card::Diamonds)iD++;  // бубей
    }
    if(iH>4)royalFlushHighCard.suit=Card::Hearts;                  // выставляем черви
    if(iC>4)royalFlushHighCard.suit=Card::Clubs;                  // выставляем крести
    if(iS>4)royalFlushHighCard.suit=Card::Spades;                  // выставляем пики
    if(iD>4)royalFlushHighCard.suit=Card::Diamonds;                  // выставляем бубны

    if(royalFlushHighCard.suit!=Card::NoSuit)                       // если флеш собрался
    {
        for(int i=0;i<cards.size();i++) // проходим по списку карт
        {
            if(cards.at(i).suit!=royalFlushHighCard.suit) // если рубашка текущей карты отличается от масти флеша
            {
                cards.removeAt(i);      // удаляем данную карту из списка
                i--;
            }
        }
        if(cards.size()>4)              // если осталось больше четырех карт в списке, производим проверку на стрит
        {
            for(int i=0;i<cards.size()-1;i++) // проходим по списку
            {
                if(cards.at(i).nominal==cards.at(i+1).nominal+1) // если номинал следующей карты отличается на единицу от текущей
                {
                    count++;
                    if(count==2)        // если первый раз карты подряд
                    {
                        royalFlushHighCard.nominal=cards.at(i).nominal; // присваиваем номинал старшей карты
                    }
                }
                else                    // если карты отличаются не на единицу
                {
                    count=1;            // начинаем отсчет заново
                    royalFlushHighCard.nominal=Card::NoNominal;        // сбрасываем старшую карту
                }
                if(count==5)            // если собралось 5 карт подряд
                    break;              // выходим из цикла проверки
            }
        }
        if(royalFlushHighCard.nominal == Card::Ace)               // если определили старшую карту стрита туз
            bRoyalFlush=true;           // выставляем наличие комбинации флеш рояль
        else                            // если флеш рояль не собрался
        {   royalFlushHighCard.suit = Card::NoSuit;     // очищаем масть флеша
            royalFlushHighCard.nominal = Card::NoNominal; // обнуляем старшую карту
        }
    }

    return bRoyalFlush;
}


