#include "paths.h"

#include <QDir>
#include <QStandardPaths>

namespace Paths {

static QString ensure(const QString &dir) {
    QDir().mkpath(dir);
    return dir;
}

QString appSupport() { return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation); }
QString bmpDir() { static const QString d = ensure(appSupport() + "/BMP"); return d; }
QString bmpPrefs() { return bmpDir() + "/bmp-prefs.json"; }
QString bmpMemory() { return bmpDir() + "/bmp-memory.json"; }
QString higgsfieldCredentials() { return QDir::homePath() + "/.config/higgsfield/credentials.json"; }
QString defaultOutput() { return QDir::homePath() + "/Desktop"; }
QString sorterDir() { static const QString d = ensure(appSupport() + "/Sorter"); return d; }
QString sorterDb() { return sorterDir() + "/sorter-db.json"; }
QString sorterPrefs() { return sorterDir() + "/sorter-prefs.json"; }
QString sorterLibrary() { return ensure(sorterDir() + "/library"); }
QString sorterThumbs() { return ensure(sorterDir() + "/thumbs-qt"); }
QString studioDir() { static const QString d = ensure(appSupport() + "/BrotherhoodStudioQt"); return d; }
QString studioPrefs() { return studioDir() + "/studio-prefs.json"; }

}  // namespace Paths
