//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : App-private state for QtPatWinlink — implementation.
//

#include "RmsPrefs.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

RmsPrefs::RmsPrefs()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    m_path = base + "/liaisonos/qtpatwinlink/qtpatwinlink.json";
    load();
}

QString RmsPrefs::keyFor(const QString &cs, const QString &freq, const QString &modem)
{
    return cs.trimmed().toUpper() + "|" + freq.trimmed() + "|" + modem.trimmed();
}

void RmsPrefs::ensureDir() const
{
    QDir().mkpath(QFileInfo(m_path).absolutePath());
}

void RmsPrefs::load()
{
    QFile f(m_path);
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return;

    QJsonObject root = doc.object();

    QJsonObject last = root.value("lastSelected").toObject();
    m_last.callsign = last.value("callsign").toString();
    m_last.freq     = last.value("freq").toString();
    m_last.modem    = last.value("modem").toString();

    m_favKeys.clear();
    m_favLines.clear();
    for (const auto &v : root.value("favorites").toArray()) {
        QJsonObject o = v.toObject();
        QString cs    = o.value("callsign").toString();
        QString freq  = o.value("freq").toString();
        QString modem = o.value("modem").toString();
        if (cs.isEmpty()) continue;
        QString k = keyFor(cs, freq, modem);
        if (!m_favKeys.contains(k)) {
            m_favKeys.insert(k);
            m_favLines.append(k);
        }
    }

    m_favoritesOnly = root.value("favoritesOnly").toBool(false);

    m_manual.clear();
    for (const auto &v : root.value("manualStations").toArray()) {
        QJsonObject o = v.toObject();
        ManualStation s;
        s.callsign = o.value("callsign").toString();
        s.freq_hz  = o.value("freq_hz").toDouble();
        s.bw_hz    = o.value("bw_hz").toInt(2300);
        s.modem    = o.value("modem").toString();
        s.band     = o.value("band").toString();
        s.notes    = o.value("notes").toString();
        if (!s.callsign.isEmpty() && s.freq_hz > 0.0)
            m_manual.append(s);
    }
}

void RmsPrefs::save() const
{
    ensureDir();
    QJsonObject root;

    QJsonObject last;
    last["callsign"] = m_last.callsign;
    last["freq"]     = m_last.freq;
    last["modem"]    = m_last.modem;
    root["lastSelected"] = last;

    QJsonArray favs;
    for (const QString &line : m_favLines) {
        QStringList parts = line.split("|");
        if (parts.size() < 3) continue;
        QJsonObject o;
        o["callsign"] = parts[0];
        o["freq"]     = parts[1];
        o["modem"]    = parts[2];
        favs.append(o);
    }
    root["favorites"] = favs;
    root["favoritesOnly"] = m_favoritesOnly;

    QJsonArray man;
    for (const ManualStation &s : m_manual) {
        QJsonObject o;
        o["callsign"] = s.callsign;
        o["freq_hz"]  = s.freq_hz;
        o["bw_hz"]    = s.bw_hz;
        o["modem"]    = s.modem;
        o["band"]     = s.band;
        o["notes"]    = s.notes;
        man.append(o);
    }
    root["manualStations"] = man;

    QFile f(m_path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
}

void RmsPrefs::setLastSelected(const Selection &s)
{
    if (m_last.callsign == s.callsign && m_last.freq == s.freq && m_last.modem == s.modem)
        return;
    m_last = s;
    save();
}

bool RmsPrefs::isFavorite(const QString &callsign, const QString &freq, const QString &modem) const
{
    return m_favKeys.contains(keyFor(callsign, freq, modem));
}

void RmsPrefs::toggleFavorite(const QString &callsign, const QString &freq, const QString &modem)
{
    QString k = keyFor(callsign, freq, modem);
    if (m_favKeys.contains(k)) {
        m_favKeys.remove(k);
        m_favLines.removeAll(k);
    } else {
        m_favKeys.insert(k);
        m_favLines.append(k);
    }
    save();
}

void RmsPrefs::setFavoritesOnly(bool on)
{
    if (m_favoritesOnly == on) return;
    m_favoritesOnly = on;
    save();
}

void RmsPrefs::addOrUpdateManual(const ManualStation &s)
{
    if (s.callsign.trimmed().isEmpty() || s.freq_hz <= 0.0) return;
    // Replace if same callsign+freq+modem — lets operator edit an entry
    // just by re-submitting the dialog with the same key.
    for (int i = 0; i < m_manual.size(); ++i) {
        const auto &e = m_manual[i];
        if (e.callsign.trimmed().toUpper() == s.callsign.trimmed().toUpper()
            && qAbs(e.freq_hz - s.freq_hz) < 1.0
            && e.modem == s.modem) {
            m_manual[i] = s;
            save();
            return;
        }
    }
    m_manual.append(s);
    save();
}

void RmsPrefs::removeManual(const QString &callsign, double freq_hz, const QString &modem)
{
    for (int i = m_manual.size() - 1; i >= 0; --i) {
        const auto &e = m_manual[i];
        if (e.callsign.trimmed().toUpper() == callsign.trimmed().toUpper()
            && qAbs(e.freq_hz - freq_hz) < 1.0
            && e.modem == modem) {
            m_manual.removeAt(i);
        }
    }
    save();
}
