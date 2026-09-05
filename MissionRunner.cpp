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
    , m_grace(new QTimer(this))
{
    m_timeout->setSingleShot(true);
    m_timeout->setInterval(m_p.timeoutMin * 60 * 1000);
    connect(m_timeout, &QTimer::timeout, this, &MissionRunner::onTimeout);

    // Backstop: once the link goes down we normally complete on the
    // /api/connect long-poll return. If that HTTP reply is ever lost, this
    // fires a few seconds later so the mission finalizes instead of hanging
    // to the hard timeout. By then any received mail is long since on disk.
    m_grace->setSingleShot(true);
    m_grace->setInterval(8 * 1000);
    connect(m_grace, &QTimer::timeout, this, &MissionRunner::onGrace);

    connect(m_pat, &PatClient::statusReady,     this, &MissionRunner::onStatusReady);
    connect(m_pat, &PatClient::wsEvent,         this, &MissionRunner::onWsEvent);
    connect(m_pat, &PatClient::connectFinished, this, &MissionRunner::onConnectFinished);
    connect(m_pat, &PatClient::error,           this, &MissionRunner::onPatError);
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

    // connected=false && dialing=false after we'd gone active: the ARQ link
    // has torn down. We do NOT complete off this WS status directly — it can
    // fire while Pat is still flushing the last received-mail chunks to disk.
    // The authoritative "session done, mail on disk" signal is the
    // /api/connect long-poll returning (onConnectFinished), which normally
    // lands moments after this. Arm a short grace timer as a backstop so the
    // mission still finalizes if that HTTP reply is ever lost.
    if (m_sawConnected && !m_graceArmed) {
        m_graceArmed = true;
        m_grace->start();
        qInfo().noquote() << "[mission] link down — awaiting /api/connect return "
                             "(grace backstop armed)";
    }
}

void MissionRunner::onConnectFinished(const QString &error)
{
    if (m_done) return;
    // Pat's /api/connect long-poll has returned — its session goroutine is
    // fully finished and every received message is written to disk. This is
    // the authoritative completion signal. `error` is non-empty for Pat's
    // benign clean-termination HTTP 500 (or a real transport failure); once
    // the link went active it's session-end either way. If we never saw the
    // link go active, the session never really started → failure.
    if (m_sawConnected) {
        if (!error.isEmpty())
            qInfo().noquote() << "[mission] /api/connect returned" << error
                              << "after active session — treating as complete";
        completeMission();
    } else {
        failMission(error.isEmpty()
            ? QString("session ended before the link went active")
            : error);
    }
}

void MissionRunner::onPatError(const QString &msg)
{
    if (m_done) return;
    // Non-fatal. Transient PatClient errors — WS blips that auto-reconnect, or
    // Pat's benign clean-termination HTTP 500 (also surfaced here) — must not
    // kill a mission. The authoritative end/failure comes from
    // onConnectFinished; a genuinely hung session is caught by the hard
    // timeout. Just log and broadcast for operator visibility.
    qInfo().noquote() << "[mission] pat error (non-fatal):" << msg;
    m_bcast->emitLog("pat-error", msg);
}

void MissionRunner::onTimeout()
{
    if (m_done) return;
    failMission(QString("timed out after %1 min").arg(m_p.timeoutMin));
    if (m_pat) m_pat->disconnect();
}

void MissionRunner::onGrace()
{
    if (m_done) return;
    // Link went down but the /api/connect reply never arrived. Received mail
    // is on disk well within this grace window, so finalize as complete.
    qInfo().noquote() << "[mission] grace elapsed after link-down "
                         "(no /api/connect return) — completing";
    completeMission();
}

void MissionRunner::completeMission()
{
    m_done = true;
    m_timeout->stop();
    m_grace->stop();
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
    m_grace->stop();
    m_bcast->emitState(MissionBroadcaster::State::Failed, reason);
    qWarning().noquote() << "[mission] failed:" << reason;
    emit finished(1);
}
