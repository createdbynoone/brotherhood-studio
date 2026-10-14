#pragma once
#include <QSet>
#include <QString>
#include <QStringList>

namespace Triage {

enum Status { Unsorted = 0, Keep, Maybe, Discard, Archived };

QString statusName(int s);            // "keep" ...
int statusFromName(const QString &n); // -1 if unknown
int statusSortOrder(int s);           // keep, maybe, unsorted, discard, archived

enum Source { Desktop = 0, Folder, Drop };
QString sourceName(int s);
int sourceFromName(const QString &n);

enum Filter { All = 0, FUnsorted, FKeep, FMaybe, FDiscard, FArchived };   // same order as the UI strip
enum SortKey { Newest = 0, Oldest, ByStatus, ByName };

bool isVideo(const QString &path);
bool isMedia(const QString &path);
/// Files BMP writes: bmp_*.{jpg,jpeg,png,webp,mp4,mov,webm}
bool isBmpOutput(const QString &name);

struct Category {
    QString id;
    QString name;
    QString color;
    QString parentId;      // empty = top-level category
    double createdAt = 0;
    bool isParent() const { return parentId.isEmpty(); }
};

}  // namespace Triage
