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

    // Called by the caller (ConnectView) right after starting a session so
    // the console can offer a "Retry without QSY" fallback if PAT's log
    // stream shows a hamlib/rigctl failure.
    void setConnectUrl(const QString &url) { m_originalUrl = url; }

public slots:
    void onWsEvent(const QJsonObject &event);

signals:
    void sessionDone(bool wasConnected);
    // Fired when the operator clicks "Skip QSY & Retry" after a hamlib
    // failure — payload is the ORIGINAL connect URL (still with ?freq=).
    // Caller is expected to strip the freq/bw params and reconnect.
    void retryWithoutQsy(const QString &originalUrl);

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
    QPushButton    *m_retryNoQsyBtn;    // hidden until hamlib failure detected
    QLabel         *m_qsyErrorBanner;   // red bar shown alongside the retry btn

    bool            m_connected = false;
    bool            m_sessionEnded = false;
    bool            m_qsyFailDetected = false; // one-shot to avoid re-showing
    QTimer         *m_endedDebounce = nullptr;
    QString         m_lastLine;
    QString         m_originalUrl;       // set via setConnectUrl()
    int             m_repeatCount = 0;
};
