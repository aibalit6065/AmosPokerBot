#ifndef SETTINGSPARSER_H
#define SETTINGSPARSER_H

#include <QList>
#include <QMap>
#include <QRect>
#include <QSize>
#include <QString>

enum TableSize {
    Table6Players = 6,
    Table9Players = 9,
    Table10Players = 10,
};

struct PlayerSettings
{
    QRect playerRect;
    QRect nameRect;
    QRect bankRect;
    QRect buttonRect;
    QRect betRect;
    QList<QRect> cardsRects;
};

struct TableSettings
{
    QSize defaultPictureSize;
    QRect totalPotRect;
    QList<QRect> cardsRects;
    QList<PlayerSettings> playersSettings;
    QList<QRect> buttonsRects;
    QRect betInputRect;
    QRect optionsTabRect;
    QRect chatTabRect;
    QRect rebuyTabRect;
    QRect rebuyInputRect;
    QRect rebuyButtonRect;
    QRect sitOutNextBBcheckboxRect;
    QRect sitOutCheckboxRect;
    QRect popupWindowButtonRect;
    QRect popupWindowPartRect;

    QString playerTemplFileName;
    QString buttonTemplFileName;
    QString button2TemplFileName;
    QString activePlayerTemplFileName;
};

class SettingsParser
{
public:
    SettingsParser();

    TableSettings parseConfigFile(QString fileName);
    TableSettings parseConfigFile(quint8 tableSize, QString fileName);
    TableSettings parseConfigFile(TableSize tableSize,QString fileName);
    void writeConfigFile(TableSettings settings, QString fileName);

private:
    TableSize m_tableSize;
    QMap<TableSize, QString> m_tableSizeCards;
    TableSettings m_settings;
};

#endif // SETTINGSPARSER_H
