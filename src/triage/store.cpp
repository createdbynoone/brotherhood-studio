#include "store.h"

#include "../core/jsonfile.h"
#include "../core/paths.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QMutex>
#include <QMutexLocker>
#include <QThreadPool>
#include <QUuid>
#include <QtConcurrent>
#include <sys/stat.h>

static QMutex gWriteMutex;     // serializes disk writes so an older snapshot never lands after a newer one

Store::Store(QObject *parent) : QObject(parent) {
    m_flushTimer.setSingleShot(true);
    m_flushTimer.setInterval(250);
    connect(&m_flushTimer, &QTimer::timeout, this, &Store::flush);
}

Store::~Store() { qDeleteAll(m_entries); }

void Store::load() {
    qDeleteAll(m_entries);
    m_entries.clear();
    m_categories.clear();

    m_rawRoot = JsonFile::readObject(Paths::sorterDb());
    const QJsonObject ents = m_rawRoot["entries"].toObject();
    for (auto it = ents.begin(); it != ents.end(); ++it) {
        QJsonObject o = it.value().toObject();
        if (!o.contains("path")) o["path"] = it.key();
        auto *e = new EntryItem(o, this);
        m_entries.insert(e->path(), e);
    }
    const QJsonObject cats = m_rawRoot["categories"].toObject();
    for (auto it = cats.begin(); it != cats.end(); ++it) {
        const QJsonObject o = it.value().toObject();
        Triage::Category c;
        c.id = o["id"].toString(it.key());
        c.name = o["name"].toString();
        c.color = o["color"].toString();
        c.parentId = o["parentId"].toString();
        c.createdAt = o["createdAt"].toDouble();
        m_categories.insert(c.id, c);
    }
    ++m_revision;

    if (m_categories.isEmpty()) {
        static const QStringList defaults = {"Fisher Hats", "Caps", "Tees", "Hoodies", "Shorts", "Sweatpants", "Jackets", "Cultural Revelations", "Essentials"};
        for (const auto &name : defaults) {
            Triage::Category c;
            c.id = uniqueId();
            c.name = name;
            c.createdAt = double(QDateTime::currentMSecsSinceEpoch());
            m_categories.insert(c.id, c);
        }
        touch();
    }
}

QString Store::uniqueId() {
    return QStringLiteral("%1-%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(QUuid::createUuid().toString(QUuid::Id128).left(5));
}

// size:birthtime-ms, same recipe as the other builds so moved files re-link across all of them
QString Store::fingerprint(const QString &path) {
    struct stat st;
    if (::stat(path.toUtf8().constData(), &st) != 0) return {};
    const double birth = double(st.st_birthtimespec.tv_sec) + double(st.st_birthtimespec.tv_nsec) / 1e9;
    return QStringLiteral("%1:%2").arg(qint64(st.st_size)).arg(qint64(qRound64(birth * 1000)));
}

void Store::touch() {
    ++m_revision;
    m_flushTimer.start();
    emit changed();
}

QList<EntryItem *> Store::reconcile(const QStringList &diskPaths, int source) {
    QHash<QString, EntryItem *> byFingerprint;
    for (auto *e : std::as_const(m_entries)) if (!e->fingerprint().isEmpty()) byFingerprint.insert(e->fingerprint(), e);

    const QSet<QString> diskSet(diskPaths.begin(), diskPaths.end());
    QSet<QString> seen;
    QList<EntryItem *> added;

    for (const QString &path : diskPaths) {
        if (auto *e = m_entries.value(path)) {
            e->setFingerprint(fingerprint(path));
            e->setMissing(false);
            seen.insert(path);
            continue;
        }
        const QString fp = fingerprint(path);
        EntryItem *moved = fp.isEmpty() ? nullptr : byFingerprint.value(fp);
        if (moved && !diskSet.contains(moved->path())) {
            m_entries.remove(moved->path());
            moved->setPath(path);
            moved->setFingerprint(fp);
            moved->setMissing(false);
            m_entries.insert(path, moved);
            seen.insert(path);
        } else {
            auto *e = EntryItem::makeNew(path, fp, source);
            e->setParent(this);
            m_entries.insert(path, e);
            added << e;
            seen.insert(path);
        }
    }

    if (source == Triage::Desktop) {
        for (auto *e : std::as_const(m_entries))
            if (!seen.contains(e->path()) && e->source() == Triage::Desktop) e->setMissing(!QFileInfo::exists(e->path()));
    }
    touch();
    return added;
}

void Store::remove(const QString &path) {
    if (auto *e = m_entries.take(path)) e->deleteLater();
    touch();
}

void Store::purgeMissing() {
    const auto all = m_entries.values();
    for (auto *e : all) if (e->missing()) { m_entries.remove(e->path()); e->deleteLater(); }
    touch();
}

QString Store::addCategory(const QString &nameIn, const QString &parentId) {
    const QString name = nameIn.trimmed();
    if (name.isEmpty() || name.size() > 80) return {};
    if (!parentId.isEmpty() && !m_categories.contains(parentId)) return {};
    Triage::Category c;
    c.id = uniqueId();
    c.name = name;
    c.parentId = parentId;
    c.createdAt = double(QDateTime::currentMSecsSinceEpoch());
    m_categories.insert(c.id, c);
    touch();
    return c.id;
}

void Store::renameCategory(const QString &id, const QString &nameIn) {
    const QString name = nameIn.trimmed();
    if (name.isEmpty() || !m_categories.contains(id)) return;
    m_categories[id].name = name;
    touch();
}

void Store::deleteCategory(const QString &id) {
    if (!m_categories.contains(id)) return;
    QSet<QString> ids{id};
    for (const auto &c : std::as_const(m_categories)) if (c.parentId == id) ids.insert(c.id);
    for (const auto &k : ids) m_categories.remove(k);
    for (auto *e : std::as_const(m_entries)) {
        QStringList kept;
        for (const auto &c : e->categories()) if (!ids.contains(c)) kept << c;
        e->setCategories(kept);
    }
    touch();
}

QJsonObject Store::snapshot() const {
    QJsonObject ents;
    for (auto *e : m_entries) ents.insert(e->path(), e->toJson());
    QJsonObject cats;
    for (const auto &c : m_categories) {
        QJsonObject o{{"id", c.id}, {"name", c.name}, {"createdAt", c.createdAt}};
        if (!c.color.isEmpty()) o["color"] = c.color;
        if (!c.parentId.isEmpty()) o["parentId"] = c.parentId;
        cats.insert(c.id, o);
    }
    QJsonObject root = m_rawRoot;
    if (!root.contains("version")) root["version"] = 1;
    root["entries"] = ents;
    root["categories"] = cats;
    return root;
}

void Store::write(const QJsonObject &db) {
    QMutexLocker l(&gWriteMutex);
    JsonFile::writeObject(Paths::sorterDb(), db);
}

void Store::flush() {
    m_flushTimer.stop();
    const QJsonObject snap = snapshot();
    (void)QtConcurrent::run([snap] { write(snap); });
}

void Store::flushSync() {
    m_flushTimer.stop();
    const QJsonObject snap = snapshot();
    write(snap);
}

QStringList Store::expand(const QStringList &paths) {
    QStringList out;
    for (const QString &p : paths) {
        const QFileInfo fi(p);
        if (!fi.exists()) continue;
        if (fi.isDir()) {
            for (const QString &f : QDir(p).entryList(QDir::Files, QDir::Name))
                if (Triage::isMedia(f)) out << QDir(p).filePath(f);
        } else if (Triage::isMedia(p)) {
            out << p;
        }
    }
    return out;
}

QString Store::copyToLibrary(const QString &src) {
    const QDir dir(Paths::sorterLibrary());
    const QFileInfo fi(src);
    QString dest = dir.filePath(fi.fileName());
    if (QFileInfo::exists(dest)) {
        if (QFileInfo(dest).size() == fi.size()) return dest;
        int i = 1;
        while (QFileInfo::exists(dir.filePath(QStringLiteral("%1_%2.%3").arg(fi.completeBaseName()).arg(i).arg(fi.suffix())))) ++i;
        dest = dir.filePath(QStringLiteral("%1_%2.%3").arg(fi.completeBaseName()).arg(i).arg(fi.suffix()));
    }
    QFile::copy(src, dest);
    return dest;
}
