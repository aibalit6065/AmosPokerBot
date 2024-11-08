#ifndef SYSTEMAPI_H
#define SYSTEMAPI_H

#include <QImage>
#include <QPixmap>
#include <QPoint>
#include <QString>
#include <QtMath>
#include <windows.h>

#define X_IMG_OFFSET 2
#define Y_IMG_OFFSET 29

class SystemApi: public QObject
{
    Q_OBJECT
public:
typedef WId WindowHandle;

    static WindowHandle handleFromPoint(QPoint point);
    static WindowHandle handleFromString(QString windowName);
    static WindowHandle ancestorHandle(WindowHandle childHandle);
    static QString windowText(WindowHandle handle);

    SystemApi();
    ~SystemApi();

    WindowHandle handle();
    bool setHandle(WindowHandle handle);
    void moveMouse(QPoint p, double delay = 0);
    void randMoveMouse(QRect rect);
    void clickMouse();
    void moveAndClickMouse(QPoint p, double delay = 0);
    void sendAltF4();
    void clearText();
    void toEnglish();
    void typeText(QString text);
    QImage takeScreenShot();
    QImage lastScreenShot();

public slots:
    void saveLastScreenShot();

private:
    WindowHandle m_handle;
    WindowHandle m_actionHandle;
    QPixmap m_screenShot;
	QString m_windowClassName;
};

#endif // SYSTEMAPI_H
