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
                     const QString &cc = {}, const QStringList &attachments = {});

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

    // WebSocket
    void connectWebSocket();

signals:
    void mailboxReady(const QString &box, const QJsonArray &messages);
    void messageReady(const QString &box, const QString &mid, const QJsonObject &message);
    void messageDeleted(const QString &box, const QString &mid);
    void messagePosted();
    void messagePostFailed(const QString &detail);
    void rmsListReady(const QJsonArray &stations);
    void statusReady(const QJsonObject &status);
    void positionReady(const QJsonObject &pos);
    void formTemplatesUpdated();
    void formCatalogReady(const QJsonObject &catalog);
    void formDataReady(const QString &to, const QString &cc,
                       const QString &subject, const QString &body);
    void wsEvent(const QJsonObject &event);
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
