#include "thumbprovider.h"

#include "../core/paths.h"
#include "../platform/mac.h"
#include "triagetypes.h"

#include <QFile>
#include <QImage>
#include <QQuickImageResponse>
#include <QQuickTextureFactory>
#include <QRunnable>
#include <QThread>
#include <QUrl>
#include <atomic>

namespace {

constexpr int kThumbSide = 400;
constexpr int kFullSide = 4096;

QString cacheFile(const QString &path, const QString &fingerprint) {
    const QString key = fingerprint.isEmpty() ? path : fingerprint;
    const QString hash = QString::fromLatin1(Mac::sha256(key.toUtf8()).left(10).toHex());
    return Paths::sorterThumbs() + "/" + hash + ".jpg";
}

class Response : public QQuickImageResponse, public QRunnable {
public:
    Response(QString path, QString fingerprint, bool full) : m_path(std::move(path)), m_fp(std::move(fingerprint)), m_full(full) {
        setAutoDelete(false);
    }

    QQuickTextureFactory *textureFactory() const override { return QQuickTextureFactory::textureFactoryForImage(m_image); }
    void cancel() override { m_cancelled = true; }

    void run() override {
        if (!m_cancelled) m_image = m_full ? loadFull() : loadThumb();
        emit finished();
    }

private:
    QImage loadFull() const { return Mac::downsample(m_path, kFullSide); }

    QImage loadThumb() const {
        const QString cache = cacheFile(m_path, m_fp);
        if (QFile::exists(cache)) {
            QImage cached(cache);
            if (!cached.isNull()) return cached;
        }
        if (m_cancelled) return {};
        QImage img = Triage::isVideo(m_path) ? Mac::videoFrame(m_path, kThumbSide) : Mac::downsample(m_path, kThumbSide);
        if (!img.isNull()) img.save(cache, "JPG", 72);
        return img;
    }

    QString m_path, m_fp;
    bool m_full;
    QImage m_image;
    std::atomic<bool> m_cancelled{false};
};

}  // namespace

ThumbProvider::ThumbProvider(bool full) : m_full(full) {
    m_pool.setMaxThreadCount(qMax(2, QThread::idealThreadCount() - 2));
}

QQuickImageResponse *ThumbProvider::requestImageResponse(const QString &id, const QSize &) {
    const int q = id.indexOf('?');
    const QString enc = q < 0 ? id : id.left(q);
    const QString fp = q < 0 ? QString() : id.mid(q + 1);
    auto *r = new Response(QUrl::fromPercentEncoding(enc.toUtf8()), fp, m_full);
    m_pool.start(r);
    return r;
}
