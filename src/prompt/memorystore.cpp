#include "memorystore.h"

#include "../core/jsonfile.h"
#include "../core/paths.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QLocale>
#include <QMutex>
#include <QMutexLocker>
#include <QUuid>
#include <algorithm>

namespace MemoryStore {

static QMutex mtx;

static QList<MemoryEntry> load() {
    QList<MemoryEntry> out;
    const QJsonArray arr = JsonFile::readObject(Paths::bmpMemory()).value("entries").toArray();
    for (const auto &v : arr) {
        const QJsonObject o = v.toObject();
        MemoryEntry e;
        e.id = o["id"].toString();
        e.timestamp = o["timestamp"].toDouble();
        e.description = o["description"].toString();
        e.prompt = o["prompt"].toString();
        e.fired = o["fired"].toBool();
        e.aspectRatio = o["aspectRatio"].toString();
        out.append(e);
    }
    return out;
}

static void save(const QList<MemoryEntry> &all) {
    QJsonArray arr;
    for (const auto &e : all) {
        QJsonObject o{{"id", e.id}, {"timestamp", e.timestamp}, {"description", e.description},
                      {"prompt", e.prompt}, {"fired", e.fired}};
        if (!e.aspectRatio.isEmpty()) o["aspectRatio"] = e.aspectRatio;
        arr.append(o);
    }
    JsonFile::writeObject(Paths::bmpMemory(), QJsonObject{{"entries", arr}});
}

QList<MemoryEntry> entries() {
    QMutexLocker l(&mtx);
    auto all = load();
    std::reverse(all.begin(), all.end());
    return all;
}

void stats(int *total, int *fired) {
    QMutexLocker l(&mtx);
    const auto all = load();
    *total = all.size();
    *fired = int(std::count_if(all.begin(), all.end(), [](const MemoryEntry &e) { return e.fired; }));
}

MemoryEntry add(const QString &description, const QString &prompt) {
    QMutexLocker l(&mtx);
    auto all = load();
    MemoryEntry e;
    e.timestamp = double(QDateTime::currentMSecsSinceEpoch());
    e.id = QStringLiteral("%1-%2").arg(qint64(e.timestamp)).arg(QUuid::createUuid().toString(QUuid::Id128).left(5));
    e.description = description;
    e.prompt = prompt;
    all.append(e);
    if (all.size() > 200) all = all.mid(all.size() - 200);
    save(all);
    return e;
}

void markFired(const QString &id, const QString &aspectRatio) {
    QMutexLocker l(&mtx);
    auto all = load();
    for (auto &e : all) {
        if (e.id == id) { e.fired = true; e.aspectRatio = aspectRatio; save(all); return; }
    }
}

QString context() {
    QList<MemoryEntry> all;
    { QMutexLocker l(&mtx); all = load(); }
    if (all.isEmpty()) return {};

    QList<MemoryEntry> fired, recent;
    for (const auto &e : all) (e.fired ? fired : recent).append(e);
    QList<MemoryEntry> pool = fired.mid(qMax<qsizetype>(0, fired.size() - 8)) + recent.mid(qMax<qsizetype>(0, recent.size() - 5));
    if (pool.isEmpty()) return {};
    std::sort(pool.begin(), pool.end(), [](const MemoryEntry &a, const MemoryEntry &b) { return a.timestamp < b.timestamp; });

    const QLocale es(QLocale::Spanish, QLocale::Colombia);
    QStringList blocks;
    for (const auto &e : pool) {
        QString date = es.toString(QDateTime::fromMSecsSinceEpoch(qint64(e.timestamp)), "d MMM");
        if (date.endsWith('.')) date.chop(1);
        const QString label = e.fired ? QStringLiteral("★ FIRED") : QStringLiteral("○ generated");
        blocks << QStringLiteral("[%1 · %2]\nBrief: \"%3\"\nPrompt:\n%4").arg(label, date, e.description, e.prompt);
    }
    return QStringLiteral(
               "\n\n## PROMPT MEMORY — %1 past Brotherhood prompts (★ = approved & fired to Higgsfield)\n"
               "Study these to calibrate vocabulary, light descriptions, garment detail depth, color language, and brand tone. "
               "Fired prompts are your strongest signal — replicate what makes them work.\n\n%2\n\n---\n"
               "Apply learnings silently. Output ONLY the new prompt.")
        .arg(pool.size())
        .arg(blocks.join("\n\n---\n\n"));
}

}  // namespace MemoryStore
