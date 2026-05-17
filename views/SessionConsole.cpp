//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Session console — shows live protocol exchange during Winlink connect
//

#include "SessionConsole.h"
#include "../TouchStyle.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QJsonObject>

SessionConsole::SessionConsole(PatClient *client, bool touchMode, QWidget *parent)
    : QDialog(parent)
    , m_client(client)
    , m_touchMode(touchMode)
{
    setWindowTitle("Winlink Session");
    setMinimumSize(700, touchMode ? 360 : 260);

    int labelPt   = touchMode ? 13 : 10;
    int statusPt  = touchMode ? 15 : 11;
    int consolePt = touchMode ? 11 : 9;
    int progressH = touchMode ? 22 : 16;

    setStyleSheet(QString(
        "QDialog        { background: #0d0d0d; color: #e0e0e0; }"
        "QLabel         { color: #e0e0e0; font-size: %1pt; }"
        "QLabel#status  { font-size: %2pt; font-weight: bold; }"
        "QProgressBar   { background: #1a1a1a; border: 1px solid #333; border-radius: 4px; height: %3px; }"
        "QProgressBar::chunk { background: #ffa500; border-radius: 3px; }"
        "QPlainTextEdit { background: #0a0a0a; color: #00cc44; font-family: Monospace;"
        "                 font-size: %4pt; border: 1px solid #1a1a1a; border-radius: 4px; }"
        "QPushButton    { border: none; border-radius: 6px; padding: 6px 18px; font-size: 10pt; }"
    ).arg(labelPt).arg(statusPt).arg(progressH).arg(consolePt));

    // ── Status bar ─────────────────────────────────────────────────────────────
    m_statusLabel = new QLabel("Connecting...");
    m_statusLabel->setObjectName("status");

    m_midLabel = new QLabel("");
    m_midLabel->setStyleSheet("color: #888888; font-size: 9pt;");

    auto *statusRow = new QHBoxLayout();
    statusRow->addWidget(m_statusLabel);
    statusRow->addStretch();
    statusRow->addWidget(m_midLabel);

    // ── Progress bar ───────────────────────────────────────────────────────────
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFormat("%p%  %v / %m bytes");
    m_progressBar->hide();

    // ── Console ────────────────────────────────────────────────────────────────
    m_console = new QPlainTextEdit();
    m_console->setReadOnly(true);
    m_console->setMaximumBlockCount(2000);
    m_console->setMaximumHeight(touchMode ? 150 : 110);

    // ── Buttons ────────────────────────────────────────────────────────────────
    m_disconnectBtn = new QPushButton("Disconnect");
    m_closeBtn      = new QPushButton("Close");
    m_closeBtn->setEnabled(false);

    if (touchMode) {
        m_disconnectBtn->setStyleSheet(touchStyle::dangerButton);
        m_closeBtn->setStyleSheet(touchStyle::neutralButton);
    } else {
        m_disconnectBtn->setStyleSheet("background: #c92a2a; color: white;");
        m_closeBtn->setStyleSheet("background: #333333; color: white;");
    }

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRow->addWidget(m_disconnectBtn);
    btnRow->addWidget(m_closeBtn);

    // ── Layout ─────────────────────────────────────────────────────────────────
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);
    layout->addLayout(statusRow);
    layout->addWidget(m_progressBar);
    layout->addWidget(m_console);
    layout->addLayout(btnRow);

    QObject::connect(m_disconnectBtn, &QPushButton::clicked, this, &SessionConsole::onDisconnectClicked);
    QObject::connect(m_closeBtn,      &QPushButton::clicked, this, &QWidget::close);
    QObject::connect(m_client, &PatClient::wsEvent,    this, &SessionConsole::onWsEvent);
    QObject::connect(m_client, &PatClient::statusReady, this, [this](const QJsonObject &s) {
        updateStatus(s);
    });

    appendLog("Session started — " +
              QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
}

void SessionConsole::onWsEvent(const QJsonObject &event)
{
    if (event.contains("LogLine"))
        appendLog(event.value("LogLine").toString().trimmed());

    if (event.contains("Progress"))
        updateProgress(event.value("Progress").toObject());

    if (event.contains("Status")) {
        // Parse WS status object and call updateStatus with normalized fields
        QJsonObject ws = event.value("Status").toObject();
        updateStatus(ws);
    }

    if (event.contains("Notification")) {
        QJsonObject n = event.value("Notification").toObject();
        appendLog("[!] " + n.value("title").toString() + " — " + n.value("body").toString());
    }
}

void SessionConsole::appendLog(const QString &line)
{
    if (line.isEmpty()) return;

    if (line == m_lastLine) {
        ++m_repeatCount;
        int dots = qMin(m_repeatCount * 2, 40);
        QString collapsed = line + " " + QString(dots, '.');
        if (m_repeatCount > 20) collapsed += QString(" (x%1)").arg(m_repeatCount + 1);

        QTextCursor cur = m_console->textCursor();
        cur.movePosition(QTextCursor::End);
        cur.select(QTextCursor::LineUnderCursor);
        cur.removeSelectedText();
        cur.insertText(collapsed);
    } else {
        m_console->appendPlainText(line);
        m_lastLine = line;
        m_repeatCount = 0;
    }
    m_console->ensureCursorVisible();
}

void SessionConsole::updateProgress(const QJsonObject &p)
{
    int total      = p.value("bytes_total").toInt();
    int transferred = p.value("bytes_transferred").toInt();
    QString mid    = p.value("mid").toString();
    QString subject = p.value("subject").toString();
    bool done      = p.value("done").toBool();
    bool receiving = p.value("receiving").toBool();
    bool sending   = p.value("sending").toBool();

    if (total > 0) {
        m_progressBar->show();
        m_progressBar->setMaximum(total);
        m_progressBar->setValue(transferred);
    }

    if (!mid.isEmpty()) {
        QString dir = receiving ? "↓" : sending ? "↑" : "";
        m_midLabel->setText(QString("%1 %2  %3").arg(dir, mid, subject));
    }

    if (done) {
        m_progressBar->setValue(total > 0 ? total : m_progressBar->maximum());
        appendLog(QString("✓ Transfer complete: %1").arg(subject));
    }
}

void SessionConsole::updateStatus(const QJsonObject &status)
{
    bool connected = status.value("connected").toBool();
    bool dialing   = status.value("dialing").toBool();
    QString remote = status.value("remote_addr").toString();

    if (dialing) {
        m_statusLabel->setText("Dialing " + remote + "...");
        m_statusLabel->setStyleSheet("color: #fcc419; font-size: 11pt; font-weight: bold;");
    } else if (connected) {
        m_statusLabel->setText("Connected — " + remote);
        m_statusLabel->setStyleSheet("color: #51cf66; font-size: 11pt; font-weight: bold;");
        m_connected = true;
    } else {
        m_statusLabel->setText("Session ended");
        m_statusLabel->setStyleSheet("color: #aaaaaa; font-size: 11pt; font-weight: bold;");
        m_disconnectBtn->setEnabled(false);
        m_closeBtn->setEnabled(true);
        m_progressBar->hide();
        appendLog("--- Session ended ---");
        m_connected = false;
        emit sessionDone();
    }
}

void SessionConsole::onDisconnectClicked()
{
    appendLog("Disconnecting...");
    m_disconnectBtn->setEnabled(false);
    m_client->disconnect();
}
