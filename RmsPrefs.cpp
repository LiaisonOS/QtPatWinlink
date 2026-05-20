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
