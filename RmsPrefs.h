//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : App-private state for QtPatWinlink — last selected RMS, favorites,
//           filter toggles. Stored in ~/.config/emcomm-tools/qtpatwinlink/qtpatwinlink.json.
//

#pragma once

#include <QString>
#include <QSet>
#include <QList>

class RmsPrefs
{
public:
    struct Selection {
        QString callsign;
        QString freq;
        QString modem;
        bool    isValid() const { return !callsign.isEmpty(); }
    };

    // Operator-entered station — used when Winlink's RMS list can't be
    // downloaded (no internet) or when we need to reach an unlisted BBS.
    // freq_hz is the tuning (dial) frequency; bw_hz is the modem bandwidth
    // used to build the "varahf:///CALL?freq=X&bw=Y" connect URL that pat
    // expects (see main.cpp:51 for the reference format).
    struct ManualStation {
        QString callsign;      // e.g. "VE2CJR-10"
        double  freq_hz = 0.0; // dial freq
        int     bw_hz   = 2300;
        QString modem;         // "varahf" | "varafm" | "winmor" | "pactor" | "packet"
        QString band;          // "40m" (derived at entry time, kept for filtering)
        QString notes;         // free text
    };

    RmsPrefs();

    void load();
    void save() const;

    Selection lastSelected() const { return m_last; }
    void setLastSelected(const Selection &s);

    bool isFavorite(const QString &callsign, const QString &freq, const QString &modem) const;
    void toggleFavorite(const QString &callsign, const QString &freq, const QString &modem);

    bool favoritesOnly() const { return m_favoritesOnly; }
    void setFavoritesOnly(bool on);

    // Manual-station CRUD. Uniqueness key = callsign+freq+modem (same shape
    // as favorites so a station can be both manual and favorited).
    const QList<ManualStation> &manualStations() const { return m_manual; }
    void addOrUpdateManual(const ManualStation &s);
    void removeManual(const QString &callsign, double freq_hz, const QString &modem);

private:
    static QString keyFor(const QString &cs, const QString &freq, const QString &modem);
    void ensureDir() const;

    QString              m_path;
    Selection            m_last;
    QSet<QString>        m_favKeys;     // for O(1) lookup
    QStringList          m_favLines;    // serialized "callsign|freq|modem" preserving order
    bool                 m_favoritesOnly = false;
    QList<ManualStation> m_manual;
};
