//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : MissionBroadcaster — localhost UDP status emitter for automated
//           QtPatWinlink missions.
//

#include "MissionBroadcaster.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QHostAddress>

MissionBroadcaster::MissionBroadcaster(const QString &missionId, quint16 port, QObject *parent)
    : QObject(parent)
    , m_sock(new QUdpSocket(this))
    , m_heartbeat(new QTimer(this))
    , m_missionId(missionId)
    , m_port(port)
    , m_lastState(State::Idle)
{
    m_heartbeat->setInterval(10 * 1000);   // 10s while active
    m_heartbeat->setSingleShot(false);
    connect(m_heartbeat, &QTimer::timeout, this, &MissionBroadcaster::onHeartbeat);
}

QString MissionBroadcaster::stateString(State s)
{
    switch (s) {
        case State::Idle:        return "idle";
        case State::Connecting:  return "connecting";
        case State::Active:      return "active";
        case State::Complete:    return "complete";
        case State::Failed:      return "failed";
    }
    return "unknown";
}

void MissionBroadcaster::emitState(State s, const QString &detail)
{
    m_lastState  = s;
    m_lastDetail = detail;

    QJsonObject o;
    o["app"]        = "QtPatWinlink";
    o["mission_id"] = m_missionId;
    o["kind"]       = "state";
    o["ts"]         = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    o["state"]      = stateString(s);
    if (!detail.isEmpty())
        o["detail"] = detail;

    QByteArray dgram = QJsonDocument(o).toJson(QJsonDocument::Compact);
    m_sock->writeDatagram(dgram, QHostAddress::LocalHost, m_port);

    // Heartbeat only runs while we're in an active mission state — once
    // Complete or Failed lands, subscribers should react and we stop chattering.
    if (s == State::Connecting || s == State::Active) {
        if (!m_heartbeat->isActive())
            m_heartbeat->start();
    } else {
        m_heartbeat->stop();
    }
}

void MissionBroadcaster::onHeartbeat()
{
    // Re-emit the last state so a late-joining subscriber catches up.
    QJsonObject o;
    o["app"]        = "QtPatWinlink";
    o["mission_id"] = m_missionId;
    o["kind"]       = "state";
    o["ts"]         = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    o["state"]      = stateString(m_lastState);
    o["heartbeat"]  = true;
    if (!m_lastDetail.isEmpty())
        o["detail"] = m_lastDetail;

    QByteArray dgram = QJsonDocument(o).toJson(QJsonDocument::Compact);
    m_sock->writeDatagram(dgram, QHostAddress::LocalHost, m_port);
}

void MissionBroadcaster::emitLog(const QString &source, const QString &message)
{
    QJsonObject o;
    o["app"]        = "QtPatWinlink";
    o["mission_id"] = m_missionId;
    o["kind"]       = "log";
    o["ts"]         = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    o["source"]     = source;       // e.g. "pat", "modem"
    o["message"]    = message;

    QByteArray dgram = QJsonDocument(o).toJson(QJsonDocument::Compact);
    m_sock->writeDatagram(dgram, QHostAddress::LocalHost, m_port);
}
