//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : QtPatWinlink — native Qt replacement for Pat Winlink web UI
//

#include <QApplication>
#include <QCommandLineParser>
#include "MainWindow.h"

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

    parser.addOption(touchOpt);
    parser.addOption(patUrlOpt);
    parser.addOption(bandOpt);
    parser.addOption(modemOpt);
    parser.process(app);

    QString touchRaw = parser.value(touchOpt).trimmed().toLower();
    bool    touchMode = (touchRaw == "on" || touchRaw == "true" || touchRaw == "1");
    QString patUrl    = parser.value(patUrlOpt);
    QString band      = parser.value(bandOpt);
    QString modem     = parser.value(modemOpt);

    MainWindow w(patUrl, touchMode, band, modem);
    if (touchMode) w.showMaximized();
    else           w.show();

    return app.exec();
}
