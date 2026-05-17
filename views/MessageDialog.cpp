//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Message viewer dialog with action toolbar
//

#include "MessageDialog.h"
#include "LongPressFilter.h"
#include "../TouchStyle.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QFrame>
#include <QToolBar>
#include <QAction>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QScroller>
#include <QMenu>
#include <QTextCursor>
#include <QApplication>
#include <QScreen>
#include <QClipboard>
#include <QTimer>

static QString btnStyle(const QString &bg, const QString &hover, bool touch,
                        const QString &textColor = "#e0e0e0", bool bold = false)
{
    int radius = touch ? 6 : 4;
    int padV   = touch ? 0 : 5;
    int padH   = touch ? 22 : 14;
    int fontPx = touch ? 18 : 10;
    QString minH = touch ? "min-height: 58px;" : "";
    QString weight = bold ? "font-weight: bold;" : "";
    return QString(
        "QPushButton { background: %1; color: %3; border: none;"
        "  border-radius: %4px; padding: %5px %6px; font-size: %7%8; %9 %10 }"
        "QPushButton:hover { background: %2; }"
    ).arg(bg, hover, textColor)
     .arg(radius).arg(padV).arg(padH).arg(fontPx).arg(touch ? "px" : "pt").arg(minH, weight);
}

MessageDialog::MessageDialog(const QJsonObject &message, const QString &box,
                             PatClient *client, bool touchMode, QWidget *parent)
    : QDialog(parent)
{
    QString subject = message.value("Subject").toString();
    QString from    = message.value("From").toObject().value("Addr").toString();
    QString mid     = message.value("MID").toString();

    QString toList;
    for (const auto &t : message.value("To").toArray())
        toList += t.toObject().value("Addr").toString() + " ";
    toList = toList.trimmed();

    QString body = message.value("Body").toString();

    setWindowTitle("Message — " + subject);
    setMinimumSize(700, 520);

    int labelPt   = touchMode ? 13 : 10;
    int hkeyPt    = touchMode ? 13 : 10;
    int bodyPt    = touchMode ? 13 : 10;

    setStyleSheet(QString(
        "QDialog { background: #121212; color: #e0e0e0; }"
        "QLabel  { color: #e0e0e0; font-size: %1pt; }"
        "QLabel#hkey { color: #888888; font-weight: bold; font-size: %2pt; }"
        "QTextEdit { background: #1e1e1e; color: #e0e0e0; border: 1px solid #2a2a2a;"
        "            border-radius: 6px; font-size: %3pt; padding: 6px; }"
        "QFrame#sep { color: #2a2a2a; }"
    ).arg(labelPt).arg(hkeyPt).arg(bodyPt));

    // ── Toolbar ───────────────────────────────────────────────────────────────
    auto *toolbar = new QWidget();
    toolbar->setStyleSheet("background: #1a1a1a; border-bottom: 1px solid #2a2a2a;");

    auto *btnReply    = new QPushButton("Reply");
    auto *btnReplyAll = new QPushButton("Reply All");
    auto *btnForward  = new QPushButton("Forward");
    auto *btnEditNew  = new QPushButton("Edit as New");
    QPushButton *btnCopy = touchMode ? new QPushButton("Copy") : nullptr;
    auto *btnArchive  = new QPushButton("Archive");
    auto *btnDelete   = new QPushButton("Delete");
    auto *btnClose    = new QPushButton("Close");

    btnReply->setStyleSheet(btnStyle("#ffa500", "#ffb733", touchMode, "#000000", true));
    btnReplyAll->setStyleSheet(btnStyle("#ffa500", "#ffb733", touchMode, "#000000", true));
    btnForward->setStyleSheet(btnStyle("#333333", "#444444", touchMode));
    btnEditNew->setStyleSheet(btnStyle("#333333", "#444444", touchMode));
    if (btnCopy) btnCopy->setStyleSheet(btnStyle("#333333", "#444444", touchMode));
    btnArchive->setStyleSheet(btnStyle("#333333", "#444444", touchMode));
    btnDelete->setStyleSheet(btnStyle("#c92a2a", "#e03131", touchMode));
    btnClose->setStyleSheet(btnStyle("#2a2a2a", "#3a3a3a", touchMode));

    auto *tbLayout = new QHBoxLayout(toolbar);
    tbLayout->setContentsMargins(8, 6, 8, 6);
    tbLayout->setSpacing(6);
    tbLayout->addWidget(btnReply);
    tbLayout->addWidget(btnReplyAll);
    tbLayout->addWidget(btnForward);
    tbLayout->addWidget(btnEditNew);
    if (btnCopy) tbLayout->addWidget(btnCopy);
    tbLayout->addSpacing(16);
    tbLayout->addWidget(btnArchive);
    tbLayout->addWidget(btnDelete);
    tbLayout->addStretch();
    tbLayout->addWidget(btnClose);

    // ── Header ────────────────────────────────────────────────────────────────
    auto *fromKey    = new QLabel("From:");    fromKey->setObjectName("hkey");
    auto *toKey      = new QLabel("To:");      toKey->setObjectName("hkey");
    auto *subjectKey = new QLabel("Subject:"); subjectKey->setObjectName("hkey");
    auto *dateKey    = new QLabel("Date:");    dateKey->setObjectName("hkey");

    auto *fromVal    = new QLabel(from);
    auto *toVal      = new QLabel(toList);
    auto *subjectVal = new QLabel(subject);
    auto *dateVal    = new QLabel(message.value("Date").toString().left(19).replace("T", " "));

    auto *header = new QGridLayout();
    header->setColumnStretch(1, 1);
    header->setSpacing(6);
    header->setContentsMargins(0, 0, 0, 0);
    header->addWidget(fromKey,    0, 0); header->addWidget(fromVal,    0, 1);
    header->addWidget(toKey,      1, 0); header->addWidget(toVal,      1, 1);
    header->addWidget(subjectKey, 2, 0); header->addWidget(subjectVal, 2, 1);
    header->addWidget(dateKey,    3, 0); header->addWidget(dateVal,    3, 1);

    auto *sep = new QFrame();
    sep->setObjectName("sep");
    sep->setFrameShape(QFrame::HLine);

    // ── Body ──────────────────────────────────────────────────────────────────
    auto *bodyEdit = new QTextEdit();
    bodyEdit->setReadOnly(true);
    bodyEdit->setPlainText(body);
    bodyEdit->setFont(QFont("Monospace", touchMode ? 12 : 10));

    if (touchMode) {
        bodyEdit->setTextInteractionFlags(Qt::NoTextInteraction);
        QScroller::grabGesture(bodyEdit->viewport(), QScroller::LeftMouseButtonGesture);

        auto *lp = new LongPressFilter(bodyEdit->viewport());
        QObject::connect(lp, &LongPressFilter::longPressed, this,
            [this, bodyEdit](const QPoint &pos) {
                bodyEdit->setTextInteractionFlags(Qt::TextSelectableByMouse |
                                                  Qt::TextSelectableByKeyboard);
                QTextCursor cur = bodyEdit->cursorForPosition(pos);
                cur.select(QTextCursor::WordUnderCursor);
                bodyEdit->setTextCursor(cur);

                auto *menu = new QMenu(this);
                menu->setAttribute(Qt::WA_DeleteOnClose);
                menu->setStyleSheet(touchStyle::menuStyle);

                QAction *copyAct   = menu->addAction("Copy");
                QAction *selAllAct = menu->addAction("Select All");

                QObject::connect(copyAct, &QAction::triggered, bodyEdit, [bodyEdit]() {
                    bodyEdit->copy();
                    bodyEdit->setTextInteractionFlags(Qt::NoTextInteraction);
                });
                QObject::connect(selAllAct, &QAction::triggered, bodyEdit, [bodyEdit]() {
                    bodyEdit->selectAll();
                });

                menu->popup(bodyEdit->viewport()->mapToGlobal(pos));
            });
    }

    // ── Layout ────────────────────────────────────────────────────────────────
    auto *content = new QVBoxLayout();
    content->setContentsMargins(16, 12, 16, 12);
    content->setSpacing(10);
    content->addLayout(header);
    content->addWidget(sep);
    content->addWidget(bodyEdit);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(toolbar);
    layout->addLayout(content);

    // ── Actions ───────────────────────────────────────────────────────────────
    QObject::connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);

    QObject::connect(btnReply, &QPushButton::clicked, this, [this, from, subject, body]() {
        QString quoted = "\n\n--- Original Message ---\n" + body;
        emit composeRequested(from, "Re: " + subject, quoted);
        accept();
    });

    QObject::connect(btnReplyAll, &QPushButton::clicked, this, [this, from, toList, subject, body]() {
        QString allTo = (from + " " + toList).trimmed();
        QString quoted = "\n\n--- Original Message ---\n" + body;
        emit composeRequested(allTo, "Re: " + subject, quoted);
        accept();
    });

    QObject::connect(btnForward, &QPushButton::clicked, this, [this, subject, body]() {
        QString fwdBody = "\n\n--- Forwarded Message ---\n" + body;
        emit composeRequested("", "Fwd: " + subject, fwdBody);
        accept();
    });

    QObject::connect(btnEditNew, &QPushButton::clicked, this, [this, from, subject, body]() {
        emit composeRequested(from, subject, body);
        accept();
    });

    QObject::connect(btnDelete, &QPushButton::clicked, this, [this, client, box, mid]() {
        auto btn = QMessageBox::question(this, "Delete", "Delete this message?",
                                         QMessageBox::Yes | QMessageBox::No);
        if (btn == QMessageBox::Yes) {
            client->deleteMessage(box, mid);
            emit deleted();
            accept();
        }
    });

    QObject::connect(btnArchive, &QPushButton::clicked, this, [this, client, box, mid]() {
        // Pat doesn't have a direct archive API — move by re-posting is complex.
        // For now emit signal so parent can handle it.
        Q_UNUSED(client) Q_UNUSED(box) Q_UNUSED(mid)
        emit archived();
        accept();
    });

    if (btnCopy) {
        QObject::connect(btnCopy, &QPushButton::clicked, this, [btnCopy, bodyEdit, body]() {
            QString text;
            if (bodyEdit->textCursor().hasSelection()) {
                text = bodyEdit->textCursor().selectedText();
                text.replace(QChar::ParagraphSeparator, '\n');
            } else {
                text = body;
            }
            QApplication::clipboard()->setText(text);
            bodyEdit->setTextInteractionFlags(Qt::NoTextInteraction);
            btnCopy->setText("Copied ✓");
            QTimer::singleShot(1500, btnCopy, [btnCopy]() { btnCopy->setText("Copy"); });
        });
    }

    if (touchMode) {
        QWidget *parentWin = parentWidget() ? parentWidget()->window() : nullptr;
        QRect bounds = parentWin ? parentWin->geometry()
                                 : QApplication::primaryScreen()->availableGeometry();
        const int margin = 32;
        setGeometry(bounds.adjusted(margin, margin, -margin, -margin));
    }
}
