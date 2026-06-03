//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : QtPatWinlink — native Qt replacement for Pat Winlink web UI.
//           Two startup modes:
//             - Interactive (default): full MainWindow GUI for the operator.
//             - --auto-mission: headless single-session mission driven by
//                               li-automation. Status is broadcast on UDP
//                               localhost:7456 so the daemon can follow along.
//

#include <QApplication>
#include <QCommandLineParser>
#include <QObject>
#include <QProcess>
#include <QTcpSocket>
#include <QTimer>
#include <functional>
#include <memory>
#include "MainWindow.h"
#include "PatClient.h"
#include "MissionBroadcaster.h"
#include "MissionRunner.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("QtPatWinlink");
    app.setOrganizationName("LiaisonOS");

    QCommandLineParser parser;
    parser.setApplicationDescription("Pat Winlink native Qt UI");
    parser.addHelpOption();

    QCommandLineOption touchOpt ("touch",   "Touch mode UI (on|off, default off)", "on|off", "off");
    QCommandLineOption patUrlOpt("pat-url", "Pat base URL",   "url",   "http://localhost:8080");
    QCommandLineOption bandOpt  ("band",    "Default band filter (e.g. 20m)", "band",  "");
    QCommandLineOption modemOpt ("modem",   "Default modem filter (e.g. varahf)", "modem", "");

    // ── Mail viewer (no modem, alongside presence mode) ──────────────────────
    QCommandLineOption mailViewerOpt("mail-viewer",
        "Open as a read/compose mail viewer only. Spawns its own pat http on "
        "start, kills it on close. Use while a presence mode (JS8/VarAC) is "
        "running — no VARA, no Connect view.");

    // ── Automation flags (Phase 2 of li-automation) ──────────────────────────
    QCommandLineOption autoMissionOpt("auto-mission",
        "Run a single headless Winlink mission and exit. Requires --rms.");
    QCommandLineOption connectUrlOpt ("connect-url",
        "REQUIRED with --auto-mission. The Pat connect URL passed verbatim "
        "(e.g. varahf:///VA2OPS-10?freq=14105&bw=2300). Operator owns it.",
        "url", "");
    QCommandLineOption rmsOpt        ("rms",
        "Informational target callsign for the broadcast/log (optional). "
        "Pat parses the actual target from --connect-url.", "callsign", "");
    QCommandLineOption timeoutMinOpt ("timeout-min",
        "Auto-mission hard timeout in minutes (default 10)", "min", "10");
    QCommandLineOption missionIdOpt  ("mission-id",
        "Auto-mission label for status broadcast (default \"unnamed\")", "id", "unnamed");

    parser.addOption(touchOpt);
    parser.addOption(patUrlOpt);
    parser.addOption(bandOpt);
    parser.addOption(modemOpt);
    parser.addOption(mailViewerOpt);
    parser.addOption(autoMissionOpt);
    parser.addOption(connectUrlOpt);
    parser.addOption(rmsOpt);
    parser.addOption(timeoutMinOpt);
    parser.addOption(missionIdOpt);
    parser.process(app);

    QString touchRaw = parser.value(touchOpt).trimmed().toLower();
    bool    touchMode = (touchRaw == "on" || touchRaw == "true" || touchRaw == "1");
    QString patUrl    = parser.value(patUrlOpt);
    QString band      = parser.value(bandOpt);
    QString modem     = parser.value(modemOpt);

    // ── Mail viewer path ─────────────────────────────────────────────────────
    // Spawns its own pat http (so the operator can read mail even while a
    // presence mode like JS8/VarAC is running). Polls port 8080 event-driven
    // until pat is accepting, then opens MainWindow with the Connect view
    // disabled. On quit, kills pat http.
    if (parser.isSet(mailViewerOpt)) {
        auto *patProc = new QProcess(&app);
        patProc->setProgram("pat");
        patProc->setArguments({"http"});
        patProc->setProcessChannelMode(QProcess::ForwardedChannels);
        patProc->start();

        QObject::connect(&app, &QCoreApplication::aboutToQuit, [patProc]() {
            if (patProc->state() != QProcess::NotRunning) {
                patProc->terminate();
                if (!patProc->waitForFinished(3000))
                    patProc->kill();
            }
        });

        // Event-driven port-ready probe. Try connecting to :8080; on success,
        // open MainWindow. On failure, retry in 200ms. No blocking, no sleep.
        auto openMainWindow = [patUrl, touchMode, band, modem]() {
            auto *w = new MainWindow(patUrl, touchMode, band, modem, /*mailViewer=*/true);
            if (touchMode) w->showMaximized();
            else           w->show();
        };
        auto probe = std::make_shared<std::function<void()>>();
        *probe = [probe, openMainWindow, &app]() {
            auto *s = new QTcpSocket(&app);
            QObject::connect(s, &QTcpSocket::connected, &app,
                             [s, openMainWindow]() {
                s->disconnectFromHost();
                s->deleteLater();
                openMainWindow();
            });
            QObject::connect(s, &QTcpSocket::errorOccurred, &app,
                             [s, probe](QAbstractSocket::SocketError) {
                s->deleteLater();
                QTimer::singleShot(200, [probe]() { (*probe)(); });
            });
            s->connectToHost("127.0.0.1", 8080);
        };
        (*probe)();

        return app.exec();
    }

    // ── Auto-mission path ────────────────────────────────────────────────────
    if (parser.isSet(autoMissionOpt)) {
        QString connectUrl = parser.value(connectUrlOpt).trimmed();
        if (connectUrl.isEmpty()) {
            qCritical("--auto-mission requires --connect-url <url>");
            return 2;
        }

        MissionRunner::Params p;
        p.missionId  = parser.value(missionIdOpt);
        p.rms        = parser.value(rmsOpt).trimmed();   // informational
        p.connectUrl = connectUrl;
        p.timeoutMin = parser.value(timeoutMinOpt).toInt();
        if (p.timeoutMin <= 0) p.timeoutMin = 10;

        auto *pat    = new PatClient(patUrl);
        auto *bcast  = new MissionBroadcaster(p.missionId);
        auto *runner = new MissionRunner(pat, bcast, p);

        // Pop a live session console showing this mission's broadcast.
        // The mission_id can contain spaces (e.g. "Morning Winlink"), so
        // it MUST be single-quoted in the -e shell command — otherwise the
        // wrapping /bin/bash -c splits on whitespace and li-listen sees
        // extra positional args, errors out, and QtTerminal closes.
        QProcess::startDetached("QtTerminal", {
            "--title", QString("Mission: %1").arg(p.missionId),
            "-e", QString("python3 /opt/liaisonos/bin/li-listen --mission '%1'")
                      .arg(QString(p.missionId).replace("'", "'\\''"))
        });

        // Always exit 0 in auto-mission mode regardless of mission outcome.
        // The real result is carried by the UDP broadcast (complete | failed),
        // not by the process exit code. A non-zero exit would make the
        // et-supervisor label the chain step "CRASHED" and trigger noisy
        // teardown logging even on a clean failed-mission path.
        QObject::connect(runner, &MissionRunner::finished,
                         &app,   [&app](int /*code*/) { app.exit(0); });

        // Pat needs the WS for richer status events (some setups).
        pat->connectWebSocket();

        runner->start();
        return app.exec();
    }

    // ── Interactive path (existing behaviour) ────────────────────────────────
    MainWindow w(patUrl, touchMode, band, modem);
    if (touchMode) w.showMaximized();
    else           w.show();

    return app.exec();
}
