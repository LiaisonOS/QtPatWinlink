//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : QtPatWinlink main window — Desktop and Touch UI
//

#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include "PatClient.h"
#include "views/InboxView.h"
#include "views/OutboxView.h"
#include "views/SentView.h"
#include "views/ArchiveView.h"
#include "views/ComposeView.h"
#include "views/ConnectView.h"
#include "views/ActionMenu.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const QString &patUrl, bool touchMode,
                        const QString &band = {}, const QString &modem = {},
                        bool mailViewer = false,
                        QWidget *parent = nullptr);

private slots:
    void showInbox();
    void showOutbox();
    void showSent();
    void showArchive();
    void showCompose(const QString &to = {}, const QString &subject = {}, const QString &body = {});
    void showConnect();
    void onStatusReady(const QJsonObject &status);
    void onWsEvent(const QJsonObject &event);

private:
    void buildUI();
    void setActiveTab(QPushButton *btn);
    void setStatusIdle(const QString &text, const QString &color = "#888888");
    void setStatusActivity(const QString &text);
    void handoffToBrowser();
    bool notifySupervisorIgnoreExit();
    bool launchBrowser(const QString &url);

    PatClient      *m_client;
    bool            m_touchMode;
    bool            m_mailViewer = false;  // true = read-only mailbox UI (no Connect)
    QString         m_band;
    QString         m_modem;

    QStackedWidget *m_stack;
    QLabel         *m_statusLabel;

    InboxView      *m_inbox;
    OutboxView     *m_outbox;
    SentView       *m_sent;
    ArchiveView    *m_archive;
    ComposeView    *m_compose;
    ConnectView    *m_connect;
    ActionMenu     *m_actionMenu;

    QPushButton    *m_btnInbox;
    QPushButton    *m_btnOutbox;
    QPushButton    *m_btnSent;
    QPushButton    *m_btnArchive;
    QPushButton    *m_btnAction;
};
