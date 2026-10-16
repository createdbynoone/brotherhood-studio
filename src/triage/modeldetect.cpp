#include "modeldetect.h"

#include <QFile>

namespace ModelDetect {

static quint32 be32(const uchar *p) { return quint32(p[0]) << 24 | quint32(p[1]) << 16 | quint32(p[2]) << 8 | p[3]; }

// Concatenated metadata blocks of a PNG (every chunk but the pixel data) or a JPEG (APPn / COM segments before the scan)
static QByteArray metadata(QFile &f) {
    QByteArray out;
    const QByteArray head = f.read(8);
    constexpr int kCap = 6 * 1024 * 1024;
    if (head.startsWith("\x89PNG\r\n\x1a\n")) {
        for (;;) {
            const QByteArray h = f.read(8);
            if (h.size() < 8) break;
            const quint32 len = be32(reinterpret_cast<const uchar *>(h.constData()));
            const QByteArray type = h.mid(4, 4);
            if (type == "IEND") break;
            if (type == "IDAT" || len > quint32(kCap)) { if (!f.seek(f.pos() + len + 4)) break; continue; }
            out += f.read(len);
            f.seek(f.pos() + 4);                                  // crc
            if (out.size() > kCap) break;
        }
    } else if (head.startsWith("\xff\xd8")) {
        f.seek(2);
        for (;;) {
            const QByteArray m = f.read(2);
            if (m.size() < 2 || uchar(m[0]) != 0xFF) break;
            const uchar marker = uchar(m[1]);
            if (marker == 0xDA || marker == 0xD9) break;           // scan data / end: no more metadata
            const QByteArray l = f.read(2);
            if (l.size() < 2) break;
            const int len = (uchar(l[0]) << 8 | uchar(l[1])) - 2;
            if (len < 0) break;
            if ((marker >= 0xE0 && marker <= 0xEF) || marker == 0xFE) out += f.read(len); else f.seek(f.pos() + len);
            if (out.size() > kCap) break;
        }
    }
    return out;
}

QString fromMetadata(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    const QByteArray meta = metadata(f).toLower();
    if (meta.isEmpty()) return {};
    if (meta.contains("seedream")) return QStringLiteral("Seedream 5.0 Pro");
    if (meta.contains("openai") || meta.contains("chatgpt") || meta.contains("gpt-image") || meta.contains("gpt_image"))
        return QStringLiteral("GPT Image 2.5");
    if (meta.contains("gemini") || meta.contains("synthid") || meta.contains("google")) return QStringLiteral("Nano Banana Pro");
    return {};
}

}  // namespace ModelDetect
