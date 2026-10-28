#include "prefs.h"

#include "jsonfile.h"
#include "paths.h"

#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>

namespace Prefs {

static QMutex mtx;

static QJsonValue get(const QString &file, const QString &key) { QMutexLocker l(&mtx); return JsonFile::readObject(file).value(key); }
static void set(const QString &file, const QString &key, const QJsonValue &v) {
    QMutexLocker l(&mtx);
    QJsonObject o = JsonFile::readObject(file);
    o[key] = v;
    JsonFile::writeObject(file, o);
}

QString outputPath() {
    const QString p = get(Paths::bmpPrefs(), "outputPath").toString().trimmed();
    return p.isEmpty() ? Paths::defaultOutput() : p;
}
void setOutputPath(const QString &path) { set(Paths::bmpPrefs(), "outputPath", path); }
int authFailCount() { return get(Paths::bmpPrefs(), "authFailCount").toInt(0); }
double authLockUntil() { return get(Paths::bmpPrefs(), "authLockUntil").toDouble(0); }
void setAuth(int failCount, double lockUntil) {
    QMutexLocker l(&mtx);
    QJsonObject o = JsonFile::readObject(Paths::bmpPrefs());
    o["authFailCount"] = failCount;
    o["authLockUntil"] = lockUntil;
    JsonFile::writeObject(Paths::bmpPrefs(), o);
}

double sorterGridSize(double fallback) { const auto v = get(Paths::sorterPrefs(), "gridSize"); return v.isDouble() ? v.toDouble() : fallback; }
bool sorterInspectorOpen(bool fallback) { const auto v = get(Paths::sorterPrefs(), "inspectorOpen"); return v.isBool() ? v.toBool() : fallback; }
void setSorterGridSize(double v) { set(Paths::sorterPrefs(), "gridSize", v); }
void setSorterInspectorOpen(bool v) { set(Paths::sorterPrefs(), "inspectorOpen", v); }

QString studioLayout() { const QString s = get(Paths::studioPrefs(), "layout").toString(); return s.isEmpty() ? "split" : s; }
double studioSplitFraction() { const auto v = get(Paths::studioPrefs(), "splitFraction"); return v.isDouble() ? v.toDouble() : 0.44; }
void setStudioLayout(const QString &layout) { set(Paths::studioPrefs(), "layout", layout); }
void setStudioSplitFraction(double v) { set(Paths::studioPrefs(), "splitFraction", v); }
QVariantMap studioCamera() { return get(Paths::studioPrefs(), "camera").toObject().toVariantMap(); }
void setStudioCamera(const QVariantMap &camera) { set(Paths::studioPrefs(), "camera", QJsonObject::fromVariantMap(camera)); }
int studioUiScale() { const int v = get(Paths::studioPrefs(), "uiScale").toInt(100); return qBound(50, v, 100); }
void setStudioUiScale(int percent) { set(Paths::studioPrefs(), "uiScale", qBound(50, percent, 100)); }

}  // namespace Prefs
