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

private:
    PatClient    *m_client;
    RmsPrefs      m_prefs;
    bool          m_touchMode;
    QJsonArray    m_allStations;

    QPushButton  *m_bandBtn;
    QPushButton  *m_modemBtn;
    QPushButton  *m_favOnlyBtn;
    QLineEdit    *m_searchFilter;
    QPushButton  *m_applyBtn;
    QPushButton  *m_refreshBtn;
    QTableWidget *m_table;
    QPushButton  *m_connectBtn;
    QPushButton  *m_cancelBtn;
    QLabel       *m_statusLabel;

    QString       m_band;     // empty = all bands
    QString       m_modem;    // empty = all modems
};
