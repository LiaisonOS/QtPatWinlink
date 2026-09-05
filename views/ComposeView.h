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
#include <QCheckBox>
#include <QStringList>
#include <QJsonObject>
#include "../PatClient.h"

class ComposeView : public QWidget
{
    Q_OBJECT

public:
    explicit ComposeView(PatClient *client, bool touchMode,
                         const QString &activeModem = {},
                         QWidget *parent = nullptr);
    void prefill(const QString &to, const QString &subject, const QString &body);
    // Re-fetch Pat config so the P2P Only checkbox default reflects the
    // current listen state. Call every time the view becomes visible —
    // catches "operator just enabled P2P Listen in the other dialog".
    void refreshP2PDefault();

signals:
    void done();
    void templateRequested();   // user tapped "Use Template"

private slots:
    void onSend();
    void onPosted();
    void onPostFailed(const QString &detail);
    void onAttachClicked();
    void onAttachmentRowClicked(QListWidgetItem *item);
    void onConfigReady(const QJsonObject &config);

private:
    void setSending(bool sending);
    void refreshAttachmentList();

    PatClient  *m_client;
    bool        m_touchMode;
    bool        m_sending = false;
    QString     m_modem;    // canonical listen key (varahf / varafm / ax25 / …)
    QLineEdit  *m_toField;
    QLineEdit  *m_subjectField;
    QTextEdit  *m_bodyField;
    QPushButton *m_sendBtn;
    QPushButton *m_cancelBtn;
    QPushButton *m_attachBtn;
    QPushButton *m_templateBtn;
    QListWidget *m_attachList;
    QStringList  m_attachments;     // absolute file paths
    QCheckBox   *m_p2pOnly;         // route P2P-only (never via RMS)
};
