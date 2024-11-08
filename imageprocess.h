#ifndef IMAGEPROCESS_H
#define IMAGEPROCESS_H

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <tesseract/tesseract/baseapi.h>
#include <tesseract/leptonica/allheaders.h>
#include <gamedefs.h>
#include <settingsparser.h>
#include <QImage>

#define VALUE_BINS              100
#define SATUR_BINS              100
#define VALUE_RANGE_MIN         0
#define VALUE_RANGE_MAX         255
#define SATUR_RANGE_MIN         0
#define SATUR_RANGE_MAX         255
#define HIST_CHANNEL_1          0//1
#define HIST_CHANNEL_2          2
#define PLAYER_MATCH_SENS       0.58
#define ACT_PLAYER_MATCH_SENS   0.55//0.88
#define BUTTON_MATCH_SENS       0.095
#define BUTTON2_MATCH_SENS      0.85
#define MAX_SEARCH_COUNT        15

#define CHEAT_DOLLAR_DEL
#define SPACES_MIN              20
#define SPACES_MAX              50
#define POPUP_BRIGHTNESS_MAX    145

using namespace Game;

class ImageProcess: public QObject
{
    Q_OBJECT
public:
    ImageProcess();
    ~ImageProcess();
    void processImage(cv::Mat tableImage, Table *table);
    void processImage(QImage tableImage, Table *table);
    void setTableSize(TableSize size);
    TableSize tableSize();

public slots:
    void tableSizeChanged(QString tableSize);

private:
    cv::Mat imageToMat(QImage &image);
    cv::Rect toCvtRect(QRect rect, bool scale = true);
    cv::Mat prepareImage(cv::Mat sourceImage, cv::Rect rect);
    QString cleanText(QString text, bool isNumber = false);

    QString recognizeText(cv::Mat textImage);
    double compareHistogram(const cv::Mat baseImage, const cv::Mat testImage);
    QList<cv::Rect> matchingTemplate(const cv::Mat baseImage, const cv::Mat templImage, short counts = 15);

    bool setTableImage(cv::Mat tableImage);

    bool isValidImage();
    double getTotalPot();
    double getPlayerBet(cv::Rect playerPosition, short playerNumber);
    bool isButton(cv::Rect playerPosition, short playerNumber);
    QList<Card> getPlayerCards(cv::Rect playerPosition, short playerNumber);
    double getPlayerBank(cv::Rect playerPosition, short playerNumber);
    QString getPlayerName(cv::Rect playerPosition, short playerNumber);
    QList<PlayerData> getPlayers();
    QList<Card> getCards();
    QList<QRect> getGameButtons();
    QRect getBetInput();
    QRect getOptionsTab();
    QRect getChatTab();
    QRect getRebuyTab();
    QRect getRebuyInput();
    QRect getRebuyButton();
    QRect getSitOutNextBBcheckbox();
    QRect getSitOutCheckbox();
    QRect getPopupWindowButton();
    QRect getPopupWindowPart();

    bool m_scaleChanged;
    cv::Mat m_tableImage;
    cv::Mat m_playerTemplate;
    cv::Mat m_buttonTemplate;
    cv::Mat m_button2Template;
    cv::Mat m_activePlayerTemplate;

    tesseract::TessBaseAPI *m_tessApi;
    QPointF m_scalePoint;

    TableSize currentTableSize;
    SettingsParser settingsParser;
    TableSettings m_tableSettings;
};

#endif // IMAGEPROCESS_H
