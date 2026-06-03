//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Pat Winlink REST + WebSocket client
//

#include "PatClient.h"
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QUrl>
#include <QTimer>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QNetworkCookie>
#include <QNetworkCookieJar>

PatClient::PatClient(const QString &baseUrl, QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_ws(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
    , m_baseUrl(baseUrl)
{
    QObject::connect(m_ws, &QWebSocket::connected,          this, &PatClient::onWsConnected);
    QObject::connect(m_ws, &QWebSocket::disconnected,       this, &PatClient::onWsDisconnected);
    QObject::connect(m_ws, &QWebSocket::textMessageReceived,this, &PatClient::onWsTextMessageReceived);
    QObject::connect(m_ws, &QWebSocket::errorOccurred,      this, &PatClient::onWsError);
}

// ── Mailbox ──────────────────────────────────────────────────────────────────

void PatClient::fetchMailbox(const QString &box)
{
    auto *reply = get("/api/mailbox/" + box);
    handleReply(reply, [this, box](const QByteArray &data) {
        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError) {
            emit error("mailbox parse error: " + err.errorString());
            return;
        }
        emit mailboxReady(box, doc.array());
    });
}

void PatClient::fetchMessage(const QString &box, const QString &mid)
{
    auto *reply = get("/api/mailbox/" + box + "/" + mid);
    handleReply(reply, [this, box, mid](const QByteArray &data) {
        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError) {
            emit error("message parse error: " + err.errorString());
            return;
        }
        emit messageReady(box, mid, doc.object());
    });
}

void PatClient::deleteMessage(const QString &box, const QString &mid)
{
    auto *reply = del("/api/mailbox/" + box + "/" + mid);
    handleReply(reply, [this, box, mid](const QByteArray &) {
        emit messageDeleted(box, mid);
    });
}

void PatClient::markRead(const QString &box, const QString &mid)
{
    // Pat expects JSON body {"Read": true} and a content-type header — an
    // empty POST returns 400 and the read state never changes.
    QNetworkRequest req(QUrl(m_baseUrl + "/api/mailbox/" + box + "/" + mid + "/read"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QByteArray body = "{\"Read\":true}";
    auto *reply = m_nam->post(req, body);
    handleReply(reply, [](const QByteArray &) {});
}

void PatClient::postMessage(const QString &to, const QString &subject, const QString &body,
                             const QString &cc, const QStringList &attachments)
{
    auto *multi = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    auto addPart = [&](const QString &name, const QString &value) {
        QHttpPart part;
        part.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant(QString("form-data; name=\"%1\"").arg(name)));
        part.setBody(value.toUtf8());
        multi->append(part);
    };

    addPart("to", to);
    addPart("subject", subject);
    addPart("body", body);
    addPart("date", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!cc.isEmpty()) addPart("cc", cc);

    // Attachments — Pat reads file parts with field name "files"
    for (const QString &path : attachments) {
        QFile *file = new QFile(path);
        if (!file->open(QIODevice::ReadOnly)) { delete file; continue; }
        QFileInfo fi(path);
        QHttpPart filePart;
        filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
            QVariant(QString("form-data; name=\"files\"; filename=\"%1\"").arg(fi.fileName())));
        filePart.setHeader(QNetworkRequest::ContentTypeHeader,
                           QVariant("application/octet-stream"));
        filePart.setBodyDevice(file);
        file->setParent(multi);  // multi owns the file now
        multi->append(filePart);
    }

    QNetworkRequest req(QUrl(m_baseUrl + "/api/mailbox/out"));
    auto *reply = m_nam->post(req, multi);
    multi->setParent(reply);

    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            QString detail = reply->errorString();
            QByteArray body = reply->readAll();
            if (!body.isEmpty()) detail += " — " + QString::fromUtf8(body).trimmed();
            emit messagePostFailed(detail);
            emit error(detail);
            return;
        }
        emit messagePosted();
    });
}

// ── Connect ───────────────────────────────────────────────────────────────────

void PatClient::fetchRmsList(const QString &band, const QString &mode, bool forceDownload)
{
    QString path = "/api/rmslist";
    QStringList params;
    if (!band.isEmpty())  params << "band=" + band;
    if (!mode.isEmpty())  params << "mode=" + mode;
    if (forceDownload)    params << "force-download=true";
    if (!params.isEmpty()) path += "?" + params.join("&");

    auto *reply = get(path);
    handleReply(reply, [this](const QByteArray &data) {
        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError) {
            emit error("rmslist parse error: " + err.errorString());
            return;
        }
        emit rmsListReady(doc.array());
    });
}

void PatClient::connect(const QString &connectUrl)
{
    // Pat expects GET /api/connect?url=<encoded connect URL>
    // This call blocks on the server until the session is complete.
    QString path = "/api/connect?url=" + QUrl::toPercentEncoding(connectUrl);
    auto *reply = get(path);
    handleReply(reply, [this](const QByteArray &) {
        // Session finished — push a disconnected status so console closes cleanly
        QJsonObject status;
        status["connected"] = false;
        status["dialing"]   = false;
        status["remote_addr"] = QString();
        emit statusReady(status);
    });
}

void PatClient::disconnect()
{
    auto *reply = post("/api/disconnect");
    handleReply(reply, [](const QByteArray &) {});
}

void PatClient::fetchStatus()
{
    auto *reply = get("/api/status");
    handleReply(reply, [this](const QByteArray &data) {
        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError) {
            emit error("status parse error: " + err.errorString());
            return;
        }
        emit statusReady(doc.object());
    });
}

// ── Position ──────────────────────────────────────────────────────────────────

void PatClient::fetchPosition()
{
    auto *reply = get("/api/current_gps_position");
    handleReply(reply, [this](const QByteArray &data) {
        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError) {
            emit error("position parse error: " + err.errorString());
            return;
        }
        emit positionReady(doc.object());
    });
}

void PatClient::postPositionReport(const QJsonObject &pos)
{
    QByteArray body = QJsonDocument(pos).toJson(QJsonDocument::Compact);
    auto *reply = post("/api/posreport", body);
    handleReply(reply, [](const QByteArray &) {});
}

// ── Forms ─────────────────────────────────────────────────────────────────────

void PatClient::setFormInstanceCookie(const QString &key)
{
    QUrl url(m_baseUrl);
    QString host = url.host();
    if (host.isEmpty()) host = "localhost";
    QNetworkCookie cookie("forminstance", key.toUtf8());
    cookie.setDomain(host);
    cookie.setPath("/");
    m_nam->cookieJar()->setCookiesFromUrl({cookie}, url);
}

void PatClient::fetchFormData()
{
    auto *reply = get("/api/form");
    handleReply(reply, [this](const QByteArray &data) {
        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError) {
            emit error("form data parse error: " + err.errorString());
            return;
        }
        QJsonObject o = doc.object();
        emit formDataReady(
            o.value("msg_to").toString(),
            o.value("msg_cc").toString(),
            o.value("msg_subject").toString(),
            o.value("msg_body").toString()
        );
    });
}

void PatClient::fetchFormCatalog()
{
    auto *reply = get("/api/formcatalog");
    handleReply(reply, [this](const QByteArray &data) {
        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError) {
            emit error("formcatalog parse error: " + err.errorString());
            return;
        }
        emit formCatalogReady(doc.object());
    });
}

void PatClient::updateFormTemplates()
{
    auto *reply = post("/api/formsUpdate");
    handleReply(reply, [this](const QByteArray &) {
        emit formTemplatesUpdated();
    });
}

// ── WebSocket ─────────────────────────────────────────────────────────────────

void PatClient::connectWebSocket()
{
    QString wsUrl = m_baseUrl;
    wsUrl.replace("http://", "ws://").replace("https://", "wss://");
    m_ws->open(QUrl(wsUrl + "/ws"));
}

void PatClient::onWsConnected()
{
    m_wsUrl = m_baseUrl;
    m_wsUrl.replace("http://", "ws://").replace("https://", "wss://");
    m_wsUrl += "/ws";
    emit wsConnectedNow();
}

bool PatClient::isWebSocketConnected() const
{
    return m_ws && m_ws->state() == QAbstractSocket::ConnectedState;
}

void PatClient::onWsDisconnected()
{
    // Reconnect after 2 seconds
    QTimer::singleShot(2000, this, [this]() {
        if (!m_wsUrl.isEmpty())
            m_ws->open(QUrl(m_wsUrl));
    });
}

void PatClient::onWsTextMessageReceived(const QString &message)
{
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(message.toUtf8(), &err);
    if (err.error == QJsonParseError::NoError)
        emit wsEvent(doc.object());
}

void PatClient::onWsError(QAbstractSocket::SocketError)
{
    emit error("WebSocket: " + m_ws->errorString());
}

// ── Helpers ───────────────────────────────────────────────────────────────────

QNetworkReply *PatClient::get(const QString &path)
{
    QNetworkRequest req(QUrl(m_baseUrl + path));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return m_nam->get(req);
}

QNetworkReply *PatClient::post(const QString &path, const QByteArray &body)
{
    QNetworkRequest req(QUrl(m_baseUrl + path));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return m_nam->post(req, body);
}

QNetworkReply *PatClient::del(const QString &path)
{
    QNetworkRequest req(QUrl(m_baseUrl + path));
    return m_nam->deleteResource(req);
}

void PatClient::handleReply(QNetworkReply *reply, std::function<void(const QByteArray &)> handler)
{
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, handler]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }
        handler(reply->readAll());
    });
}
