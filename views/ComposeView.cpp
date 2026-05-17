//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Compose view — write and queue a new message
//

#include "ComposeView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QJsonObject>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>

ComposeView::ComposeView(PatClient *client, bool touchMode, QWidget *parent)
    : QWidget(parent)
    , m_client(client)
    , m_touchMode(touchMode)
{
    int fontSize  = touchMode ? 16 : 10;
    int fieldMinH = touchMode ? 44 : 0;
    int fieldRad  = touchMode ? 6 : 4;
    int btnRad    = touchMode ? 6 : 4;
    int btnPadV   = touchMode ? 0 : 6;
    int btnPadH   = touchMode ? 22 : 20;
    int btnFontPx = touchMode ? 18 : 0;
    QString btnMinH = touchMode ? "min-height: 58px;" : "";
    QString btnFontRule = touchMode
        ? QString("font-size: %1px;").arg(btnFontPx)
        : QString("font-size: %1pt;").arg(fontSize);

    QString fieldStyle = QString(
        "QLabel { color: #e0e0e0; font-size: %1pt; }"
        "QLineEdit, QTextEdit {"
        "  background: #1e1e1e; color: #e0e0e0;"
        "  border: 1px solid #333333; border-radius: %2px;"
        "  padding: 6px; font-size: %1pt;"
        "}"
        "QLineEdit { min-height: %3px; }"
    ).arg(fontSize).arg(fieldRad).arg(fieldMinH);

    QString btnStyle = QString(
        "QPushButton {"
        "  background: #ffa500; color: #000000; border: none; font-weight: bold;"
        "  border-radius: %1px; padding: %2px %3px; %4 %5"
        "}"
        "QPushButton:hover { background: #ffb733; }"
        "QPushButton#cancel { background: #333333; color: #ffffff; font-weight: normal; }"
        "QPushButton#cancel:hover { background: #444444; }"
    ).arg(btnRad).arg(btnPadV).arg(btnPadH).arg(btnFontRule, btnMinH);

    m_toField      = new QLineEdit();
    m_subjectField = new QLineEdit();
    m_bodyField    = new QTextEdit();
    m_bodyField->setMinimumHeight(200);

    m_attachBtn = new QPushButton("📎 Attach File");
    m_attachBtn->setObjectName("cancel");  // reuse the neutral-gray style
    m_templateBtn = new QPushButton("📋 Use Template");
    m_templateBtn->setObjectName("cancel");
    m_attachList = new QListWidget();
    m_attachList->setMaximumHeight(touchMode ? 120 : 80);
    m_attachList->setStyleSheet(QString(
        "QListWidget { background: #1e1e1e; color: #e0e0e0; border: 1px solid #333333;"
        "  border-radius: %1px; font-size: %2pt; }"
        "QListWidget::item { padding: %3px 10px; }"
        "QListWidget::item:hover { background: #2a2a2a; }"
    ).arg(fieldRad).arg(fontSize).arg(touchMode ? 10 : 4));
    m_attachList->hide();  // hidden until something is attached

    m_sendBtn   = new QPushButton("Send to Outbox");
    m_cancelBtn = new QPushButton("Cancel");
    m_cancelBtn->setObjectName("cancel");

    setStyleSheet(fieldStyle + btnStyle);

    auto *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->addRow("To:", m_toField);
    form->addRow("Subject:", m_subjectField);

    auto *attachRow = new QHBoxLayout();
    attachRow->addWidget(m_attachBtn);
    attachRow->addWidget(m_templateBtn);
    attachRow->addStretch();

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_sendBtn);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 16, 24, 16);
    layout->setSpacing(12);
    layout->addLayout(form);
    layout->addLayout(attachRow);
    layout->addWidget(m_attachList);
    layout->addWidget(m_bodyField);
    layout->addLayout(btnRow);

    QObject::connect(m_sendBtn,   &QPushButton::clicked, this, &ComposeView::onSend);
    QObject::connect(m_cancelBtn, &QPushButton::clicked, this, &ComposeView::done);
    QObject::connect(m_attachBtn, &QPushButton::clicked, this, &ComposeView::onAttachClicked);
    QObject::connect(m_templateBtn, &QPushButton::clicked, this, &ComposeView::templateRequested);
    QObject::connect(m_attachList, &QListWidget::itemClicked,
                     this, &ComposeView::onAttachmentRowClicked);

    QObject::connect(m_client, &PatClient::messagePosted,     this, &ComposeView::onPosted);
    QObject::connect(m_client, &PatClient::messagePostFailed, this, &ComposeView::onPostFailed);
}

void ComposeView::setSending(bool sending)
{
    m_sending = sending;
    m_sendBtn->setEnabled(!sending);
    m_cancelBtn->setEnabled(!sending);
    m_sendBtn->setText(sending ? "Sending…" : "Send to Outbox");
}

void ComposeView::prefill(const QString &to, const QString &subject, const QString &body)
{
    m_toField->setText(to);
    m_subjectField->setText(subject);
    m_bodyField->setPlainText(body);
}

void ComposeView::onSend()
{
    if (m_sending) return;

    if (m_toField->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Missing field", "To field is required.");
        return;
    }

    setSending(true);

    m_client->postMessage(
        m_toField->text().trimmed().toUpper(),
        m_subjectField->text().trimmed(),
        m_bodyField->toPlainText(),
        QString(),
        m_attachments
    );
}

void ComposeView::onPosted()
{
    if (!m_sending) return;
    setSending(false);

    m_toField->clear();
    m_subjectField->clear();
    m_bodyField->clear();
    m_attachments.clear();
    refreshAttachmentList();

    emit done();
}

void ComposeView::onPostFailed(const QString &detail)
{
    if (!m_sending) return;
    setSending(false);

    QMessageBox::critical(this, "Send failed",
        "Could not queue message to Outbox.\n\n" + detail);
}

void ComposeView::onAttachClicked()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this, "Attach Files", QString(), "All Files (*.*)");
    if (files.isEmpty()) return;
    for (const QString &f : files) {
        if (!m_attachments.contains(f)) m_attachments.append(f);
    }
    refreshAttachmentList();
}

void ComposeView::onAttachmentRowClicked(QListWidgetItem *item)
{
    if (!item) return;
    int idx = m_attachList->row(item);
    if (idx < 0 || idx >= m_attachments.size()) return;
    m_attachments.removeAt(idx);
    refreshAttachmentList();
}

static QString humanSize(qint64 bytes)
{
    if (bytes < 1024LL * 1024)      return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    return QString::number(bytes / (1024.0 * 1024), 'f', 1) + " MB";
}

void ComposeView::refreshAttachmentList()
{
    m_attachList->clear();
    for (const QString &path : m_attachments) {
        QFileInfo fi(path);
        auto *it = new QListWidgetItem(QString("📎  %1   (%2)  — tap to remove")
            .arg(fi.fileName(), humanSize(fi.size())));
        m_attachList->addItem(it);
    }
    m_attachList->setVisible(!m_attachments.isEmpty());
}
