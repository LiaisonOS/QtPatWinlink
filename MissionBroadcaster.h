//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : MissionBroadcaster — emits the current QtPatWinlink mission
//           state as a JSON datagram on localhost UDP 7456, so li-automation
//           (or any other listener) can follow a scheduled session without
//           polling Pat's HTTP API.
//

#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QString>
#include <QTimer>

class MissionBroadcaster : public QObject
{
    Q_OBJECT

public:
    // Mission state vocabulary. Keep these stable — li-automation matches on them.
    enum class State {
        Idle,
        Connecting,
        Active,
        Complete,
        Failed
    };

    explicit MissionBroadcaster(const QString &missionId,
                                quint16 port = 7456,
                                QObject *parent = nullptr);

    // Emit one datagram immediately. Re-emits also fire every 10s while the
    // mission is active (heartbeat) so a subscriber that joins late still
    // sees the current state.
    void emitState(State s, const QString &detail = QString());

    // Emit a log line — distinct datagram kind ("log") for protocol
    // exchanges (Pat WS LogLine, modem chatter, etc.). No heartbeat: log
    // events are fire-and-forget.
    void emitLog(const QString &source, const QString &message);

private slots:
    void onHeartbeat();

private:
    static QString stateString(State s);

    QUdpSocket *m_sock;
    QTimer     *m_heartbeat;
    QString     m_missionId;
    quint16     m_port;

    State       m_lastState;
    QString     m_lastDetail;
};
