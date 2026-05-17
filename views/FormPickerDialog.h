//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Modal form picker — Category dropdown + flat list of forms in that category.
//           Emits formSelected(template_path) when the user picks a form.
//

#pragma once

#include <QDialog>
#include <QJsonObject>
#include <QJsonArray>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include "../PatClient.h"

class FormPickerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FormPickerDialog(PatClient *client, bool touchMode, QWidget *parent = nullptr);

signals:
    void formSelected(const QString &templatePath);

private slots:
    void onCatalogReady(const QJsonObject &catalog);
    void onCategoryChanged(const QString &category);
    void onOpenClicked();

private:
    void collectForms(const QJsonObject &folder, const QString &pathPrefix,
                      QJsonArray &out) const;
    void populateForms(const QString &category);

    PatClient    *m_client;
    bool          m_touchMode;
    QJsonObject   m_catalog;          // root: Standard_Forms
    QPushButton  *m_categoryBtn;
    QString       m_currentCategory;
    QListWidget  *m_formList;
    QPushButton  *m_openBtn;
    QPushButton  *m_cancelBtn;
    QLabel       *m_emptyLabel;
};
