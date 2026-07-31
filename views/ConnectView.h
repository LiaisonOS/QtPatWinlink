//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Connect view — RMS station list with band/modem filter
//

#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "../PatClient.h"
#include "../RmsPrefs.h"
#include "SessionConsole.h"

class ConnectView : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectView(PatClient *client, bool touchMode, QWidget *parent = nullptr);
    void refresh();
    void setDefaultFilters(const QString &band, const QString &modem);

signals:
    void done();

private slots:
    void onRmsListReady(const QJsonArray &stations);
    void onConnectClicked();
    void applyFilters();
    void forceRefresh();
    void onSearchChanged(const QString &text);
    void openManualDialog();                     // Add
    void editManualStation(int row);             // Edit (right-click / long-press)
    void deleteManualStation(int row);           // Delete (right-click / long-press)

private:
    // Merge operator-entered manual stations from RmsPrefs into m_allStations
    // as JSON objects shaped like Pat's RMS entries — so onSearchChanged
    // renders them identically to online RMS rows (filters, favorites, star
    // toggle all just work). Called at the end of onRmsListReady and any
    // time the manual list changes.
    void reloadWithManualMerged();

    PatClient    *m_client;
    RmsPrefs      m_prefs;
    bool          m_touchMode;
    QJsonArray    m_allStations;
    QJsonArray    m_lastRmsFetch;   // last online RMS list, kept so manual-only
                                    // reloads don't need to re-hit Pat

    QPushButton  *m_bandBtn;
    QPushButton  *m_modemBtn;
    QPushButton  *m_favOnlyBtn;
    QLineEdit    *m_searchFilter;
    QPushButton  *m_applyBtn;
    QPushButton  *m_refreshBtn;
    QPushButton  *m_manualBtn;
    QTableWidget *m_table;
    QPushButton  *m_connectBtn;
    QPushButton  *m_cancelBtn;
    QLabel       *m_statusLabel;

    QString       m_band;     // empty = all bands
    QString       m_modem;    // empty = all modems
};
