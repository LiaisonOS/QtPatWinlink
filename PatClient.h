//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Pat Winlink REST + WebSocket client
//

#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QWebSocket>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

class PatClient : public QObject
{
    Q_OBJECT

public:
    explicit PatClient(const QString &baseUrl = "http://localhost:8080", QObject *parent = nullptr);

    QString baseUrl() const { return m_baseUrl; }
    void    setFormInstanceCookie(const QString &key);

    // Mailbox
    void fetchMailbox(const QString &box);           // in / out / sent / archive
    void fetchMessage(const QString &box, const QString &mid);
    void deleteMessage(const QString &box, const QString &mid);
    void markRead(const QString &box, const QString &mid);
    void postMessage(const QString &to, const QString &subject, const QString &body,
                     const QString &cc = {}, const QStringList &attachments = {},
                     bool p2pOnly = false);

    // Connect
    void fetchRmsList(const QString &band = {}, const QString &mode = {}, bool forceDownload = false);
    void connect(const QString &connectUrl);
    void disconnect();
    void fetchStatus();

    // Position
    void fetchPosition();
    void postPositionReport(const QJsonObject &pos);

    // Forms
    void updateFormTemplates();
    void fetchFormCatalog();
    void fetchFormData();    // GET /api/form — retrieves the most recently submitted form body

    // Config (for P2P Listen mode)
    void fetchConfig();                              // GET /api/config → configReady
    void updateConfigListen(const QStringList &modems); // PUT /api/config with new Listen[]
    void reloadConfig();                             // POST /api/reload — no restart needed

    // WebSocket
    void connectWebSocket();
    bool isWebSocketConnected() const;

signals:
    void wsConnectedNow();          // emitted from onWsConnected so headless callers can wait for WS
    void mailboxReady(const QString &box, const QJsonArray &messages);
    void messageReady(const QString &box, const QString &mid, const QJsonObject &message);
    void messageDeleted(const QString &box, const QString &mid);
    void messagePosted();
    void messagePostFailed(const QString &detail);
    void rmsListReady(const QJsonArray &stations);
    void statusReady(const QJsonObject &status);
    // Fired when the /api/connect long-poll returns — i.e. Pat's session
    // goroutine has fully finished and all received mail is written to disk.
    // `error` is empty on a clean HTTP 200 finish, or carries the transport
    // error string (Pat returns HTTP 500 even on clean termination). This is
    // the authoritative end-of-session signal for headless missions.
    void connectFinished(const QString &error);
    void positionReady(const QJsonObject &pos);
    void formTemplatesUpdated();
    void formCatalogReady(const QJsonObject &catalog);
    void formDataReady(const QString &to, const QString &cc,
                       const QString &subject, const QString &body);
    void wsEvent(const QJsonObject &event);
    // Fired when Pat's WebSocket reports a mailbox directory change
    // (Pat emits { "UpdateMailbox": true } from its fsnotify watcher).
    // Debounced by Pat to ~100ms. Views should call their own refresh().
    void mailboxUpdated();
    void configReady(const QJsonObject &config);
    void configUpdated();
    void configReloaded();
    void error(const QString &message);

private slots:
    void onWsConnected();
    void onWsDisconnected();
    void onWsTextMessageReceived(const QString &message);
    void onWsError(QAbstractSocket::SocketError err);

private:
    QNetworkReply *get(const QString &path);
    QNetworkReply *post(const QString &path, const QByteArray &body = {});
    QNetworkReply *del(const QString &path);
    void handleReply(QNetworkReply *reply, std::function<void(const QByteArray &)> handler);

    QNetworkAccessManager *m_nam;
    QWebSocket            *m_ws;
    QString                m_baseUrl;
    QString                m_wsUrl;
};
