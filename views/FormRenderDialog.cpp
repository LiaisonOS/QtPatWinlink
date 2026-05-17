//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Modal dialog rendering a Pat form HTML — implementation.
//

#include "FormRenderDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QUuid>
#include <QUrl>
#include <QUrlQuery>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QNetworkCookie>
#include <QApplication>
#include <QScreen>
#include <QMessageBox>
#include <QInputDialog>

namespace {

static const QString s_dialogStyle =
    "QMessageBox { background: #1e1e1e; }"
    "QMessageBox QLabel { color: #e0e0e0; font-size: 13pt; min-width: 320px; }"
    "QMessageBox QPushButton {"
    "  background: #2d2d2d; color: #e0e0e0; border: 1px solid #404040;"
    "  border-radius: 6px; padding: 8px 22px; min-width: 80px; font-size: 12pt;"
    "}"
    "QMessageBox QPushButton:hover { background: #444444; }"
    "QMessageBox QPushButton:default { background: #ffa500; color: #000000; font-weight: bold; }"
    "QInputDialog { background: #1e1e1e; color: #e0e0e0; }"
    "QInputDialog QLabel { color: #e0e0e0; font-size: 13pt; }"
    "QInputDialog QLineEdit {"
    "  background: #2d2d2d; color: #e0e0e0; border: 1px solid #404040;"
    "  border-radius: 4px; padding: 6px; font-size: 12pt;"
    "}";

// QWebEnginePage subclass — routes JS dialogs (alert/confirm/prompt) through
// Qt's QMessageBox/QInputDialog so they respect our dark theme instead of
// WebEngine's default black-on-black dialog.
class StyledWebPage : public QWebEnginePage
{
public:
    StyledWebPage(QWebEngineProfile *profile, QWidget *parent)
        : QWebEnginePage(profile, parent), m_parent(parent) {}

protected:
    void javaScriptAlert(const QUrl &, const QString &msg) override {
        QMessageBox box(QMessageBox::Information, "Pat Forms", msg,
                        QMessageBox::Ok, m_parent);
        box.setStyleSheet(s_dialogStyle);
        box.exec();
    }

    bool javaScriptConfirm(const QUrl &, const QString &msg) override {
        QMessageBox box(QMessageBox::Question, "Confirm", msg,
                        QMessageBox::Yes | QMessageBox::No, m_parent);
        box.setStyleSheet(s_dialogStyle);
        return box.exec() == QMessageBox::Yes;
    }

    bool javaScriptPrompt(const QUrl &, const QString &msg,
                          const QString &defaultValue, QString *result) override {
        QInputDialog dlg(m_parent);
        dlg.setStyleSheet(s_dialogStyle);
        dlg.setWindowTitle("Input");
        dlg.setLabelText(msg);
        dlg.setTextValue(defaultValue);
        bool ok = (dlg.exec() == QDialog::Accepted);
        if (ok && result) *result = dlg.textValue();
        return ok;
    }

private:
    QWidget *m_parent;
};
}  // namespace

FormRenderDialog::FormRenderDialog(PatClient *client, const QString &templatePath,
                                   bool touchMode, QWidget *parent)
    : QDialog(parent)
    , m_client(client)
    , m_touchMode(touchMode)
{
    setWindowTitle("Form — " + templatePath);
    setStyleSheet("QDialog { background: #121212; }");

    // Generate a per-session form instance key
    m_formKey = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // Set the cookie on the PatClient QNAM (so subsequent GET /api/form retrieves the right entry)
    m_client->setFormInstanceCookie(m_formKey);

    // Set the cookie on the WebEngine profile (so the form's POST submit carries it)
    QUrl baseUrl(m_client->baseUrl());
    QString host = baseUrl.host();
    if (host.isEmpty()) host = "localhost";

    QNetworkCookie cookie("forminstance", m_formKey.toUtf8());
    cookie.setDomain(host);
    cookie.setPath("/");
    QWebEngineProfile::defaultProfile()->cookieStore()->setCookie(cookie);

    m_view = new QWebEngineView(this);
    auto *page = new StyledWebPage(QWebEngineProfile::defaultProfile(), m_view);
    m_view->setPage(page);

    auto *cancelBtn = new QPushButton("Cancel");
    QString btnExtra = touchMode
        ? "font-size: 18px; font-weight: bold; min-height: 58px; padding: 0 26px;"
        : "padding: 6px 20px;";
    int btnRadius = touchMode ? 6 : 4;
    cancelBtn->setStyleSheet(QString(
        "QPushButton { background: #333333; color: #ffffff; border: none;"
        "  border-radius: %1px; %2 }"
        "QPushButton:hover { background: #444444; }"
    ).arg(btnRadius).arg(btnExtra));

    auto *footer = new QWidget();
    footer->setStyleSheet("background: #1a1a1a; border-top: 1px solid #2a2a2a;");
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(12, 8, 12, 8);
    auto *hint = new QLabel("Fill the form and click Submit — the composer will populate automatically.");
    hint->setStyleSheet("color: #888888; font-size: 10pt;");
    footerLayout->addWidget(hint);
    footerLayout->addStretch();
    footerLayout->addWidget(cancelBtn);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_view, 1);
    layout->addWidget(footer);

    QObject::connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    QObject::connect(m_view->page(), &QWebEnginePage::windowCloseRequested, this, [this]() {
        emit formSubmitted(m_formKey);
        accept();
    });
    // Fallback: if the form does a traditional POST navigation, the URL will
    // change to /api/form. That's also a successful-submit signal.
    QObject::connect(m_view->page(), &QWebEnginePage::urlChanged, this,
        [this](const QUrl &url) {
            if (url.path() == "/api/form") {
                emit formSubmitted(m_formKey);
                accept();
            }
        });

    // Build the form URL and load it.
    // /api/forms (plural) returns the *fillable HTML form* via GetFormTemplateHandler.
    // /api/form GET retrieves submitted form data; /api/form POST is the submit target.
    // /api/template returns the raw .txt for the text-only editor.
    QUrl formUrl(m_client->baseUrl() + "/api/forms");
    QUrlQuery q;
    q.addQueryItem("template", templatePath);
    formUrl.setQuery(q);
    m_view->load(formUrl);

    // Sizing
    if (touchMode) {
        QWidget *parentWin = parentWidget() ? parentWidget()->window() : nullptr;
        QRect bounds = parentWin ? parentWin->geometry()
                                 : QApplication::primaryScreen()->availableGeometry();
        const int margin = 16;
        setGeometry(bounds.adjusted(margin, margin, -margin, -margin));
    } else {
        resize(900, 700);
    }
}
