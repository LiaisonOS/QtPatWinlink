//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Modal form picker — implementation.
//

#include "FormPickerDialog.h"
#include "../TouchStyle.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QScreen>

FormPickerDialog::FormPickerDialog(PatClient *client, bool touchMode, QWidget *parent)
    : QDialog(parent)
    , m_client(client)
    , m_touchMode(touchMode)
{
    setWindowTitle("Select Form Template");
    setMinimumSize(640, 480);

    int fontSize = touchMode ? 13 : 10;
    setStyleSheet(QString(
        "QDialog { background: #121212; color: #e0e0e0; }"
        "QLabel  { color: #e0e0e0; font-size: %1pt; }"
    ).arg(fontSize));

    auto *catLabel = new QLabel("Category:");

    m_categoryBtn = new QPushButton("Loading...    ▾");
    m_categoryBtn->setEnabled(false);
    m_categoryBtn->setStyleSheet(QString(
        "QPushButton { background: #1e1e1e; color: #e0e0e0; border: 1px solid #404040;"
        "  border-radius: %1px; %2 text-align: left; padding-left: 20px; padding-right: 20px; }"
        "QPushButton:hover { background: #2a2a2a; }"
        "QPushButton:disabled { color: #666666; }"
    ).arg(touchMode ? 6 : 4)
     .arg(touchMode ? "font-size: 16px; min-height: 58px;"
                    : QString("font-size: %1pt; min-height: 30px;").arg(fontSize)));

    auto *catRow = new QHBoxLayout();
    catRow->setSpacing(12);
    catRow->addWidget(catLabel);
    catRow->addWidget(m_categoryBtn, 1);

    m_formList = new QListWidget();
    m_formList->setStyleSheet(QString(
        "QListWidget { background: #1e1e1e; color: #e0e0e0; border: 1px solid #333333;"
        "  border-radius: 6px; font-size: %1pt; }"
        "QListWidget::item { padding: %2px 14px; background: #1e1e1e; }"
        "QListWidget::item:selected { background: #ffa500; color: #000000; }"
    ).arg(fontSize).arg(touchMode ? 12 : 6));

    m_emptyLabel = new QLabel("Pick a category to see available forms.");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet("color: #666666; padding: 20px;");

    m_openBtn   = new QPushButton("Open Form");
    m_cancelBtn = new QPushButton("Cancel");
    m_openBtn->setEnabled(false);

    QString actionExtra = touchMode
        ? "font-size: 18px; font-weight: bold; min-height: 58px; padding: 0 26px;"
        : "padding: 6px 20px;";
    int btnRadius = touchMode ? 6 : 4;

    m_openBtn->setStyleSheet(QString(
        "QPushButton { background: #ffa500; color: #000000; border: none; border-radius: %1px; %2 }"
        "QPushButton:hover { background: #ffb733; }"
        "QPushButton:disabled { background: #5a3a00; color: #888888; }"
    ).arg(btnRadius).arg(actionExtra));
    m_cancelBtn->setStyleSheet(QString(
        "QPushButton { background: #333333; color: #ffffff; border: none; border-radius: %1px; %2 }"
        "QPushButton:hover { background: #444444; }"
    ).arg(btnRadius).arg(actionExtra));

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_openBtn);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(12);
    layout->addLayout(catRow);
    layout->addWidget(m_formList, 1);
    layout->addWidget(m_emptyLabel);
    layout->addLayout(btnRow);

    m_emptyLabel->hide();

    QObject::connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    QObject::connect(m_openBtn,   &QPushButton::clicked, this, &FormPickerDialog::onOpenClicked);
    QObject::connect(m_formList,  &QListWidget::itemSelectionChanged, this, [this]() {
        m_openBtn->setEnabled(m_formList->currentItem() != nullptr);
    });
    QObject::connect(m_formList,  &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *) {
        onOpenClicked();
    });

    QObject::connect(m_client, &PatClient::formCatalogReady,
                     this, &FormPickerDialog::onCatalogReady);

    m_client->fetchFormCatalog();

    if (touchMode) {
        QWidget *parentWin = parentWidget() ? parentWidget()->window() : nullptr;
        QRect bounds = parentWin ? parentWin->geometry()
                                 : QApplication::primaryScreen()->availableGeometry();
        const int margin = 32;
        setGeometry(bounds.adjusted(margin, margin, -margin, -margin));
    }
}

void FormPickerDialog::onCatalogReady(const QJsonObject &catalog)
{
    m_catalog = catalog;

    QStringList categories;
    for (const auto &v : catalog.value("folders").toArray())
        categories << v.toObject().value("name").toString();
    if (categories.isEmpty()) {
        m_categoryBtn->setText("(no categories — run Update Form Templates first)");
        m_categoryBtn->setEnabled(false);
        return;
    }

    auto *menu = new QMenu(m_categoryBtn);
    if (m_touchMode) menu->setStyleSheet(touchStyle::menuStyle);
    for (const QString &c : categories) {
        QAction *act = menu->addAction(c);
        QObject::connect(act, &QAction::triggered, this, [this, c]() { onCategoryChanged(c); });
    }
    QObject::connect(m_categoryBtn, &QPushButton::clicked, this, [this, menu]() {
        menu->setMinimumWidth(m_categoryBtn->width());
        menu->popup(m_categoryBtn->mapToGlobal(QPoint(0, m_categoryBtn->height())));
    });

    m_categoryBtn->setEnabled(true);
    onCategoryChanged(categories.first());
}

void FormPickerDialog::onCategoryChanged(const QString &category)
{
    m_currentCategory = category;
    m_categoryBtn->setText(category + "    ▾");
    populateForms(category);
}

void FormPickerDialog::collectForms(const QJsonObject &folder, const QString &pathPrefix,
                                    QJsonArray &out) const
{
    for (const auto &v : folder.value("forms").toArray()) {
        QJsonObject f = v.toObject();
        if (!pathPrefix.isEmpty()) f["__sub"] = pathPrefix;
        out.append(f);
    }
    for (const auto &v : folder.value("folders").toArray()) {
        QJsonObject sub = v.toObject();
        QString subName = sub.value("name").toString();
        QString newPrefix = pathPrefix.isEmpty() ? subName : pathPrefix + " / " + subName;
        collectForms(sub, newPrefix, out);
    }
}

void FormPickerDialog::populateForms(const QString &category)
{
    m_formList->clear();
    m_openBtn->setEnabled(false);

    QJsonObject target;
    for (const auto &v : m_catalog.value("folders").toArray()) {
        QJsonObject f = v.toObject();
        if (f.value("name").toString() == category) {
            target = f;
            break;
        }
    }
    if (target.isEmpty()) return;

    QJsonArray flat;
    collectForms(target, QString(), flat);

    for (const auto &v : flat) {
        QJsonObject f = v.toObject();
        QString name = f.value("name").toString();
        QString sub  = f.value("__sub").toString();
        QString label = sub.isEmpty() ? name : QString("%1   (%2)").arg(name, sub);

        auto *item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, f.value("template_path").toString());
        m_formList->addItem(item);
    }

    bool empty = (m_formList->count() == 0);
    m_emptyLabel->setVisible(empty);
    m_formList->setVisible(!empty);
}

void FormPickerDialog::onOpenClicked()
{
    auto *item = m_formList->currentItem();
    if (!item) return;
    QString templatePath = item->data(Qt::UserRole).toString();
    if (templatePath.isEmpty()) return;
    emit formSelected(templatePath);
    accept();
}
