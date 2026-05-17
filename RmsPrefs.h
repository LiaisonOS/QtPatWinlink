//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : App-private state for QtPatWinlink — last selected RMS, favorites,
//           filter toggles. Stored in ~/.config/emcomm-tools/qtpatwinlink/qtpatwinlink.json.
//

#pragma once

#include <QString>
#include <QSet>

class RmsPrefs
{
public:
    struct Selection {
        QString callsign;
        QString freq;
        QString modem;
        bool    isValid() const { return !callsign.isEmpty(); }
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

private:
    static QString keyFor(const QString &cs, const QString &freq, const QString &modem);
    void ensureDir() const;

    QString       m_path;
    Selection     m_last;
    QSet<QString> m_favKeys;     // for O(1) lookup
    QStringList   m_favLines;    // serialized "callsign|freq|modem" preserving order
    bool          m_favoritesOnly = false;
};
