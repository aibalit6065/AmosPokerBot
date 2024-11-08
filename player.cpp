#include "player.h"

Player::Player()
{

}

void Player::setPlayerData(PlayerData playerData)
{
    m_current = playerData.current;
    m_button = playerData.button;
    m_seatNumber = playerData.seatNumber;
    m_name = playerData.name;
    m_stack = playerData.stack;
    m_bet = playerData.bet;
    m_cards = playerData.cards;
}

void Player::setStats()
{
    m_stats.hands = 0;
    m_stats.pfr = 0;
    m_stats.vpip = 0;
    m_stats.ats = 0;
    m_stats.fbbtos = 0;
    m_stats.fsbtos = 0;
    m_stats.fcbetf = 0;
    m_stats.orEP = 0;
    m_stats.orMP = 0;
    m_stats.orCO = 0;
    m_stats.orBU = 0;
    m_stats.orSB = 0;

    /*QSqlQuery query;
    query.exec("SELECT sites.site_abbrev, p.player_name, \
               COUNT(hhps.id_player) AS hands, \
               AVG (CASE WHEN flg_vpip THEN 1 ELSE 0 END)*100 AS vpip, \
               AVG (CASE WHEN cnt_p_raise >= 1 THEN 1 ELSE 0 END)*100 AS pfr, \
               AVG (CASE WHEN flg_steal_att THEN 1 WHEN flg_steal_opp THEN 0 END)*100 AS ats, \
               AVG (CASE WHEN flg_sb_steal_fold THEN 1 WHEN flg_blind_def_opp and flg_blind_s THEN 0 END)*100 AS fsbtos, \
               AVG (CASE WHEN flg_bb_steal_fold THEN 1 WHEN flg_blind_def_opp and flg_blind_b THEN 0 END)*100 AS fbbtos, \
               AVG (CASE WHEN enum_f_cbet_action='F' THEN 1 WHEN flg_f_cbet_def_opp THEN 0 END)*100 AS fcbetf \
               FROM lookup_sites AS sites INNER \
               JOIN ( \
               player AS p INNER JOIN \
               cash_hand_player_statistics AS hhps ON (p.player_name = 'PopovAndrey')and(p.id_player = hhps.id_player) ) \
               ON p.id_site = sites.id_site \
               GROUP BY sites.site_abbrev,p.player_name \
               ORDER BY sites.site_abbrev DESC;");
    if(!query.isActive())
    {
        qDebug()<<"DB error"<<query.lastError().text();
    }

    while (query.next()) {
        bool ok;
        QString site_abbrev = query.value(0).toString();
        QString player_name = query.value(1).toString();
        int hands = query.value(2).toInt();
        double vpip = query.value(3).toDouble(&ok);
        double pfr = query.value(4).toDouble(&ok);
        double ats = query.value(5).toDouble(&ok);
        double fsbtos = query.value(6).toDouble(&ok);
        double fbbtos = query.value(7).toDouble(&ok);
        double fcbetf = query.value(8).toDouble(&ok);
        qDebug()<<"site_abbrev "<<site_abbrev<<"\n"\
                <<"player_name "<<player_name<<"\n"\
                <<"hands "<<hands<<"\n"\
                <<"vpip "<<vpip<<"\n"\
                <<"pfr "<<pfr<<"\n"\
                <<"ats "<<ats<<"\n"\
                <<"fsbtos "<<fsbtos<<"\n"\
                <<"fbbtos "<<fbbtos<<"\n"\
                <<"fcbetf "<<fcbetf;

    }*/
}

bool Player::isActive()
{
    return m_active;
}

void Player::setActive(bool active)
{
    m_active = active;
}

bool Player::isCurrent()
{
    return m_current;
}

void Player::setCurrent(bool current)
{
    m_current = current;
}

bool Player::isPlayer()
{
    return m_player;
}

void Player::setPlayer(bool player)
{
    m_player = player;
}

bool Player::isButton()
{
    return m_button;
}

void Player::setButton(bool button)
{
    m_button = button;
}

quint8 Player::number()
{
    return m_number;
}

void Player::setNumber(quint8 number)
{
    m_number = number;
}

quint8 Player::seatNumber()
{
    return m_seatNumber;
}

void Player::setSeatNumber(quint8 seatNumber)
{
    m_seatNumber = seatNumber;
}

QString Player::name()
{
    return m_name;
}

void Player::setName(QString name)
{
    m_name = name;
}

qreal Player::stack()
{
    return m_stack;
}

void Player::setStack(qreal stack)
{
    m_stack = stack;
}

qreal Player::bet()
{
    return m_bet;
}

void Player::setBet(qreal bet)
{
    m_bet = bet;
}

Position Player::position()
{
    return m_position;
}

void Player::setPosition(Position position)
{
    m_position = position;
}

QList<Card> Player::cards()
{
    return m_cards;
}

void Player::addCard(Card card)
{
    if (card.suit!=Card::NoSuit && card.nominal!=Card::NoNominal)
        m_cards.append(card);
}

void Player::setCard(Card card, quint8 number)
{
    if (card.suit!=Card::NoSuit && card.nominal!=Card::NoNominal) {
        if (number < m_cards.count())
            m_cards.replace(number, card);
        else
            m_cards.append(card);
    }
}

void Player::setCards(QList<Card> cards)
{
    m_cards = cards;
}

QList<Action> Player::actions(Stage stage)
{
    if(m_stageActions.size()>=stage)
        return m_stageActions[stage-1].actions;
    else
    {
        QList<Action> emptyList;
        return emptyList;
    }
}

int Player::actionStageSize()
{
    return m_stageActions.size();
}

void Player::setAction(Stage stage, Action action, qreal bet, int circle)
{
    if(stage>m_stageActions.size())
        return;
    else
    {
        if(circle>m_stageActions[stage-1].actions.size())
            return;
        else
        {
             m_stageActions[stage-1].actions[circle-1] = action;
             m_stageActions[stage-1].bets[circle-1] = bet;
        }
    }
}

void Player::addAction(Stage stage, Action action, qreal bet)
{
    if(stage>m_stageActions.size())
    {
        StageActions stageAction;
        stageAction.actions.append(action);
        stageAction.bets.append(bet);
        m_stageActions.append(stageAction);
    }
    else
    {
        m_stageActions[stage-1].actions.append(action);
        m_stageActions[stage-1].bets.append(bet);
    }
}

QList<qreal> Player::bets(Stage stage)
{
    if(m_stageActions.size()>=stage)
        return m_stageActions[stage-1].bets;
    else
    {
        QList<qreal> emptyList;
        return emptyList;
    }
}
