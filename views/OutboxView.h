//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Outbox view — pending outbound messages
//

#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QLabel>
#include "../PatClient.h"

class OutboxView : public QWidget
{
    Q_OBJECT

public:
    explicit OutboxView(PatClient *client, bool touchMode, QWidget *parent = nullptr);
    void refresh();

signals:
    void composeRequested(const QString &to = {}, const QString &subject = {}, const QString &body = {});

private slots:
    void onMailboxReady(const QString &box, const QJsonArray &messages);

private:
    PatClient    *m_client;
    bool          m_touchMode;
    QTableWidget *m_table;
    QLabel       *m_emptyLabel;
};
