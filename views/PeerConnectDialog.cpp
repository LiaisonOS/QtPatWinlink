//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : August 2026
// Purpose : Connect-to-Peer dialog — implementation.
//

#include "PeerConnectDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>

// Map the --modem CLI value to (URL scheme, pretty display label).
static void schemeLabelFor(const QString &raw, QString *scheme, QString *label)
{
    const QString low = raw.trimmed().toLower();
    struct Row { const char *raw; const char *scheme; const char *label; };
    static const Row rows[] = {
        {"varahf", "varahf", "VARA HF"},
        {"varafm", "varafm", "VARA FM"},
        {"ardop",  "ardop",  "ARDOP"},
        {"packet", "ax25",   "AX.25 / Packet"},
        {"ax25",   "ax25",   "AX.25 / Packet"},
    };
    for (const auto &r : rows) {
        if (low == r.raw) {
            *scheme = r.scheme;
            *label  = r.label;
            return;
        }
    }
    // Unknown modem — fall back to raw for scheme, capitalized for label.
    *scheme = low;
    *label  = raw.isEmpty() ? QStringLiteral("(unknown modem)") : raw;
}

PeerConnectDialog::PeerConnectDialog(const QString &activeModem, bool touchMode,
                                     QWidget *parent)
    : QDialog(parent)
    , m_touchMode(touchMode)
{
    schemeLabelFor(activeModem, &m_scheme, &m_modemLabel);

    setWindowTitle(tr("Connect to Peer — %1").arg(m_modemLabel));
    setModal(true);
    setMinimumWidth(touchMode ? 640 : 460);

    const int fontSize = touchMode ? 13 : 10;
    setStyleSheet(QString(
        "QDialog { background: #1a1a1a; color: #e0e0e0; }"
        "QLabel  { color: #e0e0e0; font-size: %1pt; }"
        "QLineEdit {"
        "  background: #1e1e1e; color: #e0e0e0; border: 1px solid #333333;"
        "  border-radius: 4px; padding: 4px 8px; font-size: %1pt; }"
    ).arg(fontSize));

    m_callsign = new QLineEdit(this);
    m_callsign->setPlaceholderText("KJ4ABC");
    if (touchMode) m_callsign->setMinimumHeight(48);

    auto *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->setSpacing(10);
    form->addRow(tr("Peer callsign*:"), m_callsign);

    auto *hint = new QLabel(tr(
        "Initiates a direct ARQ session over %1 to the peer. Tune the "
        "radio to the frequency you agreed on with the peer BEFORE clicking "
        "Connect — QtPatWinlink does not QSY for peer sessions (P2P is "
        "a coordinated activity). The peer must be running Pat / "
        "QtPatWinlink in Listen mode on the same modem and frequency.")
        .arg(m_modemLabel), this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color: #999; font-size: 9pt;");

    const int btnRadius = touchMode ? 6 : 4;
    const QString actionExtra = touchMode
        ? QString("font-size: 18px; font-weight: bold; min-height: 58px; padding: 0 26px;")
        : QString("padding: 6px 20px;");
    auto *connectBtn = new QPushButton(tr("Connect"), this);
    auto *cancelBtn  = new QPushButton(tr("Cancel"), this);
    connectBtn->setStyleSheet(QString(
        "QPushButton { background: #ffa500; color: #000000; font-weight: bold;"
        "              border: none; border-radius: %1px; %2 }"
        "QPushButton:hover { background: #ffb733; }"
    ).arg(btnRadius).arg(actionExtra));
    cancelBtn->setStyleSheet(QString(
        "QPushButton { background: #333333; color: white; border: none;"
        "              border-radius: %1px; %2 }"
        "QPushButton:hover { background: #444444; }"
    ).arg(btnRadius).arg(actionExtra));

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(connectBtn);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(12);
    layout->addLayout(form);
    layout->addWidget(hint);
    layout->addLayout(btnRow);

    QObject::connect(connectBtn, &QPushButton::clicked, this, &PeerConnectDialog::onAccept);
    QObject::connect(cancelBtn,  &QPushButton::clicked, this, &QDialog::reject);
}

void PeerConnectDialog::onAccept()
{
    QString cs = m_callsign->text().trimmed().toUpper();
    if (cs.isEmpty()) {
        QMessageBox::warning(this, tr("Missing field"),
                             tr("Peer callsign is required."));
        m_callsign->setFocus();
        return;
    }
    static const QRegularExpression rx("^[A-Z0-9]{3,7}(-\\d{1,2})?$");
    if (!rx.match(cs).hasMatch()) {
        auto ans = QMessageBox::question(this, tr("Callsign format"),
            tr("\"%1\" doesn't look like a typical callsign. Connect anyway?").arg(cs));
        if (ans != QMessageBox::Yes) { m_callsign->setFocus(); return; }
    }
    if (m_scheme.isEmpty()) {
        QMessageBox::warning(this, tr("No modem"),
            tr("QtPatWinlink doesn't know which modem to use for this session. "
               "Restart via a mode chain that specifies --modem."));
        return;
    }

    // No freq/bw params — operator tunes their rig manually. PAT never
    // sees a ?freq= so hamlib QSY is never attempted, no CAT conflicts.
    m_result = QString("%1:///%2").arg(m_scheme).arg(cs);
    accept();
}
