//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : July 2026
// Purpose : Manual RMS entry dialog — used when Winlink's online RMS list
//           is unavailable (no internet) or to reach an unlisted BBS.
//           Produces a RmsPrefs::ManualStation persisted alongside favorites.
//

#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include "../RmsPrefs.h"

class ManualRmsDialog : public QDialog
{
    Q_OBJECT

public:
    // If `initial` is a valid entry (callsign non-empty), the dialog opens
    // in edit mode and the Save button title changes to "Update".
    explicit ManualRmsDialog(bool touchMode,
                             const RmsPrefs::ManualStation &initial = {},
                             QWidget *parent = nullptr);

    RmsPrefs::ManualStation result() const { return m_result; }

    // Helper — derives "40m", "20m", etc. from a frequency in Hz. Uses
    // amateur-band ranges (Region 2 borders where they differ). Empty
    // string when the freq doesn't fall in a ham band we recognise.
    static QString bandForFreqHz(double freq_hz);

    // Builds the pat connect URL. Example:
    //   varahf:///VE2CJR-10?freq=14105&bw=2300
    // freq_hz is the DIAL freq the operator entered; pat's ?freq= param
    // is the same value in kHz (integer).
    static QString buildConnectUrl(const RmsPrefs::ManualStation &s);

private slots:
    void onAccept();

private:
    bool                     m_touchMode;
    RmsPrefs::ManualStation  m_result;

    QLineEdit               *m_callsign;
    QDoubleSpinBox          *m_freqMhz;
    QSpinBox                *m_bwHz;
    QComboBox               *m_modem;
    QLineEdit               *m_notes;
};
