QT       += core gui sql
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QPDFMerge
TEMPLATE = app

!contains(sql-drivers, sqlite): QTPLUGIN += qsqlite

SOURCES += main.cpp\
        mainwindow.cpp \
    gameengine.cpp \
    player.cpp \
    imageprocess.cpp \
    systemApi.cpp \
    settingsparser.cpp \
    gamestatus.cpp \
    statisticsdb.cpp \
    strategy.cpp

HEADERS  += mainwindow.h \
    gamedefs.h \
    gameengine.h \
    player.h \
    imageprocess.h \
    systemApi.h \
    settingsparser.h \
    gamestatus.h \
    statisticsdb.h \
    strategy.h

LIBS += -L$$_PRO_FILE_PWD_/libs -lopencv_core320.dll \
        -lopencv_highgui320.dll -lopencv_imgproc320.dll \
        -lopencv_imgcodecs320.dll -ltesseract

FORMS    += mainwindow.ui

RESOURCES += resources.qrc

RC_FILE = icon.rc
