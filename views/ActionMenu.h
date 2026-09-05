//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Action drop-down menu
//

#pragma once

#include <QMenu>

class ActionMenu : public QMenu
{
    Q_OBJECT

public:
    explicit ActionMenu(bool touchMode, QWidget *parent = nullptr);

signals:
    void composeRequested();
    void connectRequested();
    void positionRequested();
    void formsUpdateRequested();
    void openInBrowserRequested();
    void p2pListenRequested();
    void p2pConnectRequested();
    void aboutRequested();
    void closeRequested();
};
