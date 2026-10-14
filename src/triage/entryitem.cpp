#include "entryitem.h"

#include "triagetypes.h"

#include <QDateTime>
#include <QFileInfo>
#include <QJsonArray>
#include <QLocale>
#include <QUrl>

EntryItem::EntryItem(const QJsonObject &raw, QObject *parent) : QObject(parent), m_raw(raw) {
    m_path = raw["path"].toString();
    m_fingerprint = raw["fingerprint"].toString();
    m_status = qMax(0, Triage::statusFromName(raw["status"].toString()));
    m_rating = raw["rating"].toInt();
    for (const auto &c : raw["categories"].toArray()) m_categories << c.toString();
    m_note = raw["note"].toString();
    m_source = Triage::sourceFromName(raw["source"].toString());
    m_addedAt = raw["addedAt"].toDouble();
    m_updatedAt = raw["updatedAt"].toDouble();
    m_hasMissingKey = raw.contains("missing");
    m_missing = raw["missing"].toBool();
    m_isVideo = Triage::isVideo(m_path);
}

EntryItem *EntryItem::makeNew(const QString &path, const QString &fingerprint, int source) {
    const double now = double(QDateTime::currentMSecsSinceEpoch());
    QJsonObject o{{"path", path}, {"fingerprint", fingerprint}, {"status", "unsorted"}, {"rating", 0},
                  {"categories", QJsonArray()}, {"note", ""}, {"source", Triage::sourceName(source)},
                  {"addedAt", now}, {"updatedAt", now}, {"missing", false}};
    return new EntryItem(o);
}

QJsonObject EntryItem::toJson() const {
    QJsonObject o = m_raw;
    o["path"] = m_path;
    o["fingerprint"] = m_fingerprint;
    o["status"] = Triage::statusName(m_status);
    o["rating"] = m_rating;
    QJsonArray cats;
    for (const auto &c : m_categories) cats.append(c);
    o["categories"] = cats;
    o["note"] = m_note;
    o["source"] = Triage::sourceName(m_source);
    o["addedAt"] = m_addedAt;
    o["updatedAt"] = m_updatedAt;
    if (m_hasMissingKey || m_missing) o["missing"] = m_missing;
    return o;
}

QString EntryItem::filename() const { return QFileInfo(m_path).fileName(); }

QString EntryItem::addedLabel() const {
    const QLocale es(QLocale::Spanish, QLocale::Colombia);
    QString s = es.toString(QDateTime::fromMSecsSinceEpoch(qint64(m_addedAt)), "d MMM yyyy");
    return s.replace(".", "");
}

// image://thumb/<percent-encoded path>?<fingerprint>: the fingerprint is only there so the
// provider's disk cache and QML's pixmap cache invalidate when the file changes.
QString EntryItem::thumbSource() const {
    return QStringLiteral("image://thumb/") + QString::fromLatin1(QUrl::toPercentEncoding(m_path)) + "?" + m_fingerprint;
}

void EntryItem::setFingerprint(const QString &f) { if (m_fingerprint != f) { m_fingerprint = f; emit fingerprintChanged(); } }
void EntryItem::setStatus(int s) { if (m_status != s) { m_status = s; emit statusChanged(); } }
void EntryItem::setNote(const QString &n) { if (m_note != n) { m_note = n; emit noteChanged(); } }
void EntryItem::setMissing(bool m) { if (m_missing != m) { m_missing = m; emit missingChanged(); } }
void EntryItem::setIsNew(bool n) { if (m_isNew != n) { m_isNew = n; emit isNewChanged(); } }
void EntryItem::setSelected(bool s) { if (m_selected != s) { m_selected = s; emit selectedChanged(); } }
void EntryItem::setPrimary(bool p) { if (m_primary != p) { m_primary = p; emit primaryChanged(); } }
void EntryItem::setCategories(const QStringList &c) { if (m_categories != c) { m_categories = c; emit categoriesChanged(); } }
void EntryItem::setChips(const QStringList &names, int extra) {
    if (m_chips != names || m_chipExtra != extra) { m_chips = names; m_chipExtra = extra; emit chipsChanged(); }
}
void EntryItem::touchUpdated() { m_updatedAt = double(QDateTime::currentMSecsSinceEpoch()); }
void EntryItem::setPath(const QString &p) { m_path = p; m_isVideo = Triage::isVideo(p); }
