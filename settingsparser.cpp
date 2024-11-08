#include "settingsparser.h"
#include <QFile>
#include <QXmlStreamReader>
#include <QStringList>

SettingsParser::SettingsParser()
{
    m_tableSize = Table10Players;
}

TableSettings SettingsParser::parseConfigFile(QString fileName)
{
    return parseConfigFile(m_tableSize, fileName);
}

TableSettings SettingsParser::parseConfigFile(quint8 tableSize, QString fileName)
{
    switch(tableSize) {
    case 6:
        m_tableSize = Table6Players;
        break;
    case 9:
        m_tableSize = Table9Players;
        break;
    case 10:
    default:
        m_tableSize = Table10Players;
        break;
    }
    return parseConfigFile(m_tableSize, fileName);
}

TableSettings SettingsParser::parseConfigFile(TableSize tableSize, QString fileName)
{
    QFile config(fileName);
    config.open(QFile::ReadOnly | QFile::Text);
    QXmlStreamReader xml(&config);
    m_tableSize = tableSize;

    while (xml.readNextStartElement()) {
        if (xml.name() == "table") {
            m_settings.defaultPictureSize = QSize(xml.attributes().value("defWidth").toInt(),
                                                       xml.attributes().value("defHeight").toInt());
        }
        else if (xml.name() == "totalPot") {
            m_settings.totalPotRect = QRect(xml.attributes().value("x").toInt(),
                                                 xml.attributes().value("y").toInt(),
                                                 xml.attributes().value("width").toInt(),
                                                 xml.attributes().value("height").toInt());
            xml.skipCurrentElement();
        }
        else if (xml.name() == "templates") {
            while (xml.readNextStartElement()) {
                if (xml.name() == "playerTemplate") {
                   m_settings.playerTemplFileName = xml.attributes().value("filename").toString();
                }
                else if (xml.name() == "buttonTemplate") {
                    m_settings.buttonTemplFileName = xml.attributes().value("filename").toString();
                }
                else if (xml.name() == "button2Template") {
                    m_settings.button2TemplFileName = xml.attributes().value("filename").toString();
                }
                else if (xml.name() == "activePlayerTemplate") {
                    m_settings.activePlayerTemplFileName = xml.attributes().value("filename").toString();
                }
                xml.skipCurrentElement();
            }
        }
        else if (xml.name() == "tableSizes") {
            while (xml.readNextStartElement()) {
                if (xml.name() == "table6") {
                    m_tableSizeCards.insert(Table6Players, xml.readElementText());
                }
                else if (xml.name() == "table9") {
                    m_tableSizeCards.insert(Table9Players, xml.readElementText());
                }
                else if (xml.name() == "table10") {
                    m_tableSizeCards.insert(Table10Players, xml.readElementText());
                }
            }
        }
        else if (xml.name() == "players") {
            int i = 0;
            m_settings.playersSettings.clear();
            while (xml.readNextStartElement()) {
                QString playerNumber = "player" + QString::number(i);
                if (xml.name() == playerNumber) {
                    PlayerSettings player;
                    player.playerRect = QRect(xml.attributes().value("x").toInt(),
                                              xml.attributes().value("y").toInt(),
                                              xml.attributes().value("width").toInt(),
                                              xml.attributes().value("height").toInt());
                    while (xml.readNextStartElement()) {
                        if (xml.name() == "name") {
                            player.nameRect = QRect(xml.attributes().value("x").toInt(),
                                                    xml.attributes().value("y").toInt(),
                                                    xml.attributes().value("width").toInt(),
                                                    xml.attributes().value("height").toInt());
                            xml.skipCurrentElement();
                        }
                        else if (xml.name() == "bank") {
                            player.bankRect = QRect(xml.attributes().value("x").toInt(),
                                                    xml.attributes().value("y").toInt(),
                                                    xml.attributes().value("width").toInt(),
                                                    xml.attributes().value("height").toInt());
                            xml.skipCurrentElement();
                        }
                        else if (xml.name() == "button") {
                            player.buttonRect = QRect(xml.attributes().value("x").toInt(),
                                                      xml.attributes().value("y").toInt(),
                                                      xml.attributes().value("width").toInt(),
                                                      xml.attributes().value("height").toInt());
                            xml.skipCurrentElement();
                        }
                        else if (xml.name() == "bet") {
                            player.betRect = QRect(xml.attributes().value("x").toInt(),
                                                   xml.attributes().value("y").toInt(),
                                                   xml.attributes().value("width").toInt(),
                                                   xml.attributes().value("height").toInt());
                            xml.skipCurrentElement();
                        }
                        else if (xml.name() == "cards") {
                            int j = 0;
                            while (xml.readNextStartElement()) {
                                QString cardNumber = "card" + QString::number(j);
                                if (xml.name() == cardNumber) {
                                    QRect card(xml.attributes().value("x").toInt(),
                                               xml.attributes().value("y").toInt(),
                                               xml.attributes().value("width").toInt(),
                                               xml.attributes().value("height").toInt());
                                    player.cardsRects.append(card);
                                    ++j;
                                }
                                xml.skipCurrentElement();
                            }
                        }
                        else
                            xml.skipCurrentElement();
                    }
                    m_settings.playersSettings.append(player);
                    ++i;
                }
            }
        }
        else if (xml.name() == "cards") {
            int i = 0;
            m_settings.cardsRects.clear();
            while (xml.readNextStartElement()) {
                QString cardNumber = "card" + QString::number(i);
                if (xml.name() == cardNumber) {
                    QRect card(xml.attributes().value("x").toInt(),
                               xml.attributes().value("y").toInt(),
                               xml.attributes().value("width").toInt(),
                               xml.attributes().value("height").toInt());
                    m_settings.cardsRects.append(card);
                    ++i;
                }
                xml.skipCurrentElement();
            }
        }
        else if (xml.name() == "buttons") {
            int i = 0;
            m_settings.buttonsRects.clear();
            while (xml.readNextStartElement()) {
                QString buttonNumber = "button" + QString::number(i);
                if (xml.name() == buttonNumber) {
                    QRect button(xml.attributes().value("x").toInt(),
                                 xml.attributes().value("y").toInt(),
                                 xml.attributes().value("width").toInt(),
                                 xml.attributes().value("height").toInt());
                    m_settings.buttonsRects.append(button);
                    ++i;
                }
                xml.skipCurrentElement();
            }
        }
        else if (xml.name() == "betInput") {
            m_settings.betInputRect = QRect(xml.attributes().value("x").toInt(),
                                            xml.attributes().value("y").toInt(),
                                            xml.attributes().value("width").toInt(),
                                            xml.attributes().value("height").toInt());
            xml.skipCurrentElement();
        }
        else if (xml.name() == "optionsTab") {
            m_settings.optionsTabRect = QRect(xml.attributes().value("x").toInt(),
                                            xml.attributes().value("y").toInt(),
                                            xml.attributes().value("width").toInt(),
                                            xml.attributes().value("height").toInt());
            xml.skipCurrentElement();
        }
        else if (xml.name() == "chatTab") {
            m_settings.chatTabRect = QRect(xml.attributes().value("x").toInt(),
                                            xml.attributes().value("y").toInt(),
                                            xml.attributes().value("width").toInt(),
                                            xml.attributes().value("height").toInt());
            xml.skipCurrentElement();
        }
        else if (xml.name() == "rebuyTab") {
            m_settings.rebuyTabRect = QRect(xml.attributes().value("x").toInt(),
                                            xml.attributes().value("y").toInt(),
                                            xml.attributes().value("width").toInt(),
                                            xml.attributes().value("height").toInt());
            xml.skipCurrentElement();
        }
        else if (xml.name() == "rebuyInput") {
            m_settings.rebuyInputRect = QRect(xml.attributes().value("x").toInt(),
                                            xml.attributes().value("y").toInt(),
                                            xml.attributes().value("width").toInt(),
                                            xml.attributes().value("height").toInt());
            xml.skipCurrentElement();
        }
        else if (xml.name() == "rebuyButton") {
            m_settings.rebuyButtonRect = QRect(xml.attributes().value("x").toInt(),
                                            xml.attributes().value("y").toInt(),
                                            xml.attributes().value("width").toInt(),
                                            xml.attributes().value("height").toInt());
            xml.skipCurrentElement();
        }
        else if (xml.name() == "sitOutNextBBcheckbox") {
            m_settings.sitOutNextBBcheckboxRect = QRect(xml.attributes().value("x").toInt(),
                                            xml.attributes().value("y").toInt(),
                                            xml.attributes().value("width").toInt(),
                                            xml.attributes().value("height").toInt());
            xml.skipCurrentElement();
        }
        else if (xml.name() == "sitOutCheckbox") {
            m_settings.sitOutCheckboxRect = QRect(xml.attributes().value("x").toInt(),
                                            xml.attributes().value("y").toInt(),
                                            xml.attributes().value("width").toInt(),
                                            xml.attributes().value("height").toInt());
            xml.skipCurrentElement();
        }
        else if (xml.name() == "popupWindowButton") {
            m_settings.popupWindowButtonRect = QRect(xml.attributes().value("x").toInt(),
                                                xml.attributes().value("y").toInt(),
                                                xml.attributes().value("width").toInt(),
                                                xml.attributes().value("height").toInt());
            xml.skipCurrentElement();
        }
        else if (xml.name() == "popupWindowPart") {
            m_settings.popupWindowPartRect = QRect(xml.attributes().value("x").toInt(),
                                                xml.attributes().value("y").toInt(),
                                                xml.attributes().value("width").toInt(),
                                                xml.attributes().value("height").toInt());
            xml.skipCurrentElement();
        }
        else
            xml.skipCurrentElement();
    }
    config.close();

    TableSettings settings = m_settings;
    settings.playersSettings.clear();

    QStringList sequence = m_tableSizeCards[m_tableSize].split(",");
    for (int i=0; i<sequence.count(); ++i) {
        bool result = false;
        quint8 number = sequence.at(i).toInt(&result);
        if (result)
            settings.playersSettings.append(m_settings.playersSettings.at(number));
    }

    return settings;
}

void SettingsParser::writeConfigFile(TableSettings settings, QString fileName)
{
    QFile config(fileName);
    config.open(QFile::WriteOnly | QFile::Text);
    QXmlStreamWriter xml(&config);
    xml.setAutoFormatting(true);

    QStringList sequence = m_tableSizeCards[m_tableSize].split(",");
    for (int i=0; i<sequence.count(); ++i) {
        bool result = false;
        quint8 number = sequence.at(i).toInt(&result);
        if (result && i < settings.playersSettings.count())
            m_settings.playersSettings[number] = settings.playersSettings.at(i);
    }

    xml.writeStartDocument();
    xml.writeStartElement("table");
    xml.writeAttribute("defWidth", QString::number(m_settings.defaultPictureSize.width()));
    xml.writeAttribute("defHeight", QString::number(m_settings.defaultPictureSize.height()));

    xml.writeStartElement("templates");
    xml.writeStartElement("playerTemplate");
    xml.writeAttribute("filename", m_settings.playerTemplFileName);
    xml.writeEndElement();
    xml.writeStartElement("activePlayerTemplate");
    xml.writeAttribute("filename", m_settings.activePlayerTemplFileName);
    xml.writeEndElement();
    xml.writeStartElement("buttonTemplate");
    xml.writeAttribute("filename", m_settings.buttonTemplFileName);
    xml.writeEndElement();
    xml.writeStartElement("button2Template");
    xml.writeAttribute("filename", m_settings.button2TemplFileName);
    xml.writeEndElement();
    xml.writeEndElement();

    xml.writeStartElement("totalPot");
    xml.writeAttribute("x", QString::number(settings.totalPotRect.x()));
    xml.writeAttribute("y", QString::number(settings.totalPotRect.y()));
    xml.writeAttribute("width", QString::number(settings.totalPotRect.width()));
    xml.writeAttribute("height", QString::number(settings.totalPotRect.height()));
    xml.writeEndElement();

    xml.writeStartElement("tableSizes");
    QList<TableSize> keys = m_tableSizeCards.keys();
    for (int i=0; i<keys.count(); ++i) {
        switch(keys[i]) {
        case Table6Players:
            xml.writeTextElement("table6", m_tableSizeCards[Table6Players]);
            break;
        case Table9Players:
            xml.writeTextElement("table9", m_tableSizeCards[Table9Players]);
            break;
        case Table10Players:
            xml.writeTextElement("table10", m_tableSizeCards[Table10Players]);
            break;
        }
    }
    xml.writeEndElement();

    xml.writeStartElement("players");
    QList<PlayerSettings> players = m_settings.playersSettings;
    for (int i=0; i<players.count(); ++i) {
        xml.writeStartElement("player"+QString::number(i));
        xml.writeAttribute("x", QString::number(players.at(i).playerRect.x()));
        xml.writeAttribute("y", QString::number(players.at(i).playerRect.y()));
        xml.writeAttribute("width", QString::number(players.at(i).playerRect.width()));
        xml.writeAttribute("height", QString::number(players.at(i).playerRect.height()));

        xml.writeStartElement("name");
        xml.writeAttribute("x", QString::number(players.at(i).nameRect.x()));
        xml.writeAttribute("y", QString::number(players.at(i).nameRect.y()));
        xml.writeAttribute("width", QString::number(players.at(i).nameRect.width()));
        xml.writeAttribute("height", QString::number(players.at(i).nameRect.height()));
        xml.writeEndElement();

        xml.writeStartElement("bank");
        xml.writeAttribute("x", QString::number(players.at(i).bankRect.x()));
        xml.writeAttribute("y", QString::number(players.at(i).bankRect.y()));
        xml.writeAttribute("width", QString::number(players.at(i).bankRect.width()));
        xml.writeAttribute("height", QString::number(players.at(i).bankRect.height()));
        xml.writeEndElement();

        xml.writeStartElement("button");
        xml.writeAttribute("x", QString::number(players.at(i).buttonRect.x()));
        xml.writeAttribute("y", QString::number(players.at(i).buttonRect.y()));
        xml.writeAttribute("width", QString::number(players.at(i).buttonRect.width()));
        xml.writeAttribute("height", QString::number(players.at(i).buttonRect.height()));
        xml.writeEndElement();

        xml.writeStartElement("bet");
        xml.writeAttribute("x", QString::number(players.at(i).betRect.x()));
        xml.writeAttribute("y", QString::number(players.at(i).betRect.y()));
        xml.writeAttribute("width", QString::number(players.at(i).betRect.width()));
        xml.writeAttribute("height", QString::number(players.at(i).betRect.height()));
        xml.writeEndElement();

        xml.writeStartElement("cards");
        QList<QRect> playerCards = players.at(i).cardsRects;
        for (int j=0; j<playerCards.count(); ++j) {
            xml.writeStartElement("card"+QString::number(j));
            xml.writeAttribute("x", QString::number(playerCards.at(j).x()));
            xml.writeAttribute("y", QString::number(playerCards.at(j).y()));
            xml.writeAttribute("width", QString::number(playerCards.at(j).width()));
            xml.writeAttribute("height", QString::number(playerCards.at(j).height()));
            xml.writeEndElement();
        }
        xml.writeEndElement();

        xml.writeEndElement();
    }
    xml.writeEndElement();

    xml.writeStartElement("cards");
    QList<QRect> tableCards = m_settings.cardsRects;
    for (int i=0; i<tableCards.count(); ++i) {
        xml.writeStartElement("card"+QString::number(i));
        xml.writeAttribute("x", QString::number(tableCards.at(i).x()));
        xml.writeAttribute("y", QString::number(tableCards.at(i).y()));
        xml.writeAttribute("width", QString::number(tableCards.at(i).width()));
        xml.writeAttribute("height", QString::number(tableCards.at(i).height()));
        xml.writeEndElement();
    }
    xml.writeEndElement();

    xml.writeStartElement("buttons");
    QList<QRect> tableButtons = m_settings.buttonsRects;
    for (int i=0; i<tableButtons.count(); ++i) {
        xml.writeStartElement("button"+QString::number(i));
        xml.writeAttribute("x", QString::number(tableButtons.at(i).x()));
        xml.writeAttribute("y", QString::number(tableButtons.at(i).y()));
        xml.writeAttribute("width", QString::number(tableButtons.at(i).width()));
        xml.writeAttribute("height", QString::number(tableButtons.at(i).height()));
        xml.writeEndElement();
    }
    xml.writeEndElement();

    xml.writeStartElement("betInput");
    xml.writeAttribute("x", QString::number(settings.betInputRect.x()));
    xml.writeAttribute("y", QString::number(settings.betInputRect.y()));
    xml.writeAttribute("width", QString::number(settings.betInputRect.width()));
    xml.writeAttribute("height", QString::number(settings.betInputRect.height()));
    xml.writeEndElement();

    xml.writeStartElement("optionsTab");
    xml.writeAttribute("x", QString::number(settings.optionsTabRect.x()));
    xml.writeAttribute("y", QString::number(settings.optionsTabRect.y()));
    xml.writeAttribute("width", QString::number(settings.optionsTabRect.width()));
    xml.writeAttribute("height", QString::number(settings.optionsTabRect.height()));
    xml.writeEndElement();

    xml.writeStartElement("chatTab");
    xml.writeAttribute("x", QString::number(settings.chatTabRect.x()));
    xml.writeAttribute("y", QString::number(settings.chatTabRect.y()));
    xml.writeAttribute("width", QString::number(settings.chatTabRect.width()));
    xml.writeAttribute("height", QString::number(settings.chatTabRect.height()));
    xml.writeEndElement();

    xml.writeStartElement("rebuyTab");
    xml.writeAttribute("x", QString::number(settings.rebuyTabRect.x()));
    xml.writeAttribute("y", QString::number(settings.rebuyTabRect.y()));
    xml.writeAttribute("width", QString::number(settings.rebuyTabRect.width()));
    xml.writeAttribute("height", QString::number(settings.rebuyTabRect.height()));
    xml.writeEndElement();

    xml.writeStartElement("rebuyInput");
    xml.writeAttribute("x", QString::number(settings.rebuyInputRect.x()));
    xml.writeAttribute("y", QString::number(settings.rebuyInputRect.y()));
    xml.writeAttribute("width", QString::number(settings.rebuyInputRect.width()));
    xml.writeAttribute("height", QString::number(settings.rebuyInputRect.height()));
    xml.writeEndElement();

    xml.writeStartElement("rebuyButton");
    xml.writeAttribute("x", QString::number(settings.rebuyButtonRect.x()));
    xml.writeAttribute("y", QString::number(settings.rebuyButtonRect.y()));
    xml.writeAttribute("width", QString::number(settings.rebuyButtonRect.width()));
    xml.writeAttribute("height", QString::number(settings.rebuyButtonRect.height()));
    xml.writeEndElement();

    xml.writeStartElement("sitOutNextBBcheckbox");
    xml.writeAttribute("x", QString::number(settings.sitOutNextBBcheckboxRect.x()));
    xml.writeAttribute("y", QString::number(settings.sitOutNextBBcheckboxRect.y()));
    xml.writeAttribute("width", QString::number(settings.sitOutNextBBcheckboxRect.width()));
    xml.writeAttribute("height", QString::number(settings.sitOutNextBBcheckboxRect.height()));
    xml.writeEndElement();

    xml.writeStartElement("sitOutCheckbox");
    xml.writeAttribute("x", QString::number(settings.sitOutCheckboxRect.x()));
    xml.writeAttribute("y", QString::number(settings.sitOutCheckboxRect.y()));
    xml.writeAttribute("width", QString::number(settings.sitOutCheckboxRect.width()));
    xml.writeAttribute("height", QString::number(settings.sitOutCheckboxRect.height()));
    xml.writeEndElement();

    xml.writeStartElement("popupWindowButton");
    xml.writeAttribute("x", QString::number(settings.popupWindowButtonRect.x()));
    xml.writeAttribute("y", QString::number(settings.popupWindowButtonRect.y()));
    xml.writeAttribute("width", QString::number(settings.popupWindowButtonRect.width()));
    xml.writeAttribute("height", QString::number(settings.popupWindowButtonRect.height()));
    xml.writeEndElement();

    xml.writeStartElement("popupWindowPart");
    xml.writeAttribute("x", QString::number(settings.popupWindowPartRect.x()));
    xml.writeAttribute("y", QString::number(settings.popupWindowPartRect.y()));
    xml.writeAttribute("width", QString::number(settings.popupWindowPartRect.width()));
    xml.writeAttribute("height", QString::number(settings.popupWindowPartRect.height()));
    xml.writeEndElement();
    
    xml.writeEndElement();
    xml.writeEndDocument();
    config.close();
}
