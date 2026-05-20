//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Position Report dialog — implementation.
//

#include "PositionReportDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QDateTime>
#include <QJsonObject>
#include <QApplication>
#include <QScreen>
#include <QRegularExpression>

// Parse a lat/lon value as decimal degrees OR Pat's DDM-with-hemisphere format
// (e.g. "45-29.6527N", "073-33.316W"). Returns 0 with ok=false if unparseable.
static double parseLatLon(const QString &s, bool *ok)
{
    QString trimmed = s.trimmed();

    // Try plain decimal first
    bool dOk = false;
    double d = trimmed.toDouble(&dOk);
    if (dOk) { if (ok) *ok = true; return d; }

    // Try DDM: "DDD-MM.MMMM[NSEW]"
    static const QRegularExpression re(
        "^(\\d+)-(\\d+(?:\\.\\d+)?)([NSEWnsew])$");
    auto match = re.match(trimmed);
    if (!match.hasMatch()) { if (ok) *ok = false; return 0.0; }

    double deg = match.captured(1).toDouble();
    double min = match.captured(2).toDouble();
    QChar  hem = match.captured(3).toUpper()[0];
    double dec = deg + min / 60.0;
    if (hem == 'S' || hem == 'W') dec = -dec;
    if (ok) *ok = true;
    return dec;
}

PositionReportDialog::PositionReportDialog(PatClient *client, bool touchMode, QWidget *parent)
    : QDialog(parent)
    , m_client(client)
    , m_touchMode(touchMode)
{
    setWindowTitle("Send Position Report");
    setMinimumSize(560, 380);

    int fontSize = touchMode ? 13 : 10;
    int fieldMinH = touchMode ? 44 : 0;
    int btnRadius = touchMode ? 6 : 4;

    QString sheet = QString(
        "QDialog { background: #121212; }"
        "QLabel  { color: #e0e0e0; font-size: %1pt; }"
        "QLineEdit, QTextEdit {"
        "  background: #1e1e1e; color: #e0e0e0;"
        "  border: 1px solid #333333; border-radius: %2px;"
        "  padding: 6px; font-size: %1pt;"
        "}"
        "QLineEdit { min-height: %3px; }"
    ).arg(fontSize).arg(btnRadius).arg(fieldMinH);
    setStyleSheet(sheet);

    m_latField    = new QLineEdit();
    m_lonField    = new QLineEdit();
    m_commentField = new QTextEdit();
    m_latField->setPlaceholderText("e.g. 45.5017");
    m_lonField->setPlaceholderText("e.g. -73.5673");
    m_commentField->setPlaceholderText("Optional comment (location, status, etc.)");
    m_commentField->setMinimumHeight(touchMode ? 110 : 70);

    m_gpsBtn    = new QPushButton("📡 Get from GPS");
    m_sendBtn   = new QPushButton("Send to Outbox");
    m_cancelBtn = new QPushButton("Cancel");

    QString neutralBtn = QString(
        "QPushButton { background: #333333; color: #ffffff; border: none;"
        "  border-radius: %1px; %2 }"
        "QPushButton:hover { background: #444444; }"
    ).arg(btnRadius)
     .arg(touchMode ? "font-size: 16px; min-height: 58px; padding: 0 22px;"
                    : QString("font-size: %1pt; padding: 6px 16px;").arg(fontSize));

    QString primaryBtn = QString(
        "QPushButton { background: #ffa500; color: #000000; font-weight: bold; border: none;"
        "  border-radius: %1px; %2 }"
        "QPushButton:hover { background: #ffb733; }"
    ).arg(btnRadius)
     .arg(touchMode ? "font-size: 18px; min-height: 58px; padding: 0 26px;"
                    : QString("font-size: %1pt; padding: 6px 18px;").arg(fontSize));

    m_gpsBtn->setStyleSheet(neutralBtn);
    m_cancelBtn->setStyleSheet(neutralBtn);
    m_sendBtn->setStyleSheet(primaryBtn);

    auto *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->setSpacing(10);

    auto *latRow = new QHBoxLayout();
    latRow->addWidget(m_latField, 1);
    auto *lonRow = new QHBoxLayout();
    lonRow->addWidget(m_lonField, 1);
    form->addRow("Lat (°):", latRow);
    form->addRow("Lon (°):", lonRow);
    form->addRow(new QLabel("Comment:"), m_commentField);

    auto *gpsRow = new QHBoxLayout();
    gpsRow->addWidget(m_gpsBtn);
    gpsRow->addStretch();

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_sendBtn);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(12);
    layout->addLayout(form);
    layout->addLayout(gpsRow);
    layout->addStretch();
    layout->addLayout(btnRow);

    QObject::connect(m_gpsBtn,    &QPushButton::clicked, this, &PositionReportDialog::onGetFromGps);
    QObject::connect(m_sendBtn,   &QPushButton::clicked, this, &PositionReportDialog::onSend);
    QObject::connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    QObject::connect(m_client, &PatClient::positionReady,
                     this, &PositionReportDialog::onPositionReady);

    if (touchMode) {
        QWidget *parentWin = parentWidget() ? parentWidget()->window() : nullptr;
        QRect bounds = parentWin ? parentWin->geometry()
                                 : QApplication::primaryScreen()->availableGeometry();
        const int margin = 48;
        setGeometry(bounds.adjusted(margin, margin, -margin, -margin));
    }
}

void PositionReportDialog::onGetFromGps()
{
    m_gpsBtn->setEnabled(false);
    m_gpsBtn->setText("📡 Asking GPS...");
    m_client->fetchPosition();
}

void PositionReportDialog::onPositionReady(const QJsonObject &pos)
{
    m_gpsBtn->setEnabled(true);
    m_gpsBtn->setText("📡 Get from GPS");

    // Pat may return lat/lon as decimal numbers OR as DDM strings like "45-29.6527N".
    // Try common key names and parse either format.
    auto pickValue = [&](const QStringList &keys, bool *ok) -> double {
        for (const QString &k : keys) {
            auto v = pos.value(k);
            if (v.isDouble()) { if (ok) *ok = true; return v.toDouble(); }
            if (v.isString()) return parseLatLon(v.toString(), ok);
        }
        if (ok) *ok = false;
        return 0.0;
    };

    bool latOk = false, lonOk = false;
    double lat = pickValue({"lat", "Lat", "Latitude", "latitude"}, &latOk);
    double lon = pickValue({"lon", "Lon", "Longitude", "longitude", "lng", "Lng"}, &lonOk);

    if (!latOk || !lonOk) {
        QMessageBox::warning(this, "No GPS fix",
            "GPS position is not available right now. Enter coordinates manually.");
        return;
    }

    m_latField->setText(QString::number(lat, 'f', 6));
    m_lonField->setText(QString::number(lon, 'f', 6));
}

void PositionReportDialog::onSend()
{
    bool latOk = false, lonOk = false;
    double lat = m_latField->text().trimmed().toDouble(&latOk);
    double lon = m_lonField->text().trimmed().toDouble(&lonOk);

    if (!latOk || lat < -90.0 || lat > 90.0) {
        QMessageBox::warning(this, "Invalid latitude",
            "Latitude must be a decimal number between -90 and 90.");
        return;
    }
    if (!lonOk || lon < -180.0 || lon > 180.0) {
        QMessageBox::warning(this, "Invalid longitude",
            "Longitude must be a decimal number between -180 and 180.");
        return;
    }

    QJsonObject pos;
    pos["Lat"]     = lat;
    pos["Lon"]     = lon;
    pos["Comment"] = m_commentField->toPlainText().trimmed();
    pos["Date"]    = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    m_client->postPositionReport(pos);
    emit sent();
    accept();
}
