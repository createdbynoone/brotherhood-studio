#include "staging.h"

#include "../core/err.h"
#include "../platform/mac.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QSet>
#include <QStandardPaths>

namespace Staging {

static const int kMaxEdge = 2560;
static QMutex mtx;
static int counter = 0;

static QString root() { return QDir::tempPath() + "/bmp-staged-refs-qt"; }

void reset() {
    QMutexLocker l(&mtx);
    QDir(root()).removeRecursively();
    QDir().mkpath(root());
    counter = 0;
}

QStringList stage(const QStringList &originals) {
    static const QSet<QString> passthrough = {"jpg", "jpeg", "png", "webp", "gif"};
    QStringList out;
    for (const QString &original : originals) {
        if (!QFileInfo::exists(original))
            throw Err(QStringLiteral("File not found: %1").arg(QFileInfo(original).fileName()));
        QString ext = QFileInfo(original).suffix().toLower();
        if (ext.isEmpty()) ext = "jpg";
        int n;
        { QMutexLocker l(&mtx); n = ++counter; QDir().mkpath(root()); }

        const auto props = Mac::imageProps(original);
        const int longEdge = qMax(props.w, props.h);
        if (props.ok && (!passthrough.contains(ext) || longEdge > kMaxEdge || (props.orientation != 1 && ext != "png"))) {
            const QString staged = QStringLiteral("%1/image%2.jpg").arg(root()).arg(n);
            if (Mac::writeNormalizedJpeg(original, staged, kMaxEdge, 0.92)) { out << staged; continue; }
        }
        const QString staged = QStringLiteral("%1/image%2.%3").arg(root()).arg(n).arg(ext);
        QFile::remove(staged);
        if (!QFile::copy(original, staged)) throw Err(QStringLiteral("No se pudo copiar %1").arg(QFileInfo(original).fileName()));
        out << staged;
    }
    return out;
}

}  // namespace Staging
