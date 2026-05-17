//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Modal dialog that renders a Pat form HTML in an embedded QWebEngineView.
//           Sets a unique `forminstance` cookie on both PatClient and the WebEngine
//           profile so Pat can key the submitted form data for retrieval afterwards.
//           Closes itself when the form's window.close() fires (post-submit).
//

#pragma once

#include <QDialog>
#include <QWebEngineView>
#include "../PatClient.h"

class FormRenderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FormRenderDialog(PatClient *client, const QString &templatePath,
                              bool touchMode, QWidget *parent = nullptr);

    QString formKey() const { return m_formKey; }

signals:
    void formSubmitted(const QString &formKey);

private:
    PatClient      *m_client;
    bool            m_touchMode;
    QString         m_formKey;
    QWebEngineView *m_view;
};
