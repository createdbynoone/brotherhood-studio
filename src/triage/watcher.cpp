#include "watcher.h"

#include "triagetypes.h"

#include <QDir>
#include <QFileInfo>
#include <QPointer>
#include <QTimer>

FolderWatcher::FolderWatcher(const QString &path, QObject *parent) : QObject(parent), m_path(path) {
    const auto l = listing();
    m_known = QSet<QString>(l.begin(), l.end());
    m_watcher.addPath(m_path);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { scan(); });
}

QStringList FolderWatcher::listing() const {
    QStringList out;
    for (const QString &n : QDir(m_path).entryList(QDir::Files)) if (Triage::isBmpOutput(n)) out << n;
    return out;
}

void FolderWatcher::stop() {
    if (!m_watcher.directories().isEmpty()) m_watcher.removePaths(m_watcher.directories());
}

void FolderWatcher::scan() {
    const auto l = listing();
    const QSet<QString> now(l.begin(), l.end());
    const QSet<QString> addedNames = now - m_known;
    const QSet<QString> removedNames = m_known - now;
    m_known = now;
    if (!removedNames.isEmpty()) {
        QStringList gone;
        for (const auto &n : removedNames) gone << QDir(m_path).filePath(n);
        emit removed(gone);
    }
    for (const auto &n : addedNames) waitStable(QDir(m_path).filePath(n), -1, 0);
}

void FolderWatcher::waitStable(const QString &full, qint64 prev, int checks) {
    QPointer<FolderWatcher> self(this);
    QTimer::singleShot(300, this, [self, full, prev, checks] {
        if (!self) return;
        const qint64 size = QFileInfo(full).exists() ? QFileInfo(full).size() : -1;
        if (size > 0 && size == prev) emit self->added({full});
        else if (checks < 12) self->waitStable(full, size, checks + 1);
    });
}
