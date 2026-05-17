//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Inbox view — lists received messages
//

#include "InboxView.h"
#include "MessageDialog.h"
#include "LongPressFilter.h"
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QScroller>

InboxView::InboxView(PatClient *client, bool touchMode, QWidget *parent)
    : QWidget(parent)
    , m_client(client)
    , m_touchMode(touchMode)
{
    int rowH = touchMode ? 65 : 28;
    int fontSize = touchMode ? 13 : 10;

    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels({"From", "Subject", "Date", "MID"});
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setDefaultSectionSize(rowH);
    m_table->verticalHeader()->hide();
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setStyleSheet(QString(
        "QTableWidget { background: #121212; color: #e0e0e0; gridline-color: #2a2a2a; font-size: %1pt; }"
        "QTableWidget::item { color: #e0e0e0; background: #121212; padding: 0 14px; }"
        "QTableWidget::item:selected { background: #ffa500; color: #000000; }"
        "QHeaderView::section { background: #1e1e1e; color: #aaaaaa; border: none; padding: 6px 14px; font-size: %1pt; }"
    ).arg(fontSize));

    m_emptyLabel = new QLabel("No messages in Inbox");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet("color: #555555; font-size: 13pt;");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_table);
    layout->addWidget(m_emptyLabel);
    m_emptyLabel->hide();

    QObject::connect(m_client, &PatClient::mailboxReady, this, &InboxView::onMailboxReady);
    QObject::connect(m_table, &QTableWidget::cellDoubleClicked, this, &InboxView::onRowDoubleClicked);

    if (touchMode) {
        m_table->setSelectionMode(QAbstractItemView::SingleSelection);
        QScroller::grabGesture(m_table->viewport(), QScroller::LeftMouseButtonGesture);

        auto *longPress = new LongPressFilter(m_table->viewport());
        QObject::connect(longPress, &LongPressFilter::longPressed, this, [this](const QPoint &p) {
            int row = m_table->rowAt(p.y());
            if (row >= 0) onRowDoubleClicked(row, 0);
        });
    }
}

void InboxView::refresh()
{
    m_client->fetchMailbox("in");
}

void InboxView::onMailboxReady(const QString &box, const QJsonArray &messages)
{
    if (box != "in") return;

    m_table->setRowCount(0);

    if (messages.isEmpty()) {
        m_table->hide();
        m_emptyLabel->show();
        return;
    }

    m_table->show();
    m_emptyLabel->hide();

    for (const auto &val : messages) {
        QJsonObject msg = val.toObject();
        int row = m_table->rowCount();
        m_table->insertRow(row);

        auto *fromItem    = new QTableWidgetItem(msg.value("From").toObject().value("Addr").toString());
        auto *subjectItem = new QTableWidgetItem(msg.value("Subject").toString());
        auto *dateItem    = new QTableWidgetItem(msg.value("Date").toString().left(16).replace("T", " "));
        QString mid = msg.value("MID").toString();
        auto *midItem = new QTableWidgetItem(mid);

        subjectItem->setData(Qt::UserRole, mid);
        subjectItem->setData(Qt::UserRole + 1, msg);

        // Bold unread
        if (msg.value("Unread").toBool(false)) {
            QFont f = fromItem->font(); f.setBold(true);
            fromItem->setFont(f); subjectItem->setFont(f);
        }

        m_table->setItem(row, 0, fromItem);
        m_table->setItem(row, 1, subjectItem);
        m_table->setItem(row, 2, dateItem);
        m_table->setItem(row, 3, midItem);
    }
}

void InboxView::onRowDoubleClicked(int row, int)
{
    auto *subjectItem = m_table->item(row, 1);
    if (!subjectItem) return;
    QString mid = subjectItem->data(Qt::UserRole).toString();

    auto *conn = new QMetaObject::Connection;
    *conn = QObject::connect(m_client, &PatClient::messageReady,
        this, [this, conn](const QString &box, const QString &, const QJsonObject &full) {
            if (box != "in") return;
            QObject::disconnect(*conn);
            delete conn;
            MessageDialog dlg(full, "in", m_client, m_touchMode, this);
            QObject::connect(&dlg, &MessageDialog::composeRequested, this, &InboxView::composeRequested);
            QObject::connect(&dlg, &MessageDialog::deleted, this, &InboxView::refresh);
            dlg.exec();
        });

    m_client->fetchMessage("in", mid);
}
