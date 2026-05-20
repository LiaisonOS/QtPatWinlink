//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Session console — shows live protocol exchange during Winlink connect
//

#pragma once

#include <QDialog>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QJsonObject>
#include <QTimer>
#include "../PatClient.h"

class SessionConsole : public QDialog
{
    Q_OBJECT

public:
    explicit SessionConsole(PatClient *client, bool touchMode, QWidget *parent = nullptr);

public slots:
    void onWsEvent(const QJsonObject &event);

signals:
    void sessionDone(bool wasConnected);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onDisconnectClicked();
    void onSessionTrulyEnded();

private:
    void appendLog(const QString &line);
    void updateProgress(const QJsonObject &progress);
    void updateStatus(const QJsonObject &status);

    PatClient      *m_client;
    bool            m_touchMode;

    QLabel         *m_statusLabel;
    QLabel         *m_midLabel;
    QProgressBar   *m_progressBar;
    QPlainTextEdit *m_console;
    QPushButton    *m_disconnectBtn;
    QPushButton    *m_closeBtn;

    bool            m_connected = false;
    bool            m_sessionEnded = false;
    QTimer         *m_endedDebounce = nullptr;
    QString         m_lastLine;
    int             m_repeatCount = 0;
};
