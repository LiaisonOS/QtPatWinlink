//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : July 2026
// Purpose : Manual RMS entry dialog — implementation.
//

#include "ManualRmsDialog.h"
#include "../TouchStyle.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QRegularExpression>
#include <QCheckBox>

ManualRmsDialog::ManualRmsDialog(bool touchMode,
                                  const RmsPrefs::ManualStation &initial,
                                  QWidget *parent)
    : QDialog(parent)
    , m_touchMode(touchMode)
{
    const bool editing = !initial.callsign.trimmed().isEmpty();
    setWindowTitle(editing ? tr("Edit Manual Station") : tr("Add Manual Station"));
    setModal(true);
    setMinimumWidth(touchMode ? 640 : 460);

    const int fontSize = touchMode ? 13 : 10;
    setStyleSheet(QString(
        "QDialog { background: #1a1a1a; color: #e0e0e0; }"
        "QLabel { color: #e0e0e0; font-size: %1pt; }"
        "QLineEdit, QDoubleSpinBox, QSpinBox, QComboBox {"
        "  background: #1e1e1e; color: #e0e0e0; border: 1px solid #333333;"
        "  border-radius: 4px; padding: 4px 8px; font-size: %1pt; }"
        "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button,"
        "QSpinBox::up-button, QSpinBox::down-button { width: 20px; }"
        "QComboBox QAbstractItemView {"
        "  background: #1e1e1e; color: #e0e0e0;"
        "  selection-background-color: #ffa500; selection-color: #000000; }"
    ).arg(fontSize));

    m_callsign = new QLineEdit(this);
    m_callsign->setPlaceholderText("VE2CJR-10");
    m_callsign->setText(initial.callsign);

    m_freqMhz = new QDoubleSpinBox(this);
    m_freqMhz->setDecimals(5);              // 10 Hz resolution (e.g. 7.10325 MHz)
    m_freqMhz->setRange(0.1, 500.0);
    m_freqMhz->setSuffix(" MHz");
    m_freqMhz->setValue(initial.freq_hz > 0 ? initial.freq_hz / 1e6 : 14.10500);
    m_freqMhz->setSingleStep(0.001);

    m_bwHz = new QSpinBox(this);
    m_bwHz->setRange(500, 4000);
    m_bwHz->setSuffix(" Hz");
    m_bwHz->setSingleStep(100);
    m_bwHz->setValue(initial.bw_hz > 0 ? initial.bw_hz : 2300);

    m_modem = new QComboBox(this);
    // Order matters: most-common HF workflow first. "Packet 1200"/"Packet 9600"
    // match how Winlink's RMS list labels packet stations. Both map to the
    // ax25:// PAT scheme — the baud is a pat interface config concern.
    m_modem->addItems({"varahf", "varafm", "ardop", "pactor",
                       "Packet 1200", "Packet 9600"});
    if (!initial.modem.isEmpty()) {
        int idx = m_modem->findText(initial.modem);
        if (idx >= 0) m_modem->setCurrentIndex(idx);
    }

    m_skipQsy = new QCheckBox(tr("Skip QSY (I'll tune the radio manually)"), this);
    m_skipQsy->setChecked(initial.skip_qsy);

    m_notes = new QLineEdit(this);
    m_notes->setPlaceholderText("Optional — e.g. QC EMCOMM gateway");
    m_notes->setText(initial.notes);

    if (touchMode) {
        const int rowH = 48;
        m_callsign->setMinimumHeight(rowH);
        m_freqMhz->setMinimumHeight(rowH);
        m_bwHz->setMinimumHeight(rowH);
        m_modem->setMinimumHeight(rowH);
        m_notes->setMinimumHeight(rowH);
        m_skipQsy->setMinimumHeight(rowH);
    }

    auto *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->setSpacing(10);
    form->addRow(tr("Callsign*:"),    m_callsign);
    form->addRow(tr("Frequency*:"),   m_freqMhz);
    form->addRow(tr("Bandwidth:"),    m_bwHz);
    form->addRow(tr("Modem:"),        m_modem);
    form->addRow(QString(),           m_skipQsy);
    form->addRow(tr("Notes:"),        m_notes);

    auto *hint = new QLabel(tr("Frequency is the DIAL freq. VARA HF default 2300 Hz; "
                                "VARA FM 500 Hz; Packet uses the audio bandwidth. "
                                "Tick \"Skip QSY\" when the radio has no CAT or when you tune it yourself."), this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color: #888; font-size: 9pt;");

    const int btnRadius = touchMode ? 6 : 4;
    const QString actionExtra = touchMode
        ? QString("font-size: 18px; font-weight: bold; min-height: 58px; padding: 0 26px;")
        : QString("padding: 6px 20px;");

    auto *saveBtn   = new QPushButton(editing ? tr("Update") : tr("Save"), this);
    auto *cancelBtn = new QPushButton(tr("Cancel"), this);
    saveBtn->setStyleSheet(QString(
        "QPushButton { background: #ffa500; color: #000000; font-weight: bold;"
        "  border: none; border-radius: %1px; %2 }"
        "QPushButton:hover { background: #ffb733; }"
    ).arg(btnRadius).arg(actionExtra));
    cancelBtn->setStyleSheet(QString(
        "QPushButton { background: #333333; color: white; border: none;"
        "  border-radius: %1px; %2 }"
        "QPushButton:hover { background: #444444; }"
    ).arg(btnRadius).arg(actionExtra));

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(saveBtn);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(12);
    layout->addLayout(form);
    layout->addWidget(hint);
    layout->addLayout(btnRow);

    QObject::connect(saveBtn,   &QPushButton::clicked, this, &ManualRmsDialog::onAccept);
    QObject::connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void ManualRmsDialog::onAccept()
{
    QString cs = m_callsign->text().trimmed().toUpper();
    if (cs.isEmpty()) {
        QMessageBox::warning(this, tr("Missing field"), tr("Callsign is required."));
        m_callsign->setFocus();
        return;
    }
    // Loose format check — accepts BASECALL and BASECALL-SSID (1-2 digit SSID).
    // Winlink stations are almost always -10, -12, -14. We don't want to be
    // too strict — some special-event or gateway callsigns look unusual.
    static const QRegularExpression rx("^[A-Z0-9]{3,7}(-\\d{1,2})?$");
    if (!rx.match(cs).hasMatch()) {
        auto ans = QMessageBox::question(this, tr("Callsign format"),
            tr("\"%1\" doesn't look like a typical Winlink callsign (e.g. VE2CJR-10). "
               "Save anyway?").arg(cs));
        if (ans != QMessageBox::Yes) { m_callsign->setFocus(); return; }
    }

    m_result.callsign = cs;
    m_result.freq_hz  = m_freqMhz->value() * 1e6;
    m_result.bw_hz    = m_bwHz->value();
    m_result.modem    = m_modem->currentText();
    m_result.band     = bandForFreqHz(m_result.freq_hz);
    m_result.notes    = m_notes->text().trimmed();
    m_result.skip_qsy = m_skipQsy->isChecked();
    accept();
}

QString ManualRmsDialog::bandForFreqHz(double freq_hz)
{
    // Amateur band ranges — IARU R2 where borders differ. Only bands
    // that make sense for a Winlink RMS gateway are listed.
    const double mhz = freq_hz / 1e6;
    struct B { double lo, hi; const char *name; };
    static const B bands[] = {
        {1.800,   2.000, "160m"}, {3.500,   4.000, "80m"},
        {5.330,   5.406, "60m"},  {7.000,   7.300, "40m"},
        {10.100, 10.150, "30m"},  {14.000, 14.350, "20m"},
        {18.068, 18.168, "17m"},  {21.000, 21.450, "15m"},
        {24.890, 24.990, "12m"},  {28.000, 29.700, "10m"},
        {50.000, 54.000, "6m"},   {144.000, 148.000, "2m"},
        {222.000, 225.000, "1.25m"}, {420.000, 450.000, "70cm"},
    };
    for (const auto &b : bands)
        if (mhz >= b.lo && mhz <= b.hi) return QString::fromLatin1(b.name);
    return QString();
}

QString ManualRmsDialog::buildConnectUrl(const RmsPrefs::ManualStation &s)
{
    // pat's URL scheme: "Packet 1200"/"Packet 9600" both use ax25:// — the
    // baud is a pat interface config (which KISS TNC) concern, not part of
    // the URL. VARA/ARDOP/PACTOR use the modem name verbatim.
    QString scheme;
    if (s.modem.startsWith("Packet", Qt::CaseInsensitive))
        scheme = "ax25";
    else
        scheme = s.modem.isEmpty() ? "varahf" : s.modem.toLower();

    // pat expects the DIAL freq in kHz — see main.cpp:51 example. Keep 2
    // decimals so 7.10325 MHz round-trips as freq=7103.25 (not 7103); the
    // integer-truncated version silently lost 250 Hz.
    //
    // When skip_qsy is set, we omit ?freq= entirely so pat does not attempt
    // to rigctl the radio — needed for HTs without CAT and for setups where
    // the operator tunes by hand.
    if (s.skip_qsy) {
        return QString("%1:///%2").arg(scheme).arg(s.callsign);
    }
    const double freq_khz = s.freq_hz / 1000.0;
    return QString("%1:///%2?freq=%3&bw=%4")
        .arg(scheme)
        .arg(s.callsign)
        .arg(QString::number(freq_khz, 'f', 2))
        .arg(s.bw_hz);
}
