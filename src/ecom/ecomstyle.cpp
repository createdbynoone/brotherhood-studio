#include "ecomstyle.h"

#include "../core/paths.h"
#include "../platform/mac.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <algorithm>
#include <cstdlib>

namespace EcomStyle {

QString dir() { return Paths::studioDir() + "/ecom-style"; }

void seed() {
    const QString root = QStringLiteral(":/assets/ecom-style/");
    QDirIterator it(root, {"*.jpg"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString src = it.next();
        const QString dest = dir() + "/" + src.mid(root.size());
        if (QFileInfo::exists(dest)) continue;
        QDir().mkpath(QFileInfo(dest).absolutePath());
        if (QFile::copy(src, dest)) QFile::setPermissions(dest, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
    }
    QFile readme(dir() + "/README.txt");
    if (!readme.exists() && readme.open(QIODevice::WriteOnly)) {
        readme.write("Brotherhood house-style library (used by the Ecommerce pane).\n\n"
                     "Put finished catalog photos here as <category>/<name>__<front|back>.jpg\n"
                     "(category folders: tee, hoodie, pants, shorts, headwear, or a new one).\n"
                     "New photos are measured and used the next time you press Analyze.\n");
    }
}

namespace {

struct Entry { QString path, category, view; double l = 0, t = 0, w = 0, h = 0; bool ok = false; };

// Bounding box of everything that is not the corner (background) color, as fractions of the canvas
Entry measure(const QString &path) {
    Entry e;
    e.path = path;
    e.category = QFileInfo(path).dir().dirName();
    const QString base = QFileInfo(path).completeBaseName();
    const int at = base.lastIndexOf("__");
    e.view = at >= 0 ? base.mid(at + 2) : QStringLiteral("front");
    QImage img = Mac::downsample(path, 360).convertToFormat(QImage::Format_RGB32);
    if (img.isNull()) return e;
    const int W = img.width(), H = img.height();
    long r = 0, g = 0, b = 0, n = 0;
    for (int y : {0, H - 6}) for (int x : {0, W - 6})
        for (int j = 0; j < 6; ++j) for (int i = 0; i < 6; ++i) { const QRgb c = img.pixel(x + i, y + j); r += qRed(c); g += qGreen(c); b += qBlue(c); ++n; }
    r /= n; g /= n; b /= n;
    int minX = W, maxX = -1, minY = H, maxY = -1;
    for (int y = 0; y < H; ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(img.constScanLine(y));
        for (int x = 0; x < W; ++x) {
            const int d = std::abs(qRed(line[x]) - int(r)) + std::abs(qGreen(line[x]) - int(g)) + std::abs(qBlue(line[x]) - int(b));
            if (d > 40) { minX = qMin(minX, x); maxX = qMax(maxX, x); minY = qMin(minY, y); maxY = qMax(maxY, y); }
        }
    }
    if (maxX < 0) return e;
    e.l = double(minX) / W; e.t = double(minY) / H; e.w = double(maxX + 1 - minX) / W; e.h = double(maxY + 1 - minY) / H;
    e.ok = true;
    return e;
}

QStringList files() {
    QStringList out;
    QDirIterator it(dir(), {"*.jpg", "*.jpeg", "*.png", "*.webp"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) out << it.next();
    std::sort(out.begin(), out.end());
    return out;
}

}  // namespace

int count() { return int(files().size()); }

QString index() {
    QStringList lines;
    for (const QString &f : files()) {
        const Entry e = measure(f);
        if (!e.ok) continue;
        lines << QStringLiteral("%1 | category %2 | view %3 | frame left %4 top %5 width %6 height %7")
                     .arg(e.path, e.category, e.view).arg(e.l, 0, 'f', 2).arg(e.t, 0, 'f', 2).arg(e.w, 0, 'f', 2).arg(e.h, 0, 'f', 2);
    }
    return lines.join('\n');
}

QString resolve(const QString &relativeOrAbsolute) {
    if (relativeOrAbsolute.trimmed().isEmpty()) return {};
    const QString root = QDir(dir()).canonicalPath();
    const QString candidate = QFileInfo(QDir::isAbsolutePath(relativeOrAbsolute) ? relativeOrAbsolute : dir() + "/" + relativeOrAbsolute).canonicalFilePath();
    if (root.isEmpty() || candidate.isEmpty() || !candidate.startsWith(root + "/")) return {};
    return candidate;
}

}  // namespace EcomStyle
