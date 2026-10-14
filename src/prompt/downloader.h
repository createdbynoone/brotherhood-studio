#pragma once
#include <QString>

namespace Downloader {
/// HTTPS only. Blocking (local event loop): call from worker threads. Throws Err.
void download(const QString &url, const QString &dest);
/// File extension from a result URL, falling back when it isn't a short clean one.
QString extensionOf(const QString &url, const QString &fallback);
}
