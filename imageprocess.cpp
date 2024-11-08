#include "imageprocess.h"
#include <QRegExp>
#include <QDebug>
#include <QDir>

using namespace cv;

//#define DRAW_RECTANGLE
//#define SHOW_IMAGE

#ifdef CHEAT_DOLLAR_DEL
void dollarDel(Mat image)
{
    quint16 counter = 0;
    QList<int> spaces;
    bool flag;
    for (int x=0;x<image.cols;x++) {
        flag = false;
        for (int y=0;y<image.rows;y++) {
            if (image.at<char>(y,x) != 0)
                flag = true;
        }
        if (flag) {
            if (counter != 0) {
                spaces.append(x);
                spaces.append(counter);
            }
            counter = 0;

        }
        else
            counter++;
    }

    for (int i=(spaces.count() > 2 ? 2 : 0); i<spaces.count(); i+=2) {
        if (spaces.at(i+1) >= SPACES_MIN && spaces.at(i+1) < SPACES_MAX) {
            for (int x=spaces.at(i);x<image.cols;x++)
                for (int y=0;y<image.rows;y++)
                    image.at<char>(y,x) = 0;
        }
    }
}
#endif

ImageProcess::ImageProcess()
{
    qputenv("TESSDATA_PREFIX", QDir::currentPath().toUtf8());
    m_tessApi = new tesseract::TessBaseAPI();
    m_tessApi->Init(NULL, "eng");

    setTableSize(Table10Players);
    QImage image(":/"+m_tableSettings.playerTemplFileName);
    m_playerTemplate = imageToMat(image);
    image.load(":/"+m_tableSettings.buttonTemplFileName);
    m_buttonTemplate = imageToMat(image);
    image.load(":/"+m_tableSettings.button2TemplFileName);
    m_button2Template = imageToMat(image);
    image.load(":/"+m_tableSettings.activePlayerTemplFileName);
    m_activePlayerTemplate = imageToMat(image);
}

ImageProcess::~ImageProcess()
{
    delete m_tessApi;
}

void ImageProcess::setTableSize(TableSize size)
{
    currentTableSize = size;
    m_tableSettings = settingsParser.parseConfigFile(currentTableSize, ":/config.xml");
}

TableSize ImageProcess::tableSize()
{
    return currentTableSize;
}

Mat ImageProcess::imageToMat(QImage &image)
{
    return Mat(image.height(),image.width(),CV_8UC4,
               (uchar*)image.bits(),image.bytesPerLine()).clone();
}

Rect ImageProcess::toCvtRect(QRect rect, bool scale)
{
    Rect cvtRect;

    if (scale) {
        Size size = Size(rect.width() * m_scalePoint.x(),
                         rect.height() * m_scalePoint.y());
        Point point = Point(rect.x() * m_scalePoint.x(),
                            rect.y() * m_scalePoint.y());
        cvtRect = Rect(point, size);
    }
    else {
        Size size = Size(rect.width(), rect.height());
        Point point = Point(rect.x(), rect.y());
        cvtRect = Rect(point, size);
    }

    return cvtRect;
}

Mat ImageProcess::prepareImage(Mat sourceImage, Rect rect)
{
    Mat resultImage, temp;
    qint32 low = 0,     //55-65
           high = 173;  //170-180
    temp = Mat(sourceImage, rect);
    resize(temp, temp, Size(), 9, 6, INTER_CUBIC);
    inRange(temp, Scalar(0, low, 0, 0), Scalar(high, 255, 255, 255), resultImage);
    Size size(9,9);
    GaussianBlur(resultImage, resultImage, size, 0);
    bitwise_not(resultImage, resultImage);

    return resultImage;
}

QString ImageProcess::cleanText(QString text, bool isNumber)
{
    text.chop(2);
    text.replace(",", ".");
    text.replace("o", "0", Qt::CaseInsensitive);
    text.replace("I", "1", Qt::CaseInsensitive);
    text.replace("S", "5", Qt::CaseInsensitive);
    text.replace("R", "9", Qt::CaseInsensitive);
    text.replace(" 5", "");
    text.remove("‘");
    if (isNumber)
        text.remove(QRegExp("([^0-9.])"));

    return text;
}

QString ImageProcess::recognizeText(Mat textImage)
{
    m_tessApi->SetImage(textImage.data, textImage.size().width, textImage.size().height,
                     textImage.channels(), textImage.step1());
    return QString(m_tessApi->GetUTF8Text());
}

double ImageProcess::compareHistogram(const Mat baseImage, const Mat testImage)
{
    Mat hsvBase, hsvTest;
    MatND hist_base, hist_test;

    const int histChannels[] = {HIST_CHANNEL_1, HIST_CHANNEL_2};
    const int histSize[]     = {SATUR_BINS, VALUE_BINS};
    const float histRange1[] = {SATUR_RANGE_MIN, SATUR_RANGE_MAX};
    const float histRange2[] = {VALUE_RANGE_MIN, VALUE_RANGE_MAX};
    const float* histRange[] = {histRange1, histRange2};

    //Преобразование в диапазон цвета HSV
    cvtColor(baseImage, hsvBase, COLOR_BGR2HSV);
    cvtColor(testImage, hsvTest, COLOR_BGR2HSV);

    //Вычисление гистограмм для HSV изображений
    calcHist(&hsvBase, 1, histChannels, Mat(), hist_base, 2, histSize, histRange, true, false);
    normalize(hist_base, hist_base, 0, 1, NORM_MINMAX, -1, Mat());

    calcHist(&hsvTest, 1, histChannels, Mat(), hist_test, 2, histSize, histRange, true, false);
    normalize(hist_test, hist_test, 0, 1, NORM_MINMAX, -1, Mat());

    //Сравнение гистограмм
    return compareHist(hist_base, hist_test, 0);
}

QList<Rect> ImageProcess::matchingTemplate(const Mat baseImage, const Mat templImage, short counts)
{
    Mat resultImage;
    QList<Rect> matchRects;
    Point maxLoc, tempPoint;
    unsigned char breakCounter = 0;

    //Размеры результирующей матрицы после поиска
    int result_cols = baseImage.cols - templImage.cols + 1,
        result_rows = baseImage.rows - templImage.rows + 1;
    resultImage.create(result_cols, result_rows, CV_32FC1);

    //Выполнение поиска по шаблону и нормализация
    matchTemplate(baseImage, templImage, resultImage, CV_TM_CCOEFF);
    normalize(resultImage, resultImage, 0, 1, NORM_MINMAX, -1, Mat());
    //Поиск лучших совпадений
    while (matchRects.count() < counts) {
        minMaxLoc(resultImage, new double(), new double(), new Point(), &maxLoc, Mat());
        for (int j=0;j<templImage.size().width;j++) {
            for(int k=0;k<20;k++) {
                tempPoint.x = maxLoc.x - templImage.size().width/2 + j;
                tempPoint.y = maxLoc.y - 10 + k;
                if (tempPoint.x < 0) tempPoint.x = 0;
                if (tempPoint.y < 0) tempPoint.y = 0;
                if (tempPoint.x >= resultImage.size().width) tempPoint.x = resultImage.size().width-1;
                if (tempPoint.y >= resultImage.size().height) tempPoint.y = resultImage.size().height-1;
                resultImage.at<float>(tempPoint) = 0;
            }
        }
        Rect matchRect(maxLoc, Point(maxLoc.x + templImage.cols , maxLoc.y + templImage.rows));
        Mat matchImage(baseImage, matchRect);

        //Сравнение гистограмм шаблона и найденного изображения
        double histResult = compareHistogram(templImage, matchImage);
        if (histResult >= PLAYER_MATCH_SENS) {
            matchRects.append(matchRect);
        }
        else {
            breakCounter++;
            if (breakCounter > MAX_SEARCH_COUNT)
                break;
        }
    }

    return matchRects;
}

bool ImageProcess::setTableImage(Mat tableImage)
{
    bool result = false;

    if (tableImage.data) {

        if(!m_tableImage.data)
            m_scaleChanged = true;
        else if((m_tableImage.size().width!=tableImage.size().width)||
                (m_tableImage.size().height!=tableImage.size().height))
            m_scaleChanged = true;
        else
            m_scaleChanged = false;

        m_tableImage = tableImage.clone();

        m_scalePoint.setX(m_tableImage.size().width / (qreal)m_tableSettings.defaultPictureSize.width());
        m_scalePoint.setY(m_tableImage.size().height / (qreal)(m_tableSettings.defaultPictureSize.height()));

        result = true;
    }
    else
        result = false;

    return result;
}

bool ImageProcess::isValidImage()
{
    if (!m_tableImage.data)
        return false;

    Mat popupWindowImage;
    bool result = false;
    int red=0;
    int green=0;
    int blue=0;
    int brightness=0;
    Rect scaledRect = toCvtRect(m_tableSettings.popupWindowPartRect);

    popupWindowImage = Mat(m_tableImage, scaledRect);

    for (int y=0;y<popupWindowImage.rows;y++) {
        for (int x=0;x<popupWindowImage.cols;x++)
        {
            blue = blue+popupWindowImage.at<Vec3b>(y,x)[0];
            green = green+popupWindowImage.at<Vec3b>(y,x)[1];
            red = red+popupWindowImage.at<Vec3b>(y,x)[2];
        }
    }
    blue = blue/(popupWindowImage.rows*popupWindowImage.cols);
    green = green/(popupWindowImage.rows*popupWindowImage.cols);
    red = red/(popupWindowImage.rows*popupWindowImage.cols);
    brightness=(blue+green+red)/3;
    if(brightness<POPUP_BRIGHTNESS_MAX)
        result = true;

    return result;
}

double ImageProcess::getTotalPot()
{
    if (!m_tableImage.data)
        return 0;

    Mat potImage;
    QString potString;
    Rect scaledRect = toCvtRect(m_tableSettings.totalPotRect);

    potImage = prepareImage(m_tableImage, scaledRect);
#ifdef CHEAT_DOLLAR_DEL
    dollarDel(potImage);
#endif
    potString = cleanText(recognizeText(potImage), true);
#ifdef DRAW_RECTANGLE
    rectangle(m_tableImage, scaledRect, Scalar(0,0,255));
#endif

    return potString.toDouble();
}

double ImageProcess::getPlayerBet(Rect playerPosition, short playerNumber)
{
    Mat betImage;
    QString playerBet;
    Rect scaledRect = toCvtRect(m_tableSettings.playersSettings[playerNumber].betRect);
    scaledRect.x += playerPosition.x;
    scaledRect.y += playerPosition.y;

    betImage = prepareImage(m_tableImage, scaledRect);
#ifdef CHEAT_DOLLAR_DEL
    dollarDel(betImage);
#endif

    playerBet = cleanText(recognizeText(betImage), true);
#ifdef DRAW_RECTANGLE
    rectangle(m_tableImage, scaledRect, Scalar(0,0,255));
#endif

    return playerBet.toDouble();
}

bool ImageProcess::isButton(Rect playerPosition, short playerNumber)
{
    if(!m_buttonTemplate.data)
        return false;

    Mat buttonImage;
    bool result = false;
    Rect scaledRect = toCvtRect(m_tableSettings.playersSettings[playerNumber].buttonRect);
    scaledRect.x += playerPosition.x;
    scaledRect.y += playerPosition.y;

    buttonImage = Mat(m_tableImage, scaledRect);

    if ((playerNumber == 4 || playerNumber == 5) && currentTableSize == Table10Players) {
        double histResult = compareHistogram(buttonImage, m_button2Template);
        if (histResult >= BUTTON2_MATCH_SENS)
            result = true;
    }
    else {
        double histResult = compareHistogram(buttonImage, m_buttonTemplate);
        if (histResult >= BUTTON_MATCH_SENS)
            result = true;
    }

#ifdef DRAW_RECTANGLE
    rectangle(m_tableImage, scaledRect, Scalar(0,0,255), 1);
#endif
    return result;
}

QList<Card> ImageProcess::getPlayerCards(Rect playerPosition, short playerNumber)
{
    Mat cardImage, greyCardImage;
    QString cardText;
    QList<Card> cards;

    for (int i=0; i<m_tableSettings.playersSettings[playerNumber].cardsRects.count(); i++) {
        bool isCard = true;
        Card card;
        card.nominal = Card::NoNominal;
        card.suit = Card::NoSuit;
        Rect scaledRect = toCvtRect(m_tableSettings.playersSettings[playerNumber].cardsRects[i]);
        scaledRect.x += playerPosition.x;
        scaledRect.y += playerPosition.y;

        resize(Mat(m_tableImage, scaledRect), cardImage, Size(), 4/m_scalePoint.x(), 4/m_scalePoint.y());

        cvtColor(cardImage, greyCardImage, COLOR_BGR2GRAY);
        threshold(greyCardImage, greyCardImage, 137, 255, THRESH_TOZERO_INV);
        threshold(greyCardImage, greyCardImage, 0, 255, THRESH_BINARY_INV);
        for (int y=0;y<greyCardImage.rows;y++) {
            if(greyCardImage.at<int>(y,0) == 0)
                isCard = false;
        }
        cardText = cleanText(recognizeText(greyCardImage));
        cardText.remove('.');

        for (int i=0; i<cardText.count(); i++) {
            if (cardText[i] == ' ') {
                cardText.chop(cardText.count()-i);
            }
        }
        if (!cardText.isEmpty() && isCard) {
            if (cardText.at(0).isDigit())
                card.nominal = (Card::Nominals)cardText.toInt();
            else {
                quint16 nominal = cardText.at(0).unicode();
                switch (nominal) {
                case 'J':
                    card.nominal = Card::Jack;
                    break;
                case 'Q':
                    card.nominal = Card::Queen;
                    break;
                case 'K':
                    card.nominal = Card::King;
                    break;
                case 'A':
                    card.nominal = Card::Ace;
                    break;
                }
            }
            cvtColor(cardImage, cardImage, COLOR_RGB2BGR);
            cvtColor(cardImage, cardImage, COLOR_BGR2RGB);
            threshold(cardImage, cardImage, 137, 255, THRESH_TOZERO_INV);
            bool redFlag = false, blueFlag = false, greenFlag = false;
            for (int y=0;y<cardImage.rows;y++) {
                for (int x=0;x<cardImage.cols;x++) {
                    if (cardImage.at<Vec3b>(y,x)[0] != 0) {
                        blueFlag = true;
                    }
                    if (cardImage.at<Vec3b>(y,x)[1] != 0) {
                        greenFlag = true;
                    }
                    if (cardImage.at<Vec3b>(y,x)[2] != 0) {
                        redFlag = true;
                    }
                }
            }
            card.suit = Card::Spades;
            if (!blueFlag)
                card.suit = Card::Diamonds;
            if (!greenFlag)
                card.suit = Card::Clubs;
            if (!redFlag)
                card.suit = Card::Hearts;
        }
        cards.append(card);

#ifdef DRAW_RECTANGLE
        rectangle(m_tableImage, scaledRect, Scalar(255,0,0));
#endif
    }

    return cards;
}

double ImageProcess::getPlayerBank(Rect playerPosition, short playerNumber)
{
    Mat bankImage;
    QString playerBank;
    Rect scaledRect = toCvtRect(m_tableSettings.playersSettings[playerNumber].bankRect);
    scaledRect.x += playerPosition.x;
    scaledRect.y += playerPosition.y;

    bankImage = prepareImage(m_tableImage, scaledRect);
#ifdef CHEAT_DOLLAR_DEL
    dollarDel(bankImage);
#endif
    playerBank = recognizeText(bankImage);

    if (playerBank.contains('A', Qt::CaseInsensitive))
        playerBank = "(All-in)";
    else {
        playerBank = cleanText(playerBank, true);
    }

#ifdef DRAW_RECTANGLE
    rectangle(m_tableImage, scaledRect, Scalar(0,0,255), 1);
#endif
    return playerBank.toDouble();
}

QString ImageProcess::getPlayerName(Rect playerPosition, short playerNumber)
{
    Mat nameImage;
    QString playerName;
    Rect scaledRect = toCvtRect(m_tableSettings.playersSettings[playerNumber].nameRect);
    scaledRect.x += playerPosition.x;
    scaledRect.y += playerPosition.y;

    nameImage = prepareImage(m_tableImage, scaledRect);
    bitwise_not(nameImage, nameImage);

    playerName = recognizeText(nameImage);
    playerName.chop(2);
    playerName.remove(' ');
    playerName.remove('/');
    playerName.replace('|', 'I');
    if (playerName.isEmpty())
        playerName = "FOLDED";

#ifdef DRAW_RECTANGLE
    rectangle(m_tableImage, scaledRect, Scalar(0,0,255), 1);
#endif
    return playerName;
}

QList<PlayerData> ImageProcess::getPlayers()
{
    if(!m_playerTemplate.data || !m_activePlayerTemplate.data || !m_tableImage.data)
        return QList<PlayerData>();

    Mat playerTemplate, activePlayerTemplate;
    QList<PlayerData> players;

    //Обработка найденных игроков: опреление имени, банка, дилера
    resize(m_playerTemplate, playerTemplate, Size(), m_scalePoint.x(), m_scalePoint.y());
    resize(m_activePlayerTemplate, activePlayerTemplate, Size(), m_scalePoint.x(), m_scalePoint.y());
    for (int j=0;j<m_tableSettings.playersSettings.count();j++) {
        PlayerData player;
        Rect scaledRect = toCvtRect(m_tableSettings.playersSettings[j].playerRect);

        //rectangle(m_tableImage, Rect(scaledPlayerPoint, scaledPlayerSize), Scalar::all(0));
        double histResult = compareHistogram(Mat(m_tableImage, scaledRect), playerTemplate);
        double histResult2 = compareHistogram(Mat(m_tableImage, scaledRect), activePlayerTemplate);

        if (histResult >= PLAYER_MATCH_SENS || histResult2 >= ACT_PLAYER_MATCH_SENS) {
            if (histResult2 >= ACT_PLAYER_MATCH_SENS)
                player.current = true;
            else
                player.current = false;
            player.seatNumber = j;
            player.bet = getPlayerBet(scaledRect, j);
            player.button = isButton(scaledRect, j);
            player.cards = getPlayerCards(scaledRect, j);
            player.stack = getPlayerBank(scaledRect, j);
            if(player.current)
                player.name = "";
            else{
                player.name = getPlayerName(scaledRect, j);
            }
            if (!player.name.contains("FOLDED",Qt::CaseInsensitive))
                players.append(player);
        }
#ifdef DRAW_RECTANGLE
    rectangle(m_tableImage, scaledRect, Scalar(0,0,255), 1);
#endif
    }

#ifdef SHOW_IMAGE
    imshow("BaseImage", m_tableImage);
    //waitKey();
#endif

    return players;
}

QList<Card> ImageProcess::getCards()
{
    if (!m_tableImage.data)
        return QList<Card>();

    Mat cardImage, greyCardImage;
    QString cardText;
    QList<Card> cards;

    for (int i=0; i<m_tableSettings.cardsRects.count(); i++) {
        Card card;
        card.nominal = Card::NoNominal;
        card.suit = Card::NoSuit;
        Rect scaledRect = toCvtRect(m_tableSettings.cardsRects[i]);

        resize(Mat(m_tableImage, scaledRect), cardImage, Size(), 4/m_scalePoint.x(), 4/m_scalePoint.y());

        cvtColor(cardImage, greyCardImage, COLOR_BGR2GRAY);
        threshold(greyCardImage, greyCardImage, 137, 255, THRESH_TOZERO_INV);
        threshold(greyCardImage, greyCardImage, 0, 255, THRESH_BINARY_INV);
        cardText = cleanText(recognizeText(greyCardImage));
        cardText.remove('.');

        for (int i=0; i<cardText.count(); i++) {
            if (cardText[i] == ' ') {
                cardText.chop(cardText.count()-i);
            }
        }
        if (!cardText.isEmpty()) {
            if (cardText.at(0).isDigit())
                card.nominal = (Card::Nominals)cardText.toInt();
            else {
                quint16 nominal = cardText.at(0).unicode();
                switch (nominal) {
                case 'J':
                    card.nominal = Card::Jack;
                    break;
                case 'Q':
                    card.nominal = Card::Queen;
                    break;
                case 'K':
                    card.nominal = Card::King;
                    break;
                case 'A':
                    card.nominal = Card::Ace;
                    break;
                }
            }

            cvtColor(cardImage, cardImage, COLOR_RGB2BGR);
            cvtColor(cardImage, cardImage, COLOR_BGR2RGB);
            threshold(cardImage, cardImage, 137, 255, THRESH_TOZERO_INV);
            bool redFlag = false, blueFlag = false, greenFlag = false;
            for (int y=0;y<cardImage.rows;y++) {
                for (int x=0;x<cardImage.cols;x++) {
                    if (cardImage.at<Vec3b>(y,x)[0] != 0) {
                        blueFlag = true;
                    }
                    if (cardImage.at<Vec3b>(y,x)[1] != 0) {
                        greenFlag = true;
                    }
                    if (cardImage.at<Vec3b>(y,x)[2] != 0) {
                        redFlag = true;
                    }
                }
            }
            card.suit = Card::Spades;
            if (!blueFlag)
                card.suit = Card::Diamonds;
            if (!greenFlag)
                card.suit = Card::Clubs;
            if (!redFlag)
                card.suit = Card::Hearts;
        }
        cards.append(card);
#ifdef DRAW_RECTANGLE
        rectangle(m_tableImage, scaledRect, Scalar::all(0));
#endif
    }

    return cards;
}

QList<QRect> ImageProcess::getGameButtons()
{
    for (int i=0; i<m_tableSettings.buttonsRects.count(); ++i) {
        QSize size = QSize(m_tableSettings.buttonsRects[i].width() * m_scalePoint.x(),
                           m_tableSettings.buttonsRects[i].height() * m_scalePoint.y());
        QPoint point = QPoint(m_tableSettings.buttonsRects[i].x() * m_scalePoint.x(),
                              m_tableSettings.buttonsRects[i].y() * m_scalePoint.y());
        m_tableSettings.buttonsRects[i] = QRect(point, size);
    }
    return m_tableSettings.buttonsRects;
}

QRect ImageProcess::getBetInput()
{
    QSize size = QSize(m_tableSettings.betInputRect.width() * m_scalePoint.x(),
                       m_tableSettings.betInputRect.height() * m_scalePoint.y());
    QPoint point = QPoint(m_tableSettings.betInputRect.x() * m_scalePoint.x(),
                          m_tableSettings.betInputRect.y() * m_scalePoint.y());
    m_tableSettings.betInputRect = QRect(point, size);
    return m_tableSettings.betInputRect;
}

QRect ImageProcess::getOptionsTab()
{
    QSize size = QSize(m_tableSettings.optionsTabRect.width() * m_scalePoint.x(),
                       m_tableSettings.optionsTabRect.height() * m_scalePoint.y());
    QPoint point = QPoint(m_tableSettings.optionsTabRect.x() * m_scalePoint.x(),
                          m_tableSettings.optionsTabRect.y() * m_scalePoint.y());
    m_tableSettings.optionsTabRect = QRect(point, size);
    return m_tableSettings.optionsTabRect;
}

QRect ImageProcess::getChatTab()
{
    QSize size = QSize(m_tableSettings.chatTabRect.width() * m_scalePoint.x(),
                       m_tableSettings.chatTabRect.height() * m_scalePoint.y());
    QPoint point = QPoint(m_tableSettings.chatTabRect.x() * m_scalePoint.x(),
                          m_tableSettings.chatTabRect.y() * m_scalePoint.y());
    m_tableSettings.chatTabRect = QRect(point, size);
    return m_tableSettings.chatTabRect;
}

QRect ImageProcess::getRebuyTab()
{
    QSize size = QSize(m_tableSettings.rebuyTabRect.width() * m_scalePoint.x(),
                       m_tableSettings.rebuyTabRect.height() * m_scalePoint.y());
    QPoint point = QPoint(m_tableSettings.rebuyTabRect.x() * m_scalePoint.x(),
                          m_tableSettings.rebuyTabRect.y() * m_scalePoint.y());
    m_tableSettings.rebuyTabRect = QRect(point, size);
    return m_tableSettings.rebuyTabRect ;
}

QRect ImageProcess::getRebuyInput()
{
    QSize size = QSize(m_tableSettings.rebuyInputRect.width() * m_scalePoint.x(),
                       m_tableSettings.rebuyInputRect.height() * m_scalePoint.y());
    QPoint point = QPoint(m_tableSettings.rebuyInputRect.x() * m_scalePoint.x(),
                          m_tableSettings.rebuyInputRect.y() * m_scalePoint.y());
    m_tableSettings.rebuyInputRect = QRect(point, size);
    return m_tableSettings.rebuyInputRect ;
}

QRect ImageProcess::getRebuyButton()
{
    QSize size = QSize(m_tableSettings.rebuyButtonRect.width() * m_scalePoint.x(),
                       m_tableSettings.rebuyButtonRect.height() * m_scalePoint.y());
    QPoint point = QPoint(m_tableSettings.rebuyButtonRect.x() * m_scalePoint.x(),
                          m_tableSettings.rebuyButtonRect.y() * m_scalePoint.y());
    m_tableSettings.rebuyButtonRect = QRect(point, size);
    return m_tableSettings.rebuyButtonRect;
}

QRect ImageProcess::getSitOutNextBBcheckbox()
{
    QSize size = QSize(m_tableSettings.sitOutNextBBcheckboxRect.width() * m_scalePoint.x(),
                       m_tableSettings.sitOutNextBBcheckboxRect.height() * m_scalePoint.y());
    QPoint point = QPoint(m_tableSettings.sitOutNextBBcheckboxRect.x() * m_scalePoint.x(),
                          m_tableSettings.sitOutNextBBcheckboxRect.y() * m_scalePoint.y());
    m_tableSettings.sitOutNextBBcheckboxRect = QRect(point, size);
    return m_tableSettings.sitOutNextBBcheckboxRect;
}

QRect ImageProcess::getSitOutCheckbox()
{
    QSize size = QSize(m_tableSettings.sitOutCheckboxRect.width() * m_scalePoint.x(),
                       m_tableSettings.sitOutCheckboxRect.height() * m_scalePoint.y());
    QPoint point = QPoint(m_tableSettings.sitOutCheckboxRect.x() * m_scalePoint.x(),
                          m_tableSettings.sitOutCheckboxRect.y() * m_scalePoint.y());
    m_tableSettings.sitOutCheckboxRect = QRect(point, size);
    return m_tableSettings.sitOutCheckboxRect;
}

QRect ImageProcess::getPopupWindowButton()
{
    QSize size = QSize(m_tableSettings.popupWindowButtonRect.width() * m_scalePoint.x(),
                       m_tableSettings.popupWindowButtonRect.height() * m_scalePoint.y());
    QPoint point = QPoint(m_tableSettings.popupWindowButtonRect.x() * m_scalePoint.x(),
                          m_tableSettings.popupWindowButtonRect.y() * m_scalePoint.y());
    m_tableSettings.popupWindowButtonRect = QRect(point, size);
    return m_tableSettings.popupWindowButtonRect;
}

QRect ImageProcess::getPopupWindowPart()
{
    QSize size = QSize(m_tableSettings.popupWindowPartRect.width() * m_scalePoint.x(),
                       m_tableSettings.popupWindowPartRect.height() * m_scalePoint.y());
    QPoint point = QPoint(m_tableSettings.popupWindowPartRect.x() * m_scalePoint.x(),
                          m_tableSettings.popupWindowPartRect.y() * m_scalePoint.y());
    m_tableSettings.popupWindowPartRect = QRect(point, size);
    return m_tableSettings.popupWindowPartRect;
}

void ImageProcess::processImage(Mat tableImage, Table *table)
{
//    static Mat prevImage = tableImage;

    if (!setTableImage(tableImage))
        return;

    if(m_scaleChanged)
    {
        m_tableSettings = settingsParser.parseConfigFile(currentTableSize, ":/config.xml");

        table->gameButtons = getGameButtons();
        table->betInput = getBetInput();
        table->optionsTab = getOptionsTab();
        table->chatTab = getChatTab();
        table->rebuyTab = getRebuyTab();
        table->rebuyInput = getRebuyInput();
        table->rebuyButton = getRebuyButton();
        table->sitOutNextBBcheckbox = getSitOutNextBBcheckbox();
        table->sitOutCheckbox = getSitOutCheckbox();
		table->popupWindowButton = getPopupWindowButton();
        table->popupWindowPart = getPopupWindowPart();
    }

    if(!isValidImage())
        table->valid = false;
    else
        table->valid = true;

    table->totalPot = getTotalPot();
    table->cards = getCards();
    table->playersData = getPlayers();
    for (int i=0; i<table->playersData.count();i++)
        for (int j=i+1; j<table->playersData.count();j++)
            if (table->playersData[j].seatNumber < table->playersData[i].seatNumber)
                table->playersData.swap(i, j);

    //Тормозит цикл обработки и отображает предыдущее изображение при отладке
//    imshow("ff", prevImage);
//    waitKey();
//    prevImage = tableImage;
}

void ImageProcess::processImage(QImage tableImage, Table *table)
{
    if (tableImage.format() != QImage::Format_RGB32) {
        tableImage = tableImage.convertToFormat(QImage::Format_RGB32);
    }
    Mat newImage = imageToMat(tableImage);
    processImage(newImage, table);
}

void ImageProcess::tableSizeChanged(QString tableSize)
{
    TableSize size;

    switch(tableSize.toInt()) {
    case 6:
        size = Table6Players;
        break;
    case 9:
        size = Table9Players;
        break;
    case 10:
    default:
        size = Table10Players;
        break;
    }
    setTableSize(size);
}
