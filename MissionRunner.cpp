//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : MissionRunner — drives one headless Winlink session using
//           the operator-provided connect URL verbatim. No URL building,
//           no rmslist lookup — identical to the dashboard flow after the
//           operator clicks a row in the Connect view.
//

#include "MissionRunner.h"
#include "PatClient.h"
#include "MissionBroadcaster.h"

#include <QDebug>

MissionRunner::MissionRunner(PatClient *pat,
                             MissionBroadcaster *bcast,
                             const Params &p,
                             QObject *parent)
    : QObject(parent)
    , m_pat(pat)
    , m_bcast(bcast)
    , m_p(p)
    , m_timeout(new QTimer(this))
{
    m_timeout->setSingleShot(true);
    m_timeout->setInterval(m_p.timeoutMin * 60 * 1000);
    connect(m_timeout, &QTimer::timeout, this, &MissionRunner::onTimeout);

    connect(m_pat, &PatClient::statusReady, this, &MissionRunner::onStatusReady);
    connect(m_pat, &PatClient::wsEvent,     this, &MissionRunner::onWsEvent);
    connect(m_pat, &PatClient::error,       this, &MissionRunner::onPatError);
}

void MissionRunner::start()
{
    if (m_p.connectUrl.isEmpty()) {
        failMission("connect_url is empty — operator must provide it");
        return;
    }

    QString detail = m_p.rms.isEmpty()
        ? m_p.connectUrl
        : QString("%1 (%2)").arg(m_p.rms, m_p.connectUrl);

    qInfo().noquote() << "[mission] starting:" << detail;

    m_bcast->emitState(MissionBroadcaster::State::Connecting, detail);
    m_timeout->start();
    m_pat->connect(m_p.connectUrl);
}

void MissionRunner::onWsEvent(const QJsonObject &event)
{
    // Pat's WS event channel — same one SessionConsole consumes in dashboard.
    // We broadcast each LogLine + Notification as a "log" UDP datagram so
    // li-listen (and any operator console wrapping it) shows the full
    // protocol exchange in real time.
    if (event.contains("LogLine")) {
        QString line = event.value("LogLine").toString().trimmed();
        if (!line.isEmpty()) {
            qInfo().noquote() << "[pat]" << line;
            m_bcast->emitLog("pat", line);
        }
    }
    if (event.contains("Notification")) {
        QJsonObject n = event.value("Notification").toObject();
        QString msg = n.value("title").toString() + " — " + n.value("body").toString();
        qInfo().noquote() << "[pat-notify]" << msg;
        m_bcast->emitLog("pat-notify", msg);
    }
    // Track inbound mail. Pat sends Progress events with receiving=true
    // while pulling messages from the RMS.
    if (event.contains("Progress")) {
        QJsonObject p = event.value("Progress").toObject();
        if (p.value("receiving").toBool(false))
            m_receivedMail = true;
    }

    // Status events come over WS too (not only via PatClient::statusReady).
    // Route them through the same handler so the mission actually closes
    // when Pat reports disconnect (no more "connecting" heartbeats after QRT).
    if (event.contains("Status")) {
        onStatusReady(event.value("Status").toObject());
    }
}

void MissionRunner::onStatusReady(const QJsonObject &status)
{
    if (m_done) return;

    bool connected = status.value("connected").toBool(false);
    bool dialing   = status.value("dialing").toBool(false);
    QString remote = status.value("remote_addr").toString();

    if (connected) {
        if (!m_sawConnected) {
            m_sawConnected = true;
            QString lbl = remote.isEmpty()
                ? (m_p.rms.isEmpty() ? QString("session") : m_p.rms)
                : remote;
            m_bcast->emitState(MissionBroadcaster::State::Active,
                               QString("connected to %1").arg(lbl));
            qInfo().noquote() << "[mission] connected to" << lbl;
        }
        return;
    }

    if (dialing) {
        return;  // still trying — stay in Connecting
    }

    // connected=false && dialing=false. If we previously saw connected=true,
    // the session ended cleanly. Otherwise this is the pre-connect state.
    if (m_sawConnected)
        completeMission();
}

void MissionRunner::onPatError(const QString &msg)
{
    if (m_done) return;
    // Pat returns HTTP 500 on the long-poll /api/connect even on clean
    // session termination. If we already saw the session go active
    // (connected=true via WS Status), this is a benign session-end signal
    // — not a real failure. Treat it as Complete.
    if (m_sawConnected) {
        qInfo().noquote() << "[mission] Pat returned" << msg
                          << "after active session — treating as complete";
        completeMission();
        return;
    }
    failMission(msg);
}

void MissionRunner::onTimeout()
{
    if (m_done) return;
    failMission(QString("timed out after %1 min").arg(m_p.timeoutMin));
    if (m_pat) m_pat->disconnect();
}

void MissionRunner::completeMission()
{
    m_done = true;
    m_timeout->stop();
    QString lbl = m_p.rms.isEmpty() ? QString("session") : m_p.rms;
    QString detail = QString("session ended (%1)").arg(lbl);
    if (m_receivedMail) detail += " — new mail";
    m_bcast->emitState(MissionBroadcaster::State::Complete, detail);
    qInfo().noquote() << "[mission] complete"
                      << (m_receivedMail ? "(new mail)" : "(no new mail)");
    emit finished(0);
}

void MissionRunner::failMission(const QString &reason)
{
    m_done = true;
    m_timeout->stop();
    m_bcast->emitState(MissionBroadcaster::State::Failed, reason);
    qWarning().noquote() << "[mission] failed:" << reason;
    emit finished(1);
}
