//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : August 2026
// Purpose : P2P Listen mode config dialog — implementation.
//

#include "P2PListenDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QTimer>

// Map a QtPatWinlink --modem CLI value to (canonical config key, display label).
// Fall back to raw value if unknown so we still work for future modems.
static void modemLabelFor(const QString &raw, QString *canonical, QString *label)
{
    const QString low = raw.trimmed().toLower();
    struct Row { const char *raw; const char *canon; const char *label; };
    static const Row rows[] = {
        {"varahf",  "varahf",  "VARA HF"},
        {"varafm",  "varafm",  "VARA FM"},
        {"ardop",   "ardop",   "ARDOP"},
        {"packet",  "ax25",    "AX.25 / Packet"},
        {"ax25",    "ax25",    "AX.25 / Packet"},
        {"pactor",  "pactor",  "PACTOR"},
    };
    for (const auto &r : rows) {
        if (low == r.raw) {
            *canonical = r.canon;
            *label     = r.label;
            return;
        }
    }
    *canonical = low;
    *label     = raw;
}

P2PListenDialog::P2PListenDialog(PatClient *client, bool touchMode,
                                 const QString &activeModem, QWidget *parent)
    : QDialog(parent)
    , m_client(client)
    , m_touchMode(touchMode)
{
    modemLabelFor(activeModem, &m_modem, &m_modemLabel);

    setWindowTitle(tr("P2P Listen — %1").arg(m_modemLabel));
    setModal(true);
    setMinimumWidth(touchMode ? 640 : 460);

    const int fontSize = touchMode ? 13 : 10;
    setStyleSheet(QString(
        "QDialog   { background: #1a1a1a; color: #e0e0e0; }"
        "QLabel    { color: #e0e0e0; font-size: %1pt; }"
        "QCheckBox { color: #e0e0e0; font-size: %1pt; padding: 6px 0; }"
        "QCheckBox::indicator { width: 22px; height: 22px; }"
    ).arg(fontSize));

    auto *hint = new QLabel(tr(
        "When enabled, PAT auto-accepts incoming P2P (peer-to-peer) ARQ "
        "connections on %1. Peers dialing your callsign exchange "
        "P2P-flagged messages with you directly — no CMS involved. "
        "Changes take effect immediately (no PAT restart).").arg(m_modemLabel), this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color: #999; font-size: 10pt;");

    m_toggle = new QCheckBox(
        tr("Enable P2P listening on %1").arg(m_modemLabel), this);
    m_toggle->setEnabled(false);   // enable after we've fetched current state

    m_status = new QLabel(tr("Loading current PAT configuration…"), this);
    m_status->setStyleSheet("color: #888; font-size: 10pt; font-style: italic;");

    m_saveBtn   = new QPushButton(tr("Save && Reload PAT"), this);
    m_cancelBtn = new QPushButton(tr("Cancel"), this);
    m_saveBtn->setEnabled(false);
    const int btnRadius = touchMode ? 6 : 4;
    const QString actionExtra = touchMode
        ? QString("font-size: 18px; font-weight: bold; min-height: 58px; padding: 0 26px;")
        : QString("padding: 6px 20px;");
    m_saveBtn->setStyleSheet(QString(
        "QPushButton { background: #ffa500; color: #000000; font-weight: bold;"
        "              border: none; border-radius: %1px; %2 }"
        "QPushButton:disabled { background: #444444; color: #888888; }"
        "QPushButton:hover:enabled { background: #ffb733; }"
    ).arg(btnRadius).arg(actionExtra));
    m_cancelBtn->setStyleSheet(QString(
        "QPushButton { background: #333333; color: white; border: none;"
        "              border-radius: %1px; %2 }"
        "QPushButton:hover { background: #444444; }"
    ).arg(btnRadius).arg(actionExtra));

    auto *btnRow = new QHBoxLayout();
    btnRow->addWidget(m_status, 1);
    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_saveBtn);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(14);
    layout->addWidget(hint);
    layout->addWidget(m_toggle);
    layout->addLayout(btnRow);

    QObject::connect(m_client, &PatClient::configReady,    this, &P2PListenDialog::onConfigReady);
    QObject::connect(m_client, &PatClient::error,          this, &P2PListenDialog::onPatError);
    QObject::connect(m_client, &PatClient::configReloaded, this, [this]() {
        m_status->setText(tr("Saved and reloaded — PAT %1 %2.")
                              .arg(m_modemLabel)
                              .arg(m_toggle->isChecked() ? tr("is now listening")
                                                         : tr("is NOT listening")));
        QTimer::singleShot(1200, this, &QDialog::accept);
    });
    QObject::connect(m_saveBtn,   &QPushButton::clicked, this, &P2PListenDialog::onSaveClicked);
    QObject::connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    m_client->fetchConfig();
}

void P2PListenDialog::onConfigReady(const QJsonObject &config)
{
    const QJsonArray listen = config.value("listen").toArray();
    bool present = false;
    for (const auto &v : listen) {
        if (v.toString().toLower() == m_modem) { present = true; break; }
    }
    m_toggle->setChecked(present);
    m_toggle->setEnabled(true);
    m_saveBtn->setEnabled(true);
    m_status->setText(present
        ? tr("Currently listening on %1.").arg(m_modemLabel)
        : tr("Currently NOT listening on %1.").arg(m_modemLabel));
}

void P2PListenDialog::onSaveClicked()
{
    // We need to preserve any OTHER modems in the listen array — the
    // operator may have enabled a modem from a different QtPatWinlink
    // session (e.g. VARA FM alongside VARA HF). Fetch current, splice
    // just our modem in/out, PUT back.
    m_saveBtn->setEnabled(false);
    m_cancelBtn->setEnabled(false);
    m_status->setText(tr("Saving to PAT and reloading…"));

    const bool wantOn = m_toggle->isChecked();
    // Ask PatClient for the current config, then build a new listen array
    // and push it. PatClient's updateConfigListen currently REPLACES the
    // whole listen array — for this dialog we want add/remove semantics,
    // so we fetch → mutate → PUT ourselves.
    m_client->fetchConfig();
    // One-shot handler for the fetch we just triggered — reads listen,
    // splices our modem in/out, calls updateConfigListen with the new
    // list. The connection is queued to fire only ONCE.
    auto *conn = new QObject(this);
    QObject::connect(m_client, &PatClient::configReady, conn,
        [this, wantOn, conn](const QJsonObject &cfg) {
            conn->deleteLater();  // disconnect this one-shot handler
            QJsonArray current = cfg.value("listen").toArray();
            QStringList modems;
            for (const auto &v : current) {
                const QString m = v.toString().toLower();
                if (!m.isEmpty() && m != m_modem) modems << m;
            }
            if (wantOn) modems << m_modem;
            m_client->updateConfigListen(modems);
        });
}

void P2PListenDialog::onPatError(const QString &msg)
{
    // Suppress WebSocket blips. POST /api/reload briefly cycles Pat's
    // HTTP server which drops our WebSocket connection — PatClient emits
    // an error, then auto-reconnects 2s later. Not related to the save.
    if (msg.startsWith("WebSocket", Qt::CaseInsensitive))
        return;
    m_status->setText(tr("PAT error: %1").arg(msg));
    m_status->setStyleSheet("color: #ff8080; font-size: 10pt; font-weight: bold;");
    m_saveBtn->setEnabled(true);
    m_cancelBtn->setEnabled(true);
}
