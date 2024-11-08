#include "SystemApi.h"
#include <QCursor>
#include <QDateTime>
#include <QScreen>
#include <QGuiApplication>
#include <QDebug>
#include <QLibrary>

#define USB_PID_VID "vid_046d&pid_c52b"
#define USB_DESCRIPTOR "USB_Receiver USB_Receiver"
#define USB_DOWN_KEY 1
#define USB_UP_KEY 8
#define USB_MOUSE 3
#define HID_DOWN_LBM 1
#define HID_RELEASE_MOUSEBTN 0
#define HID_CTRL 224
#define HID_SHIFT 225
#define HID_ALT 226
#define HID_COMMA 54
#define HID_DOT 55
#define HID_ZERO 39
#define HID_DELETE 76
#define HID_BACKSPACE 42
#define HID_ENTER 40
#define HID_F4 61
#define HID_A 4





QLibrary usbLib("mydll.dll");
typedef int (*__stdcall UsbPrototype)(int, int, int, int, char*, char*);
UsbPrototype sendUsb = (UsbPrototype) usbLib.resolve("Sum");

SystemApi::WindowHandle SystemApi::handleFromPoint(QPoint point)
{
    HWND winHandle;
    POINT winPoint;

    winPoint.x = point.x();
    winPoint.y = point.y();
    winHandle = WindowFromPoint(winPoint);

    return (WindowHandle)winHandle;
}

SystemApi::WindowHandle SystemApi::handleFromString(QString windowName)
{
    HWND winHandle = FindWindow(0,0);
    LPWSTR lpString = (TCHAR*)malloc(128*sizeof(TCHAR));
    QString string;

    do {
        GetWindowText(winHandle, lpString, GetWindowTextLength(winHandle)+1);
        string = QString::fromWCharArray(lpString);
        if(string.contains(windowName, Qt::CaseInsensitive))
            break;
        else
            winHandle = GetWindow(winHandle, GW_HWNDNEXT);
    } while (winHandle != 0);
    free(lpString);

    return (WindowHandle)winHandle;
}

SystemApi::WindowHandle SystemApi::ancestorHandle(WindowHandle handle)
{
    HWND parentHandle = 0;

    if (handle)
        parentHandle = GetAncestor((HWND)handle, GA_ROOT);

    return (WindowHandle)parentHandle;
}

QString SystemApi::windowText(WindowHandle handle)
{
    QString result;

    if (handle) {
        std::vector<TCHAR> buff(1024);
        GetWindowText((HWND)handle, buff.data(), 1022);
        result = QString::fromWCharArray(buff.data());
    }

    return result;
}

SystemApi::SystemApi()
{
    m_handle = 0;
}

SystemApi::~SystemApi()
{

}

SystemApi::WindowHandle SystemApi::handle()
{
	return m_handle;
}

bool SystemApi::setHandle(WindowHandle handle)
{
    bool result = false;

    if (handle) {
        m_handle = handle;
        result = true;
	}

    return result;
}

void SystemApi::moveMouse(QPoint p, double delay)
{
    LPRECT rect = new RECT;
    GetWindowRect((HWND)m_handle, rect);
    p.setX(p.x() + rect->left + X_IMG_OFFSET);
    p.setY(p.y() + rect->top + Y_IMG_OFFSET);

    if( delay == 0 ) //delay in seconds
    {
        sendUsb(USB_MOUSE,HID_RELEASE_MOUSEBTN,p.x(),p.y(),USB_PID_VID, USB_DESCRIPTOR);
        ::Sleep(25);
    }
	else
    {
        int count; //point number
        int x, y;
        double deltaX, deltaY;

        if(delay > 1) //rounded up to 1 second
            count = 40;
        else
            count = delay*1000/25; //step 25 milliseconds

        deltaX = (p.x()-QCursor::pos().x())/count;
        deltaY = (p.y()-QCursor::pos().y())/count;

        for( int n=0; n<count; n++)
        {
            x = QCursor::pos().x()+deltaX;
            y = QCursor::pos().y()+deltaY;
            if((n==count-1)&&((x!=p.x())||(y!=p.y())))
            {
                x = p.x();
                y = p.y();
            }

            sendUsb(USB_MOUSE,HID_RELEASE_MOUSEBTN,x,y,USB_PID_VID, USB_DESCRIPTOR);
            ::Sleep(25);
        }
/*
        double ax, ay, vx, vy, t, delta;
        // x1(t)= x0+ax*(t^2)/2
        // x2(t)=x0+(x1-x0)/2+vx*t-ax(t^2)/2
        ax = (4*p.x()-4*cursorPos.x())/(delay*delay);
        vx = (2*p.x()-2*cursorPos.x())/delay;
        ay = (4*p.y()-4*cursorPos.y())/(delay*delay);
        vy = (2*p.y()-2*cursorPos.y())/delay;

        // delta = delay/S
        delta = delay/qSqrt((qreal)((p.x()-cursorPos.x())*(p.x()-cursorPos.x())+ \
                                    (p.y()-cursorPos.y())*(p.y()-cursorPos.y())));

        for( t = 0; t < delay/2; t+=delta ) // first half path
        {
            x = cursorPos.x() + ax*(t*t)/2;
            y = cursorPos.y() + ay*(t*t)/2;

            QCursor::setPos(x,y);
            ::Sleep(delta*1000);
        }

        for( t = 0; t < delay/2; t+=delta ) // second half path
        {
            x = cursorPos.x() + (p.x()-cursorPos.x())/2 + vx*t - ax*(t*t)/2;
            y = cursorPos.y() + (p.y()-cursorPos.y())/2 + vy*t - ay*(t*t)/2;

            QCursor::setPos(x,y);
            ::Sleep(delta*1000);
        }
*/
	}

    qDebug()<<"Mouse moved";
}

void SystemApi::randMoveMouse(QRect rect)
{
    QPoint movePoint(rect.x() + (qrand()%rect.width()), rect.y() + (qrand()%rect.height()));
    moveMouse(movePoint, 0.5);
    qDebug()<<"move x"<<rect.x()<<"y"<<rect.y();
}

void SystemApi::clickMouse()
{
    sendUsb(USB_MOUSE,HID_DOWN_LBM,QCursor::pos().x(),QCursor::pos().y(),USB_PID_VID, USB_DESCRIPTOR);
    ::Sleep(25);
    qDebug()<<"mouse left down";
    sendUsb(USB_MOUSE,HID_RELEASE_MOUSEBTN,QCursor::pos().x(),QCursor::pos().y(),USB_PID_VID, USB_DESCRIPTOR);
    ::Sleep(25);
    qDebug()<<"mouse left up";
}

void  SystemApi::moveAndClickMouse(QPoint p, double delay)
{
    moveMouse(p, delay);
    ::Sleep(50);
    clickMouse();
    ::Sleep(50);
}

void SystemApi::sendAltF4()
{
    //winApi переводит поток, который создал заданное окно в приоритетный режим
    //и активизирует окно
    SetForegroundWindow((HWND)m_handle);
    ::Sleep(500);

    sendUsb(USB_DOWN_KEY,HID_ALT,0,0,USB_PID_VID, USB_DESCRIPTOR);
    ::Sleep(25);
    sendUsb(USB_DOWN_KEY,HID_F4,0,0,USB_PID_VID, USB_DESCRIPTOR);
    ::Sleep(25);
    sendUsb(USB_UP_KEY,HID_F4,0,0,USB_PID_VID, USB_DESCRIPTOR);
    ::Sleep(25);
    sendUsb(USB_UP_KEY,HID_ALT,0,0,USB_PID_VID, USB_DESCRIPTOR);
    ::Sleep(500);
}

void SystemApi::clearText()
{
    qDebug()<<"clearText() start";

    ::Sleep(400);
    clickMouse();
    clickMouse();
    ::Sleep(200);

    sendUsb(USB_DOWN_KEY,HID_BACKSPACE,0,0,USB_PID_VID, USB_DESCRIPTOR);
    ::Sleep(25);
    sendUsb(USB_UP_KEY,HID_BACKSPACE,0,0,USB_PID_VID, USB_DESCRIPTOR);
    ::Sleep(25);

    qDebug()<<"clearText() end";
}

void SystemApi::toEnglish()
{
    DWORD ThreadID;             //переменная получения нужного потока
    HKL ThreadLayout;           //переменная раскладки клавиатуры в нужном потоке
    HKL ThreadLayoutRus=::LoadKeyboardLayoutA("00000419",0); //переменная русской раскладки
    HWND hwnd=::GetForegroundWindow(); //хэндл активного окна
    qDebug()<<"toEnglish() start";

    //получаем ID нужного процесса
    ThreadID=::GetWindowThreadProcessId(hwnd,0);
    //получаем текущую раскладку для полученного процесса
    ThreadLayout=::GetKeyboardLayout(ThreadID);

    qDebug("ThreadLayoutRus %08x, ThreadLayout %08X, ThreadID %08X, hwnd %08X", ThreadLayoutRus, ThreadLayout, ThreadID, hwnd);

    if (ThreadLayout==ThreadLayoutRus) {
        qDebug()<<"perekluchenie";
        sendUsb(USB_DOWN_KEY,HID_ALT,0,0,USB_PID_VID, USB_DESCRIPTOR);
        ::Sleep(25);
        sendUsb(USB_DOWN_KEY,HID_SHIFT,0,0,USB_PID_VID, USB_DESCRIPTOR);
        ::Sleep(25);
        sendUsb(USB_UP_KEY,HID_SHIFT,0,0,USB_PID_VID, USB_DESCRIPTOR);
        ::Sleep(25);
        sendUsb(USB_UP_KEY,HID_ALT,0,0,USB_PID_VID, USB_DESCRIPTOR);
        ::Sleep(25);
    };
    qDebug()<<"toEnglish() end";
}

void SystemApi::typeText(QString text)
{
    text.replace(".",",");
    QByteArray ba = text.toLatin1();
	
    //очистка поля ввода
    clearText();
    clearText();

    //переключение раскладки на английскую по умолчанию
    toEnglish();

    //ввод текста
    for(int i=0; i<text.size(); i++)
    {
        if(ba[i]>=65 && ba[i]<=90) //upper case
        {
            sendUsb(USB_DOWN_KEY,HID_SHIFT,0,0,USB_PID_VID, USB_DESCRIPTOR); //down shift
            ::Sleep(70);
            sendUsb(USB_DOWN_KEY,ba[i]-61,0,0,USB_PID_VID, USB_DESCRIPTOR);
            ::Sleep(70);
            sendUsb(USB_UP_KEY,ba[i]-61,0,0,USB_PID_VID, USB_DESCRIPTOR);
            ::Sleep(70);
            sendUsb(USB_UP_KEY,HID_SHIFT,0,0,USB_PID_VID, USB_DESCRIPTOR); //up shift
            ::Sleep(70);
        }
        else if(ba[i]>=97 && ba[i]<=122) //lower case
        {
            sendUsb(USB_DOWN_KEY,ba[i]-93,0,0,USB_PID_VID, USB_DESCRIPTOR);
            ::Sleep(70);
            sendUsb(USB_UP_KEY,ba[i]-93,0,0,USB_PID_VID, USB_DESCRIPTOR);
            ::Sleep(70);
        }
        else if(ba[i]>=49 && ba[i]<=57) //number without 0
        {
            sendUsb(USB_DOWN_KEY,ba[i]-19,0,0,USB_PID_VID, USB_DESCRIPTOR);
            ::Sleep(70);
            sendUsb(USB_UP_KEY,ba[i]-19,0,0,USB_PID_VID, USB_DESCRIPTOR);
            ::Sleep(70);
        }
        else if(ba[i]==48) //0
        {
            sendUsb(USB_DOWN_KEY,HID_ZERO,0,0,USB_PID_VID, USB_DESCRIPTOR);
            ::Sleep(70);
            sendUsb(USB_UP_KEY,HID_ZERO,0,0,USB_PID_VID, USB_DESCRIPTOR);
            ::Sleep(70);
        }
        else if(ba[i]==46) //.
        {
            sendUsb(USB_DOWN_KEY,HID_DOT,0,0,USB_PID_VID, USB_DESCRIPTOR);
            ::Sleep(70);
            sendUsb(USB_UP_KEY,HID_DOT,0,0,USB_PID_VID, USB_DESCRIPTOR);
            ::Sleep(70);
        }
        else if(ba[i]==44) //,
        {
            sendUsb(USB_DOWN_KEY,HID_COMMA,0,0,USB_PID_VID, USB_DESCRIPTOR);
            ::Sleep(70);
            sendUsb(USB_UP_KEY,HID_COMMA,0,0,USB_PID_VID, USB_DESCRIPTOR);
            ::Sleep(70);
        }
    }
}

QImage SystemApi::takeScreenShot()
{
    QScreen *screen = QGuiApplication::primaryScreen();

    if (screen && m_handle)
        m_screenShot = screen->grabWindow((WId)m_handle);

    return m_screenShot.toImage();
}

QImage SystemApi::lastScreenShot()
{
    return m_screenShot.toImage();
}

void SystemApi::saveLastScreenShot()
{
    QString fileName = QDateTime::currentDateTime().toString("yyyy.MM.dd_hh-mm-ss")+".png";
    qDebug()<<"saveLastScreenShot";

    if (!fileName.isEmpty() && !m_screenShot.isNull()){
        m_screenShot.save(fileName);
    }
    else
        qDebug()<<"error while saving screenshot";
}
