#pragma once
#include <QHash>
#include <QMap>
#include <QObject>
#include <QTimer>

#include "entryitem.h"
#include "triagetypes.h"

// JSON-backed DB (same file and shape as the Electron and SwiftUI builds). Writes are debounced,
// serialized on the GUI thread (a snapshot) and written on a worker, and flushed synchronously on quit.
class Store : public QObject {
    Q_OBJECT
public:
    explicit Store(QObject *parent = nullptr);
    ~Store() override;

    void load();
    int revision() const { return m_revision; }

    const QHash<QString, EntryItem *> &entries() const { return m_entries; }
    EntryItem *entry(const QString &path) const { return m_entries.value(path); }
    const QMap<QString, Triage::Category> &categories() const { return m_categories; }

    /// Adds new paths, re-links moved files by fingerprint, flags missing desktop files. Returns the new entries.
    QList<EntryItem *> reconcile(const QStringList &diskPaths, int source);
    void touch();                       // every mutation ends here: bumps revision, debounces the write
    void remove(const QString &path);
    void purgeMissing();

    QString addCategory(const QString &name, const QString &parentId = {});   // returns id, empty on failure
    void renameCategory(const QString &id, const QString &name);
    void deleteCategory(const QString &id);

    void flush();
    void flushSync();

    static QString uniqueId();
    static QString fingerprint(const QString &path);
    static QStringList expand(const QStringList &paths);       // folders -> their media files
    static QString copyToLibrary(const QString &src);          // dedup by name + size

signals:
    void changed();

private:
    QJsonObject snapshot() const;
    static void write(const QJsonObject &db);

    QHash<QString, EntryItem *> m_entries;
    QMap<QString, Triage::Category> m_categories;
    QJsonObject m_rawRoot;              // keeps top-level keys we don't know
    int m_revision = 0;
    QTimer m_flushTimer;
};
