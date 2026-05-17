//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : QtPatWinlink main window — Desktop and Touch UI
//

#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QApplication>
#include <QScreen>
#include <QTimer>
#include <QMessageBox>
#include <QStatusBar>
#include "views/FormPickerDialog.h"
#include "views/FormRenderDialog.h"

MainWindow::MainWindow(const QString &patUrl, bool touchMode,
                       const QString &band, const QString &modem, QWidget *parent)
    : QMainWindow(parent)
    , m_client(new PatClient(patUrl, this))
    , m_touchMode(touchMode)
{
    setWindowTitle("Pat Winlink — LiaisonOS");

    if (touchMode)
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    m_inbox   = new InboxView(m_client, m_touchMode, this);
    m_outbox  = new OutboxView(m_client, m_touchMode, this);
    m_sent    = new SentView(m_client, m_touchMode, this);
    m_archive = new ArchiveView(m_client, m_touchMode, this);
    m_compose = new ComposeView(m_client, m_touchMode, this);
    m_connect = new ConnectView(m_client, m_touchMode, this);
    m_actionMenu = new ActionMenu(m_touchMode, this);

    buildUI();

    QObject::connect(m_client, &PatClient::statusReady, this, &MainWindow::onStatusReady);
    QObject::connect(m_client, &PatClient::wsEvent,     this, &MainWindow::onWsEvent);

    // Navigate to compose from inbox (reply/new)
    QObject::connect(m_inbox,  &InboxView::composeRequested,   this, &MainWindow::showCompose);
    QObject::connect(m_outbox, &OutboxView::composeRequested,  this, &MainWindow::showCompose);
    QObject::connect(m_sent,   &SentView::composeRequested,    this, &MainWindow::showCompose);
    QObject::connect(m_archive,&ArchiveView::composeRequested, this, &MainWindow::showCompose);
    QObject::connect(m_actionMenu, &ActionMenu::composeRequested, this, [this]() { showCompose(); });
    QObject::connect(m_actionMenu, &ActionMenu::connectRequested, this, &MainWindow::showConnect);
    QObject::connect(m_actionMenu, &ActionMenu::closeRequested,   this, &QWidget::close);
    QObject::connect(m_actionMenu, &ActionMenu::formsUpdateRequested, this, [this]() {
        setStatusActivity("●  Updating form templates...");
        m_client->updateFormTemplates();
    });
    QObject::connect(m_client, &PatClient::formTemplatesUpdated, this, [this]() {
        setStatusActivity("●  Form templates updated");
        QTimer::singleShot(3000, this, [this]() { setStatusIdle("●  Idle"); });
    });
    QObject::connect(m_actionMenu, &ActionMenu::aboutRequested,   this, [this]() {
        QMessageBox::about(this, "About QtPatWinlink",
            "<h2>QtPatWinlink</h2>"
            "<p>Native Qt client for Pat Winlink — replaces the browser-based "
            "Pat web UI with a touch-friendly application built for emergency "
            "communications in the field.</p>"
            "<p><b>OS:</b> LiaisonOS<br>"
            "<b>Author:</b> Sylvain Deguire — VA2OPS</p>"
            "<p>© 2026</p>");
    });

    // Compose done → back to inbox
    QObject::connect(m_compose, &ComposeView::done, this, &MainWindow::showInbox);
    QObject::connect(m_connect, &ConnectView::done, this, &MainWindow::showInbox);

    // "Use Template" → open form picker → render dialog → fetch submitted data → prefill composer
    QObject::connect(m_compose, &ComposeView::templateRequested, this, [this]() {
        auto *picker = new FormPickerDialog(m_client, m_touchMode, this);
        picker->setAttribute(Qt::WA_DeleteOnClose);
        QObject::connect(picker, &FormPickerDialog::formSelected, this,
            [this](const QString &templatePath) {
                auto *render = new FormRenderDialog(m_client, templatePath, m_touchMode, this);
                render->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(render, &FormRenderDialog::formSubmitted, this,
                    [this](const QString &) {
                        setStatusActivity("●  Form submitted — importing into composer...");
                        m_client->fetchFormData();
                    });
                render->show();
            });
        picker->show();
    });

    QObject::connect(m_client, &PatClient::formDataReady, this,
        [this](const QString &to, const QString &cc,
               const QString &subject, const QString &body) {
            QString fullTo = to;
            if (!cc.isEmpty()) {
                if (!fullTo.isEmpty()) fullTo += " ";
                fullTo += cc;
            }
            m_compose->prefill(fullTo, subject, body);
            setStatusActivity("●  Form data imported into composer");
            QTimer::singleShot(3000, this, [this]() { setStatusIdle("●  Idle"); });
        });

    if (!band.isEmpty() || !modem.isEmpty())
        m_connect->setDefaultFilters(band, modem);

    m_client->connectWebSocket();

    // Poll status every 5 seconds
    auto *timer = new QTimer(this);
    QObject::connect(timer, &QTimer::timeout, m_client, &PatClient::fetchStatus);
    timer->start(5000);

    showInbox();
}

void MainWindow::buildUI()
{
    int tabHeight  = m_touchMode ? 72 : 36;
    int fontSize   = m_touchMode ? 16 : 11;

    QString tabStyle = QString(
        "QPushButton {"
        "  border: none;"
        "  border-bottom: 3px solid transparent;"
        "  padding: 4px 16px;"
        "  font-size: %1pt;"
        "  color: #cccccc;"
        "  background: #1e1e1e;"
        "}"
        "QPushButton:hover { color: #ffffff; }"
        "QPushButton[active=true] {"
        "  border-bottom: 3px solid #ffa500;"
        "  color: #ffffff;"
        "}"
    ).arg(fontSize);

    m_btnInbox   = new QPushButton("Inbox");
    m_btnOutbox  = new QPushButton("Outbox");
    m_btnSent    = new QPushButton("Sent");
    m_btnArchive = new QPushButton("Archive");
    m_btnAction  = new QPushButton("Action ▾");

    for (auto *btn : {m_btnInbox, m_btnOutbox, m_btnSent, m_btnArchive, m_btnAction}) {
        btn->setStyleSheet(tabStyle);
        btn->setFixedHeight(tabHeight);
        btn->setCheckable(false);
    }

    m_statusLabel = new QLabel("●  Idle");
    setStatusIdle("●  Idle");

    statusBar()->setSizeGripEnabled(false);
    statusBar()->setStyleSheet(
        "QStatusBar { background: #1a1a1a; border-top: 1px solid #2a2a2a; }"
        "QStatusBar::item { border: none; }"
    );
    if (m_touchMode) statusBar()->setMinimumHeight(40);
    statusBar()->addWidget(m_statusLabel, 1);

    auto *topBar = new QHBoxLayout();
    topBar->setContentsMargins(0, 0, 0, 0);
    topBar->setSpacing(0);
    topBar->addWidget(m_btnAction);
    topBar->addWidget(m_btnInbox);
    topBar->addWidget(m_btnOutbox);
    topBar->addWidget(m_btnSent);
    topBar->addWidget(m_btnArchive);
    topBar->addStretch();

    if (m_touchMode) {
        auto *minBtn   = new QPushButton("—");
        auto *closeBtn = new QPushButton("✕");

        QString winBtnBase = QString(
            "QPushButton { background: #1e1e1e; color: #cccccc; border: none;"
            "  font-size: %1pt; min-width: %2px; }"
        ).arg(fontSize).arg(tabHeight);

        minBtn->setStyleSheet(winBtnBase +
            "QPushButton:hover { background: #2a2a2a; color: #ffffff; }");
        closeBtn->setStyleSheet(winBtnBase +
            "QPushButton:hover { background: #c92a2a; color: #ffffff; }");

        minBtn->setFixedHeight(tabHeight);
        closeBtn->setFixedHeight(tabHeight);

        topBar->addWidget(minBtn);
        topBar->addWidget(closeBtn);

        QObject::connect(minBtn,   &QPushButton::clicked, this, &QWidget::showMinimized);
        QObject::connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);
    }

    auto *topWidget = new QWidget();
    topWidget->setStyleSheet("background: #1e1e1e;");
    topWidget->setLayout(topBar);

    m_stack = new QStackedWidget();
    m_stack->addWidget(m_inbox);
    m_stack->addWidget(m_outbox);
    m_stack->addWidget(m_sent);
    m_stack->addWidget(m_archive);
    m_stack->addWidget(m_compose);
    m_stack->addWidget(m_connect);

    auto *mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(topWidget);
    mainLayout->addWidget(m_stack);

    auto *central = new QWidget();
    central->setLayout(mainLayout);
    central->setStyleSheet("background: #121212;");
    setCentralWidget(central);

    QObject::connect(m_btnInbox,   &QPushButton::clicked, this, &MainWindow::showInbox);
    QObject::connect(m_btnOutbox,  &QPushButton::clicked, this, &MainWindow::showOutbox);
    QObject::connect(m_btnSent,    &QPushButton::clicked, this, &MainWindow::showSent);
    QObject::connect(m_btnArchive, &QPushButton::clicked, this, &MainWindow::showArchive);
    QObject::connect(m_btnAction,  &QPushButton::clicked, this, [this]() {
        if (m_touchMode)
            m_actionMenu->setMinimumWidth(QApplication::primaryScreen()->geometry().width() / 3);
        m_actionMenu->popup(m_btnAction->mapToGlobal(m_btnAction->rect().bottomLeft()));
    });

    resize(m_touchMode ? 1024 : 900, m_touchMode ? 600 : 650);
}

void MainWindow::setActiveTab(QPushButton *btn)
{
    for (auto *b : {m_btnInbox, m_btnOutbox, m_btnSent, m_btnArchive, m_btnAction}) {
        b->setProperty("active", b == btn);
        b->style()->unpolish(b);
        b->style()->polish(b);
    }
}

void MainWindow::showInbox()
{
    setActiveTab(m_btnInbox);
    m_stack->setCurrentWidget(m_inbox);
    m_inbox->refresh();
}

void MainWindow::showOutbox()
{
    setActiveTab(m_btnOutbox);
    m_stack->setCurrentWidget(m_outbox);
    m_outbox->refresh();
}

void MainWindow::showSent()
{
    setActiveTab(m_btnSent);
    m_stack->setCurrentWidget(m_sent);
    m_sent->refresh();
}

void MainWindow::showArchive()
{
    setActiveTab(m_btnArchive);
    m_stack->setCurrentWidget(m_archive);
    m_archive->refresh();
}

void MainWindow::showCompose(const QString &to, const QString &subject, const QString &body)
{
    setActiveTab(nullptr);
    m_compose->prefill(to, subject, body);
    m_stack->setCurrentWidget(m_compose);
}

void MainWindow::showConnect()
{
    setActiveTab(nullptr);
    m_stack->setCurrentWidget(m_connect);
    m_connect->refresh();
}

void MainWindow::onStatusReady(const QJsonObject &status)
{
    bool connected = status.value("connected").toBool();
    setStatusIdle(QString("●  ") + (connected ? "Connected" : "Idle"),
                  connected ? "#51cf66" : "#888888");
}

void MainWindow::setStatusIdle(const QString &text, const QString &color)
{
    m_statusLabel->setStyleSheet(QString("color: %1; font-size: 11pt; padding: 0 10px;").arg(color));
    m_statusLabel->setText(text);
}

void MainWindow::setStatusActivity(const QString &text)
{
    m_statusLabel->setStyleSheet("color: #ffa500; font-size: 12pt; font-weight: bold; padding: 0 10px;");
    m_statusLabel->setText(text);
}

void MainWindow::onWsEvent(const QJsonObject &event)
{
    QString type = event.value("type").toString();
    if (type == "newMessage") {
        // Refresh whichever mailbox is currently shown
        auto *current = m_stack->currentWidget();
        if (current == m_inbox)   m_inbox->refresh();
        if (current == m_outbox)  m_outbox->refresh();
        if (current == m_sent)    m_sent->refresh();
        if (current == m_archive) m_archive->refresh();
    }
}
