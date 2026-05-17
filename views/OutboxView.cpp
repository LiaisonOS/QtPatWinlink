//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Outbox view — pending outbound messages
//

#include "OutboxView.h"
#include "MessageDialog.h"
#include "LongPressFilter.h"
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QScroller>

OutboxView::OutboxView(PatClient *client, bool touchMode, QWidget *parent)
    : QWidget(parent)
    , m_client(client)
    , m_touchMode(touchMode)
{
    int rowH = touchMode ? 65 : 28;
    int fontSize = touchMode ? 13 : 10;

    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels({"To", "Subject", "Date", "MID"});
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

    m_emptyLabel = new QLabel("Outbox is empty");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet("color: #555555; font-size: 13pt;");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_table);
    layout->addWidget(m_emptyLabel);
    m_emptyLabel->hide();

    QObject::connect(m_client, &PatClient::mailboxReady, this, &OutboxView::onMailboxReady);

    auto openRow = [this](int row) {
        auto *item = m_table->item(row, 1);
        if (!item) return;
        QString mid = item->data(Qt::UserRole).toString();
        auto *conn = new QMetaObject::Connection;
        *conn = QObject::connect(m_client, &PatClient::messageReady,
            this, [this, conn](const QString &box, const QString &, const QJsonObject &full) {
                if (box != "out") return;
                QObject::disconnect(*conn); delete conn;
                MessageDialog dlg(full, "out", m_client, m_touchMode, this);
                QObject::connect(&dlg, &MessageDialog::composeRequested, this, &OutboxView::composeRequested);
                QObject::connect(&dlg, &MessageDialog::deleted, this, &OutboxView::refresh);
                dlg.exec();
            });
        m_client->fetchMessage("out", mid);
    };

    QObject::connect(m_table, &QTableWidget::cellDoubleClicked, this,
                     [openRow](int row, int) { openRow(row); });

    if (touchMode) {
        m_table->setSelectionMode(QAbstractItemView::SingleSelection);
        QScroller::grabGesture(m_table->viewport(), QScroller::LeftMouseButtonGesture);

        auto *lp = new LongPressFilter(m_table->viewport());
        QObject::connect(lp, &LongPressFilter::longPressed, this, [this, openRow](const QPoint &p) {
            int row = m_table->rowAt(p.y());
            if (row >= 0) openRow(row);
        });
    }
}

void OutboxView::refresh()
{
    m_client->fetchMailbox("out");
}

void OutboxView::onMailboxReady(const QString &box, const QJsonArray &messages)
{
    if (box != "out") return;

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
        auto *subjectItem = new QTableWidgetItem(msg.value("Subject").toString());
        subjectItem->setData(Qt::UserRole,     msg.value("MID").toString());
        subjectItem->setData(Qt::UserRole + 1, msg);
        QString to = msg.value("To").toArray().first().toObject().value("Addr").toString();
        m_table->setItem(row, 0, new QTableWidgetItem(to));
        m_table->setItem(row, 1, subjectItem);
        m_table->setItem(row, 2, new QTableWidgetItem(msg.value("Date").toString().left(16).replace("T", " ")));
        m_table->setItem(row, 3, new QTableWidgetItem(msg.value("MID").toString()));
    }
}
