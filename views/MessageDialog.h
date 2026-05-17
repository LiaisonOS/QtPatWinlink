//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Message viewer dialog with action toolbar
//

#pragma once

#include <QDialog>
#include <QJsonObject>
#include "../PatClient.h"

class MessageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MessageDialog(const QJsonObject &message, const QString &box,
                           PatClient *client, bool touchMode = false,
                           QWidget *parent = nullptr);

signals:
    void composeRequested(const QString &to, const QString &subject, const QString &body);
    void deleted();
    void archived();
};
