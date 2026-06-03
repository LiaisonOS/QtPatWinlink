//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : MissionRunner — drives one headless Winlink session.
//           The connect URL is provided verbatim by the operator (in the
//           schedule / CLI). MissionRunner does NOT construct or transform
//           it — it hands the URL straight to PatClient::connect, exactly
//           like the dashboard's Connect view does after the user picks a
//           row from Pat's rmslist. Operator owns the URL (GIGO).
//

#pragma once

#include <QObject>
#include <QJsonObject>
#include <QTimer>
#include <QString>

class PatClient;
class MissionBroadcaster;

class MissionRunner : public QObject
{
    Q_OBJECT

public:
    struct Params {
        QString missionId;       // human label, also broadcast field
        QString rms;             // informational only, shown in broadcast
        QString connectUrl;      // REQUIRED. Passed verbatim to Pat.
        int     timeoutMin = 10; // hard cap on the whole session
    };

    MissionRunner(PatClient *pat,
                  MissionBroadcaster *bcast,
                  const Params &p,
                  QObject *parent = nullptr);

    void start();

signals:
    void finished(int exitCode);   // main.cpp connects → app.exit(0)

private slots:
    void onStatusReady(const QJsonObject &status);
    void onWsEvent(const QJsonObject &event);
    void onPatError(const QString &msg);
    void onTimeout();

private:
    void failMission(const QString &reason);
    void completeMission();

    PatClient          *m_pat;
    MissionBroadcaster *m_bcast;
    Params              m_p;

    QTimer             *m_timeout;
    bool                m_sawConnected = false;
    bool                m_done = false;
    bool                m_receivedMail = false;
};
