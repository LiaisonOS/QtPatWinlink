//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : August 2026
// Purpose : Connect-to-Peer dialog. Purpose-built for the P2P flow —
//           just callsign + Connect. The modem is inherited from the
//           mode QtPatWinlink was launched with (the mode chain already
//           selected VARA HF / VARA FM / ARDOP / Packet, and Pat is
//           bound to that modem). No frequency field: P2P is inherently
//           a coordinated activity where operator tunes their own rig
//           to whatever the peer agreed on.
//

#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QString>

class PeerConnectDialog : public QDialog
{
    Q_OBJECT

public:
    // `activeModem` is the raw --modem CLI value (varahf / varafm /
    // ardop / packet). Dialog maps it to Pat's URL scheme internally.
    explicit PeerConnectDialog(const QString &activeModem, bool touchMode,
                               QWidget *parent = nullptr);

    QString connectUrl() const { return m_result; }

private slots:
    void onAccept();

private:
    bool             m_touchMode;
    QString          m_scheme;      // varahf / varafm / ardop / ax25
    QString          m_modemLabel;  // pretty display name
    QString          m_result;

    QLineEdit       *m_callsign;
};
