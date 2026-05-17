//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Inbox view — lists received messages
//

#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QLabel>
#include "../PatClient.h"

class InboxView : public QWidget
{
    Q_OBJECT

public:
    explicit InboxView(PatClient *client, bool touchMode, QWidget *parent = nullptr);
    void refresh();

signals:
    void composeRequested(const QString &to = {}, const QString &subject = {}, const QString &body = {});

private slots:
    void onMailboxReady(const QString &box, const QJsonArray &messages);
    void onRowDoubleClicked(int row, int col);

private:
    PatClient    *m_client;
    bool          m_touchMode;
    QTableWidget *m_table;
    QLabel       *m_emptyLabel;
};
