#pragma once
#include <QByteArray>
#include <QImage>
#include <QString>
#include <functional>

class QWindow;

// Thin wrappers over Apple frameworks (CommonCrypto, ImageIO, AVFoundation,
// AppKit). Everything above this layer stays portable C++/Qt.
namespace Mac {

// ── Crypto ────────────────────────────────────────────────────
QString machineId();
bool pbkdf2Sha512(const QByteArray &pass, const QByteArray &salt, quint32 iterations, int keyLen, QByteArray *out);
QByteArray hmacSha256(const QByteArray &key, const QByteArray &data);
QByteArray sha256(const QByteArray &data);

// ── Images / video ───────────────────────────────────────────────────────
struct ImageProps { bool ok = false; int w = 0; int h = 0; int orientation = 1; };
ImageProps imageProps(const QString &path);
/// Re-encodes as JPEG with EXIF rotation baked in, long side <= maxEdge.
bool writeNormalizedJpeg(const QString &src, const QString &dst, int maxEdge, double quality);
/// Decoded bitmap, long side <= maxPixel, orientation applied. Null on failure.
QImage downsample(const QString &path, int maxPixel);
QImage videoFrame(const QString &path, int maxSide);
/// "W x H" for images, "W x H . m:ss" for video (empty when unreadable).
QString mediaInfo(const QString &path);
/// Cover-crop (never letterbox) composite as JPEG data.
QByteArray composeCover(const QString &path, int w, int h, double quality);

// ── Files ────────────────────────────────────────────────────────────────
bool trashItem(const QString &path);       // volume-correct Trash; true when the file is gone
void revealInFinder(const QString &path);

// ── Window ───────────────────────────────────────────────────────────────
void styleWindow(QWindow *w);              // transparent title bar, content under it, dark appearance
void zoomWindow(QWindow *w);
/// Custom title bar: a mouse-down for which `isDragPoint(x, y)` (window coordinates, top-left origin) is true
/// starts a native window drag (double click zooms). The click is taken before Qt sees it.
void installTitleBarDrag(QWindow *w, std::function<bool(double, double)> isDragPoint);
void setDarkAppearance();
bool reduceMotion();                        // System Settings > Accessibility > Display > Reduce motion

}  // namespace Mac
