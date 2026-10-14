#pragma once
#include <QFileSystemWatcher>
#include <QObject>
#include <QSet>
#include <QStringList>

// Watches BMP's output folder for new/removed `bmp_*` files. Waits for the file size to settle
// before reporting an addition (generation downloads write in place).
class FolderWatcher : public QObject {
    Q_OBJECT
public:
    explicit FolderWatcher(const QString &path, QObject *parent = nullptr);
    void stop();

signals:
    void added(const QStringList &paths);
    void removed(const QStringList &paths);

private:
    void scan();
    void waitStable(const QString &full, qint64 prev, int checks);
    QStringList listing() const;

    QString m_path;
    QFileSystemWatcher m_watcher;
    QSet<QString> m_known;
    QSet<QString> m_pending;
};
