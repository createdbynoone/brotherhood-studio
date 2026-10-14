#include "downloader.h"

#include "../core/err.h"

#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QUrl>

namespace Downloader {

void download(const QString &urlString, const QString &dest) {
    if (!urlString.startsWith("https://")) throw Err(QStringLiteral("Only HTTPS downloads are allowed"));
    QNetworkAccessManager nam;
    QNetworkRequest req{QUrl(urlString)};
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setTransferTimeout(120000);
    QNetworkReply *reply = nam.get(req);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() != QNetworkReply::NoError || (status && (status < 200 || status >= 300))) {
        const QString msg = status ? QStringLiteral("Download failed (HTTP %1)").arg(status) : reply->errorString();
        reply->deleteLater();
        throw Err(msg);
    }
    const QByteArray data = reply->readAll();
    reply->deleteLater();
    QSaveFile f(dest);
    if (!f.open(QIODevice::WriteOnly)) throw Err(QStringLiteral("No se pudo escribir %1").arg(QFileInfo(dest).fileName()));
    f.write(data);
    if (!f.commit()) throw Err(QStringLiteral("No se pudo guardar %1").arg(QFileInfo(dest).fileName()));
}

QString extensionOf(const QString &url, const QString &fallback) {
    const QString e = QFileInfo(QUrl(url).path()).suffix().toLower();
    return (e.isEmpty() || e.size() > 4) ? fallback : e;
}

}  // namespace Downloader
