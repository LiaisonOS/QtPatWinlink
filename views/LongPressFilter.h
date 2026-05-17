//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Event filter that detects a long-press on any widget and emits a signal.
//           Used in touch mode to substitute for context menu / double-tap / word-select.
//

#pragma once

#include <QObject>
#include <QTimer>
#include <QEvent>
#include <QMouseEvent>
#include <QPoint>
#include <QWidget>

class LongPressFilter : public QObject
{
    Q_OBJECT

public:
    explicit LongPressFilter(QWidget *target, int holdMs = 450, QObject *parent = nullptr)
        : QObject(parent ? parent : target)
    {
        m_timer.setSingleShot(true);
        m_timer.setInterval(holdMs);
        QObject::connect(&m_timer, &QTimer::timeout, this, [this]() {
            if (m_armed) emit longPressed(m_pressPos);
        });
        target->installEventFilter(this);
    }

signals:
    void longPressed(const QPoint &localPos);

protected:
    bool eventFilter(QObject *, QEvent *event) override
    {
        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                m_pressPos = me->pos();
                m_armed = true;
                m_timer.start();
            }
            break;
        }
        case QEvent::MouseMove: {
            auto *me = static_cast<QMouseEvent *>(event);
            if ((me->pos() - m_pressPos).manhattanLength() > 12) {
                m_timer.stop();
                m_armed = false;
            }
            break;
        }
        case QEvent::MouseButtonRelease:
            m_timer.stop();
            m_armed = false;
            break;
        default:
            break;
        }
        return false;  // never consume
    }

private:
    QTimer m_timer;
    QPoint m_pressPos;
    bool   m_armed = false;
};
