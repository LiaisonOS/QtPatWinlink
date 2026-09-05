//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : August 2026
// Purpose : P2P Listen mode config dialog. Manages Pat's `listen` array
//           via /api/config PUT + /api/reload — no Pat restart needed.
//           Scoped to the CURRENT running modem (passed by MainWindow
//           from the --modem CLI arg): a single toggle, not a menu of
//           modems that don't apply to this session.
//

#pragma once

#include <QDialog>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QJsonObject>
#include <QString>
#include "../PatClient.h"

class P2PListenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit P2PListenDialog(PatClient *client, bool touchMode,
                             const QString &activeModem,
                             QWidget *parent = nullptr);

private slots:
    void onConfigReady(const QJsonObject &config);
    void onSaveClicked();
    void onPatError(const QString &msg);

private:
    PatClient   *m_client;
    bool         m_touchMode;
    QString      m_modem;       // canonical (varahf / varafm / ardop / ax25 / pactor)
    QString      m_modemLabel;  // pretty (VARA HF / VARA FM / …)
    QLabel      *m_status;
    QCheckBox   *m_toggle;
    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;
};
