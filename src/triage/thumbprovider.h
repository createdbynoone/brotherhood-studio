#pragma once
#include <QQuickAsyncImageProvider>
#include <QThreadPool>

// image://thumb/<percent-encoded path>?<fingerprint>   400 px grid thumbnails (disk + GPU cached)
// image://full/<percent-encoded path>                  Focus view, long side capped at 4096 px
// Decoding happens on a private thread pool, never on the GUI thread.
class ThumbProvider : public QQuickAsyncImageProvider {
public:
    explicit ThumbProvider(bool full);
    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;

private:
    bool m_full;
    QThreadPool m_pool;
};
