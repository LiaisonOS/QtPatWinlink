//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Connect view — RMS station list with band/modem filter
//

#include "ConnectView.h"
#include "../TouchStyle.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QScroller>
#include <QMenu>
#include <QAction>

namespace {
// Table item that sorts by numeric value while displaying custom text.
class NumItem : public QTableWidgetItem {
public:
    NumItem(const QString &text, double v) : QTableWidgetItem(text), m_val(v) {}
    bool operator<(const QTableWidgetItem &other) const override {
        if (auto *o = dynamic_cast<const NumItem *>(&other))
            return m_val < o->m_val;
        return QTableWidgetItem::operator<(other);
    }
private:
    double m_val;
};
}

ConnectView::ConnectView(PatClient *client, bool touchMode, QWidget *parent)
    : QWidget(parent)
    , m_client(client)
    , m_touchMode(touchMode)
{
    int fontSize = touchMode ? 13 : 10;
    int rowH     = touchMode ? 48 : 28;

    QString baseStyle = QString(
        "QLineEdit, QLabel { color: #e0e0e0; font-size: %1pt; }"
        "QLineEdit { background: #1e1e1e; border: 1px solid #333333; border-radius: 4px; padding: 4px 8px; }"
    ).arg(fontSize);
    setStyleSheet(baseStyle);

    QStringList bands  = {"All Bands", "160m", "80m", "60m", "40m", "30m",
                          "20m", "17m", "15m", "12m", "10m", "6m", "2m", "70cm"};
    QStringList modems = {"All Modems", "varahf", "varafm", "winmor", "pactor", "packet"};

    QString filterBtnStyle = QString(
        "QPushButton { background: #1e1e1e; color: #e0e0e0; border: 1px solid #404040;"
        "  border-radius: %1px; %2 text-align: left; }"
        "QPushButton:hover { background: #2a2a2a; }"
    ).arg(touchMode ? 6 : 4)
     .arg(touchMode ? "font-size: 16px; min-height: 58px; padding-left: 20px; padding-right: 20px;"
                    : QString("font-size: %1pt; min-height: 30px; padding-left: 14px; padding-right: 14px;").arg(fontSize));

    // Menu styling — light text on dark bg, orange highlight with BLACK text
    // on hover. Without this, desktop mode falls back to Qt defaults which
    // can render black-on-black on dark themes.
    QString filterMenuStyle = touchMode ? touchStyle::menuStyle : QString(
        "QMenu { background: #1e1e1e; color: #e0e0e0;"
        "  border: 1px solid #404040; padding: 4px; }"
        "QMenu::item { padding: 6px 18px; color: #e0e0e0; }"
        "QMenu::item:selected { background: #ffa500; color: #000000; }"
    );

    auto setBtnText = [](QPushButton *btn, const QString &label, const QString &value) {
        btn->setText(label + ": " + value + "    ▾");
    };

    auto makeFilterMenu = [filterMenuStyle, setBtnText](QPushButton *btn, const QStringList &options,
                                                        QString &valueRef, const QString &label,
                                                        std::function<void()> onChange) {
        auto *menu = new QMenu(btn);
        menu->setStyleSheet(filterMenuStyle);
        for (const QString &opt : options) {
            QAction *act = menu->addAction(opt);
            QObject::connect(act, &QAction::triggered, btn,
                [btn, opt, &valueRef, label, onChange, setBtnText]() {
                bool isAll = opt.startsWith("All");
                valueRef = isAll ? QString() : opt;
                setBtnText(btn, label, isAll ? "All" : opt);
                onChange();
            });
        }
        QObject::connect(btn, &QPushButton::clicked, btn, [btn, menu]() {
            menu->setMinimumWidth(btn->width());
            menu->popup(btn->mapToGlobal(QPoint(0, btn->height())));
        });
    };

    m_bandBtn  = new QPushButton("Band: All    ▾");
    m_modemBtn = new QPushButton("Modem: All    ▾");
    m_bandBtn->setStyleSheet(filterBtnStyle);
    m_modemBtn->setStyleSheet(filterBtnStyle);

    makeFilterMenu(m_bandBtn,  bands,  m_band,  "Band",  [this]() { applyFilters(); });
    makeFilterMenu(m_modemBtn, modems, m_modem, "Modem", [this]() { applyFilters(); });

    m_searchFilter = new QLineEdit();
    m_searchFilter->setPlaceholderText("Search callsign...");
    if (touchMode) m_searchFilter->setMinimumHeight(58);

    m_favOnlyBtn = new QPushButton("★ Favorites");
    m_favOnlyBtn->setCheckable(true);
    m_favOnlyBtn->setChecked(m_prefs.favoritesOnly());
    m_favOnlyBtn->setMinimumWidth(touchMode ? 200 : 130);
    m_favOnlyBtn->setStyleSheet(QString(
        "QPushButton { background: #1e1e1e; color: #e0e0e0; border: 1px solid #404040;"
        "  border-radius: %1px; %2 }"
        "QPushButton:hover { background: #2a2a2a; }"
        "QPushButton:checked { background: #ffa500; color: #000000; font-weight: bold; border-color: #ffa500; }"
    ).arg(touchMode ? 6 : 4)
     .arg(touchMode ? "font-size: 16px; min-height: 58px; padding: 0 24px;"
                    : QString("font-size: %1pt; padding: 5px 16px;").arg(fontSize)));
    QObject::connect(m_favOnlyBtn, &QPushButton::toggled, this, [this](bool on) {
        m_prefs.setFavoritesOnly(on);
        onSearchChanged(m_searchFilter->text());
    });

    auto *filterRow = new QHBoxLayout();
    filterRow->setSpacing(8);
    filterRow->addWidget(m_bandBtn);
    filterRow->addWidget(m_modemBtn);
    filterRow->addWidget(m_favOnlyBtn);
    filterRow->addWidget(m_searchFilter, 1);

    QString touchExtra = touchMode
        ? QString("font-size: 18px; min-height: 58px; padding: 0 22px;")
        : QString("padding: 5px 14px;");
    int btnRadius = touchMode ? 6 : 4;

    m_applyBtn = new QPushButton("Apply");
    m_applyBtn->setStyleSheet(QString(
        "QPushButton { background: #ffa500; color: #000000; font-weight: bold; border: none; border-radius: %1px; %2 }"
        "QPushButton:hover { background: #ffb733; }"
    ).arg(btnRadius).arg(touchExtra));
    m_refreshBtn = new QPushButton("↻ Update");
    m_refreshBtn->setStyleSheet(QString(
        "QPushButton { background: #2d2d2d; color: #e0e0e0; border: 1px solid #404040; border-radius: %1px; %2 }"
        "QPushButton:hover { background: #444444; }"
        "QPushButton:disabled { background: #1a1a1a; color: #666666; }"
    ).arg(btnRadius).arg(touchExtra));
    filterRow->addWidget(m_applyBtn);
    filterRow->addWidget(m_refreshBtn);

    m_table = new QTableWidget(0, 6, this);
    m_table->setHorizontalHeaderLabels({"Callsign", "Freq (MHz)", "Modem", "Dist (km)", "Quality %", "★"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
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
    m_table->setSortingEnabled(true);

    m_connectBtn = new QPushButton("Connect");
    m_cancelBtn  = new QPushButton("Cancel");
    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("color: #aaaaaa; font-size: 10pt;");

    QString actionExtra = touchMode
        ? QString("font-size: 18px; font-weight: bold; min-height: 58px; padding: 0 26px;")
        : QString("padding: 6px 20px;");

    m_connectBtn->setStyleSheet(QString(
        "QPushButton { background: #2f9e44; color: white; border: none; border-radius: %1px; %2 }"
        "QPushButton:hover { background: #37b24d; }"
        "QPushButton:disabled { background: #333333; color: #666666; }"
    ).arg(btnRadius).arg(actionExtra));
    m_cancelBtn->setStyleSheet(QString(
        "QPushButton { background: #333333; color: white; border: none; border-radius: %1px; %2 }"
        "QPushButton:hover { background: #444444; }"
    ).arg(btnRadius).arg(actionExtra));

    auto *btnRow = new QHBoxLayout();
    btnRow->addWidget(m_statusLabel);
    btnRow->addStretch();
    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_connectBtn);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(10);
    layout->addLayout(filterRow);
    layout->addWidget(m_table);
    layout->addLayout(btnRow);

    QObject::connect(m_client, &PatClient::rmsListReady, this, &ConnectView::onRmsListReady);
    QObject::connect(m_connectBtn, &QPushButton::clicked, this, &ConnectView::onConnectClicked);
    QObject::connect(m_cancelBtn,  &QPushButton::clicked, this, &ConnectView::done);
    QObject::connect(m_applyBtn,   &QPushButton::clicked, this, &ConnectView::applyFilters);
    QObject::connect(m_refreshBtn, &QPushButton::clicked, this, &ConnectView::forceRefresh);
    QObject::connect(m_searchFilter, &QLineEdit::textChanged, this, &ConnectView::onSearchChanged);

    m_connectBtn->setEnabled(false);
    QObject::connect(m_table, &QTableWidget::itemSelectionChanged, this, [this]() {
        int row = m_table->currentRow();
        m_connectBtn->setEnabled(row >= 0);
        if (row < 0) return;
        auto *cs    = m_table->item(row, 0);
        auto *freq  = m_table->item(row, 1);
        auto *modem = m_table->item(row, 2);
        if (cs && freq && modem)
            m_prefs.setLastSelected({cs->text(), freq->text(), modem->text()});
    });

    QObject::connect(m_table, &QTableWidget::cellClicked, this, [this](int row, int col) {
        if (col != 5) return;
        auto *cs    = m_table->item(row, 0);
        auto *freq  = m_table->item(row, 1);
        auto *modem = m_table->item(row, 2);
        auto *star  = m_table->item(row, 5);
        if (!cs || !freq || !modem || !star) return;
        m_prefs.toggleFavorite(cs->text(), freq->text(), modem->text());
        bool isFav = m_prefs.isFavorite(cs->text(), freq->text(), modem->text());
        star->setText(isFav ? "★" : "☆");
        star->setForeground(QColor(isFav ? "#ffa500" : "#666666"));
    });

    // Double-click any non-star cell = Connect to that station.
    QObject::connect(m_table, &QTableWidget::cellDoubleClicked, this,
        [this](int row, int col) {
            if (col == 5) return;   // star column has its own click handler
            m_table->setCurrentCell(row, 0);
            onConnectClicked();
        });

    if (touchMode) {
        m_table->setSelectionMode(QAbstractItemView::SingleSelection);
        QScroller::grabGesture(m_table->viewport(), QScroller::LeftMouseButtonGesture);
    }
}

void ConnectView::setDefaultFilters(const QString &band, const QString &modem)
{
    if (!band.isEmpty()) {
        m_band = band;
        m_bandBtn->setText("Band: " + band + "    ▾");
    }
    if (!modem.isEmpty()) {
        m_modem = modem;
        m_modemBtn->setText("Modem: " + modem + "    ▾");
    }
}

void ConnectView::refresh()
{
    applyFilters();
}

void ConnectView::applyFilters()
{
    m_statusLabel->setText("Loading...");
    m_client->fetchRmsList(m_band, m_modem, false);
}

void ConnectView::forceRefresh()
{
    m_statusLabel->setText("Downloading fresh list...");
    m_refreshBtn->setEnabled(false);
    m_client->fetchRmsList(m_band, m_modem, true);
}

void ConnectView::onRmsListReady(const QJsonArray &stations)
{
    m_allStations = stations;
    m_refreshBtn->setEnabled(true);
    onSearchChanged(m_searchFilter->text());
}

void ConnectView::onSearchChanged(const QString &text)
{
    QString search = text.trimmed().toUpper();
    bool favOnly = m_prefs.favoritesOnly();

    m_table->setSortingEnabled(false);
    m_table->setRowCount(0);

    for (const auto &val : m_allStations) {
        QJsonObject s = val.toObject();
        QString callsign = s.value("callsign").toString().toUpper();
        if (!search.isEmpty() && !callsign.contains(search)) continue;

        // Quick freq/modem peek for fav check
        QJsonObject dialObjPeek = s.value("dial").toObject();
        QString freqPeek = dialObjPeek.value("desc").toString();
        if (freqPeek.isEmpty()) {
            double hz = dialObjPeek.value("hz").toString().toDouble();
            freqPeek = QString::number(hz / 1e6, 'f', 3) + " MHz";
        }
        QString modemPeek = s.value("modes").toString();
        if (favOnly && !m_prefs.isFavorite(callsign, freqPeek, modemPeek)) continue;

        int row = m_table->rowCount();
        m_table->insertRow(row);

        // dial is the tuning frequency {hz, khz, desc}
        QJsonObject dialObj = s.value("dial").toObject();
        double hz = dialObj.value("hz").toString().toDouble();
        QString freqStr = dialObj.value("desc").toString();
        if (freqStr.isEmpty())
            freqStr = QString::number(hz / 1e6, 'f', 3) + " MHz";

        double dist = s.value("distance").toDouble();
        QString distStr = dist > 0 ? QString::number(dist, 'f', 1) : "";

        // Prediction link quality
        int quality = -1;
        if (!s.value("prediction").isNull() && s.value("prediction").isObject())
            quality = s.value("prediction").toObject().value("link_quality").toInt(-1);
        QString qualStr = quality >= 0 ? QString::number(quality) + " %" : "";

        auto *callItem = new QTableWidgetItem(callsign);
        callItem->setData(Qt::UserRole,     s.value("url").toString());   // connect URL
        callItem->setData(Qt::UserRole + 1, s.value("modes").toString());

        m_table->setItem(row, 0, callItem);
        m_table->setItem(row, 1, new NumItem(freqStr, hz));
        m_table->setItem(row, 2, new QTableWidgetItem(s.value("modes").toString()));
        m_table->setItem(row, 3, new NumItem(distStr, dist));
        auto *qualItem = new NumItem(qualStr, quality);
        qualItem->setTextAlignment(Qt::AlignCenter);
        if (quality >= 70)       qualItem->setForeground(QColor("#51cf66"));
        else if (quality >= 40)  qualItem->setForeground(QColor("#fcc419"));
        else if (quality >= 0)   qualItem->setForeground(QColor("#ff6b6b"));
        m_table->setItem(row, 4, qualItem);

        bool isFav = m_prefs.isFavorite(callsign, freqStr, s.value("modes").toString());
        auto *starItem = new QTableWidgetItem(isFav ? "★" : "☆");
        starItem->setTextAlignment(Qt::AlignCenter);
        starItem->setForeground(QColor(isFav ? "#ffa500" : "#666666"));
        m_table->setItem(row, 5, starItem);
    }

    m_statusLabel->setText(QString("%1 stations").arg(m_table->rowCount()));

    m_table->setSortingEnabled(true);
    m_table->sortItems(4, Qt::DescendingOrder);

    // Restore last selected row if still in the list
    auto last = m_prefs.lastSelected();
    if (last.isValid()) {
        for (int r = 0; r < m_table->rowCount(); ++r) {
            auto *cs    = m_table->item(r, 0);
            auto *freq  = m_table->item(r, 1);
            auto *modem = m_table->item(r, 2);
            if (cs && freq && modem &&
                cs->text()    == last.callsign &&
                freq->text()  == last.freq &&
                modem->text() == last.modem) {
                m_table->setCurrentCell(r, 0);
                m_table->scrollToItem(cs);
                break;
            }
        }
    }
}

void ConnectView::onConnectClicked()
{
    int row = m_table->currentRow();
    if (row < 0) {
        m_statusLabel->setText("Select a station first.");
        return;
    }

    auto *callItem = m_table->item(row, 0);
    QString connectUrl = callItem->data(Qt::UserRole).toString();

    if (connectUrl.isEmpty()) {
        m_statusLabel->setText("No connect URL for this station.");
        return;
    }

    m_connectBtn->setEnabled(false);
    m_client->connect(connectUrl);

    // Open session console — it subscribes to WS events and shows live protocol exchange
    auto *console = new SessionConsole(m_client, m_touchMode, this->parentWidget());
    console->setAttribute(Qt::WA_DeleteOnClose);
    QObject::connect(console, &SessionConsole::sessionDone, this,
        [this](bool wasConnected) {
            m_connectBtn->setEnabled(true);
            // Only navigate away (to Inbox) if a connection was actually
            // established. On failed connects, stay on RMS view so the
            // operator can try another station.
            if (wasConnected) emit done();
        });
    // Belt-and-suspenders: always re-enable the Connect button when the
    // dialog closes (including X-out before sessionDone has fired).
    QObject::connect(console, &QDialog::finished, this, [this](int) {
        m_connectBtn->setEnabled(true);
    });
    console->show();
}
