QT += core gui widgets network websockets webenginewidgets

TARGET   = QtPatWinlink
TEMPLATE = app

CONFIG  += c++17

QTWEBSOCKETS_LIB = /home/va2ops/LiaisonOS/libs/qtwebsockets/build/lib/x86_64-linux-gnu/libQt6WebSockets.a
QTWEBSOCKETS_INC = /home/va2ops/LiaisonOS/libs/qtwebsockets/build/include/x86_64-linux-gnu/QtWebSockets

INCLUDEPATH += $$QTWEBSOCKETS_INC
LIBS        += $$QTWEBSOCKETS_LIB

SOURCES += \
    main.cpp \
    MainWindow.cpp \
    PatClient.cpp \
    RmsPrefs.cpp \
    MissionBroadcaster.cpp \
    MissionRunner.cpp \
    views/InboxView.cpp \
    views/OutboxView.cpp \
    views/SentView.cpp \
    views/ArchiveView.cpp \
    views/ComposeView.cpp \
    views/ConnectView.cpp \
    views/ActionMenu.cpp \
    views/MessageDialog.cpp \
    views/SessionConsole.cpp \
    views/FormPickerDialog.cpp \
    views/FormRenderDialog.cpp \
    views/PositionReportDialog.cpp \
    views/ManualRmsDialog.cpp

HEADERS += \
    MainWindow.h \
    PatClient.h \
    RmsPrefs.h \
    TouchStyle.h \
    MissionBroadcaster.h \
    MissionRunner.h \
    views/InboxView.h \
    views/OutboxView.h \
    views/SentView.h \
    views/ArchiveView.h \
    views/ComposeView.h \
    views/ConnectView.h \
    views/ActionMenu.h \
    views/MessageDialog.h \
    views/SessionConsole.h \
    views/LongPressFilter.h \
    views/FormPickerDialog.h \
    views/FormRenderDialog.h \
    views/PositionReportDialog.h \
    views/ManualRmsDialog.h
