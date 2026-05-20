//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Position Report dialog — sends a Winlink position report
//           (lat, lon, comment, current date/time) to the outbox via Pat.
//

#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QJsonObject>
#include "../PatClient.h"

class PositionReportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PositionReportDialog(PatClient *client, bool touchMode, QWidget *parent = nullptr);

signals:
    void sent();

private slots:
    void onGetFromGps();
    void onPositionReady(const QJsonObject &pos);
    void onSend();

private:
    PatClient   *m_client;
    bool         m_touchMode;
    QLineEdit   *m_latField;
    QLineEdit   *m_lonField;
    QPushButton *m_gpsBtn;
    QTextEdit   *m_commentField;
    QPushButton *m_sendBtn;
    QPushButton *m_cancelBtn;
};
