#include "triagetypes.h"

#include <QFileInfo>

namespace Triage {

static const QStringList kStatusNames = {"unsorted", "keep", "maybe", "discard", "archived"};

QString statusName(int s) { return kStatusNames.value(s, "unsorted"); }
int statusFromName(const QString &n) { return kStatusNames.indexOf(n); }
int statusSortOrder(int s) {
    switch (s) { case Keep: return 0; case Maybe: return 1; case Unsorted: return 2; case Discard: return 3; default: return 4; }
}

QString sourceName(int s) { return s == Folder ? "folder" : s == Drop ? "drop" : "desktop"; }
int sourceFromName(const QString &n) { return n == "folder" ? Folder : n == "drop" ? Drop : Desktop; }

static QString ext(const QString &p) { return QFileInfo(p).suffix().toLower(); }
bool isVideo(const QString &p) { static const QSet<QString> v = {"mp4", "mov", "webm", "m4v"}; return v.contains(ext(p)); }
bool isMedia(const QString &p) {
    static const QSet<QString> i = {"jpg", "jpeg", "png", "webp", "gif"};
    return isVideo(p) || i.contains(ext(p));
}
bool isBmpOutput(const QString &name) {
    static const QSet<QString> e = {"jpg", "jpeg", "png", "webp", "mp4", "mov", "webm"};
    return name.toLower().startsWith("bmp_") && e.contains(ext(name));
}

}  // namespace Triage
