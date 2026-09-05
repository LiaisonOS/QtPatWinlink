//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Action drop-down menu
//

#include "ActionMenu.h"
#include <QAction>

ActionMenu::ActionMenu(bool touchMode, QWidget *parent)
    : QMenu(parent)
{
    int fontSize = touchMode ? 14 : 10;
    int padV     = touchMode ? 11 : 8;
    int padH     = touchMode ? 32 : 24;
    int itemMinH = touchMode ? 36 : 0;

    setStyleSheet(QString(
        "QMenu { background: #1e1e1e; color: #e0e0e0; border: 1px solid #333333; font-size: %1pt; }"
        "QMenu::item:selected { background: #ffa500; color: #000000; }"
        "QMenu::item { padding: %2px %3px; min-height: %4px; }"
        "QMenu::separator { height: 1px; background: #333333; margin: 6px 12px; }"
    ).arg(fontSize).arg(padV).arg(padH).arg(itemMinH));

    auto *actConnect       = addAction("Connect to RMS");
    auto *actP2PConnect    = addAction("Connect to Peer…");
    auto *actCompose       = addAction("Compose");
    addSeparator();
    auto *actPosition      = addAction("Send Position Report");
    auto *actFormsUpdate   = addAction("Update Form Templates");
    auto *actOpenBrowser   = addAction("Open in Browser");
    addSeparator();
    auto *actP2PListen     = addAction("P2P Listen…");
    addSeparator();
    auto *actAbout         = addAction("About");
    addSeparator();
    auto *actClose         = addAction("Close");

    QObject::connect(actConnect,      &QAction::triggered, this, &ActionMenu::connectRequested);
    QObject::connect(actP2PConnect,   &QAction::triggered, this, &ActionMenu::p2pConnectRequested);
    QObject::connect(actCompose,      &QAction::triggered, this, &ActionMenu::composeRequested);
    QObject::connect(actPosition,     &QAction::triggered, this, &ActionMenu::positionRequested);
    QObject::connect(actFormsUpdate,  &QAction::triggered, this, &ActionMenu::formsUpdateRequested);
    QObject::connect(actOpenBrowser,  &QAction::triggered, this, &ActionMenu::openInBrowserRequested);
    QObject::connect(actP2PListen,    &QAction::triggered, this, &ActionMenu::p2pListenRequested);
    QObject::connect(actClose,        &QAction::triggered, this, &ActionMenu::closeRequested);
    QObject::connect(actAbout,        &QAction::triggered, this, &ActionMenu::aboutRequested);
}
