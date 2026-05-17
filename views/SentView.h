//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Sent view — sent messages
//

#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QLabel>
#include "../PatClient.h"

class SentView : public QWidget
{
    Q_OBJECT

public:
    explicit SentView(PatClient *client, bool touchMode, QWidget *parent = nullptr);
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
