//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Compose view — write and queue a new message
//

#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QListWidget>
#include <QStringList>
#include "../PatClient.h"

class ComposeView : public QWidget
{
    Q_OBJECT

public:
    explicit ComposeView(PatClient *client, bool touchMode, QWidget *parent = nullptr);
    void prefill(const QString &to, const QString &subject, const QString &body);

signals:
    void done();
    void templateRequested();   // user tapped "Use Template"

private slots:
    void onSend();
    void onPosted();
    void onPostFailed(const QString &detail);
    void onAttachClicked();
    void onAttachmentRowClicked(QListWidgetItem *item);

private:
    void setSending(bool sending);
    void refreshAttachmentList();

    PatClient  *m_client;
    bool        m_touchMode;
    bool        m_sending = false;
    QLineEdit  *m_toField;
    QLineEdit  *m_subjectField;
    QTextEdit  *m_bodyField;
    QPushButton *m_sendBtn;
    QPushButton *m_cancelBtn;
    QPushButton *m_attachBtn;
    QPushButton *m_templateBtn;
    QListWidget *m_attachList;
    QStringList  m_attachments;     // absolute file paths
};
