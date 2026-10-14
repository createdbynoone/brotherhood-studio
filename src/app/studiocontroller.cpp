#include "studiocontroller.h"

#include "../core/lock.h"
#include "../core/paths.h"
#include "../core/prefs.h"
#include "../prompt/shell.h"
#include "../platform/mac.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QGuiApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QWindow>
#include <QtConcurrent>

static const char *kRepo = "createdbynoone/brotherhood-studio";

StudioController::StudioController(QObject *parent) : QObject(parent) {
    m_layout = Prefs::studioLayout();
    if (m_layout != "prompt" && m_layout != "triage") m_layout = "split";
    m_split = qBound(0.25, Prefs::studioSplitFraction(), 0.75);
    m_active = m_layout == "triage" ? "triage" : "prompt";
    m_unlocked = AppLock::isRemembered();
    m_reduceMotion = Mac::reduceMotion();
    m_nam = new QNetworkAccessManager(this);
}

QString StudioController::version() const { return QCoreApplication::applicationVersion(); }
double StudioController::lockUntil() const { return AppLock::lockUntil(); }

void StudioController::setLayout(const QString &l) {
    if (l == m_layout || (l != "prompt" && l != "split" && l != "triage")) return;
    m_layout = l;
    if (l == "prompt") setActivePane("prompt"); else if (l == "triage") setActivePane("triage");
    Prefs::setStudioLayout(l);
    emit layoutChanged();
}

void StudioController::setSplitFraction(double f) {
    f = qBound(0.25, f, 0.75);
    if (qFuzzyCompare(f, m_split)) return;
    m_split = f;
    emit splitFractionChanged();
}
void StudioController::saveSplit() { Prefs::setStudioSplitFraction(m_split); }

void StudioController::setActivePane(const QString &p) {
    if (p == m_active || (p != "prompt" && p != "triage")) return;
    m_active = p;
    emit activePaneChanged();
}

// ⌃Tab: move the active pane (opening the split first if only one pane is visible)
void StudioController::cyclePane() {
    if (m_layout == "split") { setActivePane(m_active == "prompt" ? "triage" : "prompt"); return; }
    const QString next = m_layout == "prompt" ? "triage" : "prompt";
    setLayout("split");
    setActivePane(next);
}

void StudioController::verifyKey(const QString &key) {
    if (key.isEmpty()) return;
    (void)QtConcurrent::run([this, key] {
        const bool ok = AppLock::verify(key);       // ~100 ms of PBKDF2: off the GUI thread
        QMetaObject::invokeMethod(this, [this, ok] {
            double until = 0;
            if (ok) { AppLock::clearFailures(); m_unlocked = true; emit unlockedChanged(); }
            else { until = AppLock::registerFailure(); emit lockChanged(); }
            emit keyResult(ok, until);
        }, Qt::QueuedConnection);
    });
}

void StudioController::forgetKey() { AppLock::forget(); emit toast(QStringLiteral("The key will be asked again on next launch"), 0); }

// ── Updates ──────────────────────────────────────────────────────────────
static QList<int> versionParts(QString v) {
    v.remove(QRegularExpression("^[vV]"));
    QList<int> out;
    for (const auto &p : v.split('.')) out << p.toInt();
    return out;
}
static bool isNewer(const QString &remote, const QString &local) {
    const auto r = versionParts(remote), l = versionParts(local);
    for (int i = 0; i < qMax(r.size(), l.size()); ++i) {
        const int a = i < r.size() ? r[i] : 0, b = i < l.size() ? l[i] : 0;
        if (a != b) return a > b;
    }
    return false;
}

void StudioController::checkForUpdate() {
    m_checking = true; emit updateChanged();
    // BS_UPDATE_API lets a test point the check at a local server (see CLAUDE.md); production uses GitHub.
    const QString api = qEnvironmentVariable("BS_UPDATE_API", QStringLiteral("https://api.github.com/repos/%1/releases/latest").arg(kRepo));
    QNetworkRequest req{QUrl(api)};
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setTransferTimeout(8000);
    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        m_checking = false;
        if (reply->error() == QNetworkReply::NoError) {
            const auto o = QJsonDocument::fromJson(reply->readAll()).object();
            const QString tag = o["tag_name"].toString();
            if (!tag.isEmpty() && isNewer(tag, version())) {
                m_updateVersion = QString(tag).remove(QRegularExpression("^[vV]"));
                m_updateUrl = QUrl(o["html_url"].toString());
                m_updateAsset = QUrl();
                for (const auto &a : o["assets"].toArray()) {       // Brotherhood-Studio-X.Y.Z-arm64-mac.zip
                    const QString name = a.toObject()["name"].toString();
                    if (name.endsWith("-mac.zip")) { m_updateAsset = QUrl(a.toObject()["browser_download_url"].toString()); break; }
                }
            }
        }
        emit updateChanged();
    });
}

// Downloads the release zip, then hands over to a small script that waits for this process to exit,
// swaps the .app (keeping the old one until the new one is in place), clears quarantine and relaunches.
void StudioController::installUpdate() {
    if (m_updating) return;
    // .../Brotherhood Studio.app/Contents/MacOS -> bundle path
    QDir d(QCoreApplication::applicationDirPath());
    d.cdUp(); d.cdUp();
    const QString bundle = d.absolutePath();
    if (m_updateAsset.isEmpty() || !bundle.endsWith(".app") || !QFileInfo(QFileInfo(bundle).absolutePath()).isWritable()) {
        if (m_updateUrl.isValid()) QDesktopServices::openUrl(m_updateUrl);     // fall back to the release page
        return;
    }
    m_updating = true; m_progress = 0; m_updateError.clear(); emit updatingChanged();

    const QString zip = QDir::tempPath() + "/brotherhood-studio-update.zip";
    QFile::remove(zip);
    QNetworkRequest req(m_updateAsset);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 got, qint64 total) {
        if (total > 0) { m_progress = double(got) / double(total); emit updatingChanged(); }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, zip, bundle] {
        reply->deleteLater();
        auto fail = [this](const QString &msg) { m_updating = false; m_updateError = msg; emit updatingChanged(); emit toast(msg, 2); };
        if (reply->error() != QNetworkReply::NoError) { fail(QStringLiteral("Update download failed")); return; }
        QFile f(zip);
        if (!f.open(QIODevice::WriteOnly) || f.write(reply->readAll()) < 1) { fail(QStringLiteral("Could not save the update")); return; }
        f.close();

        const QString script = QDir::tempPath() + "/brotherhood-studio-update.sh";
        QFile s(script);
        if (!s.open(QIODevice::WriteOnly)) { fail(QStringLiteral("Could not prepare the update")); return; }
        s.write(R"SH(#!/bin/bash
PID="$1"; ZIP="$2"; DEST="$3"
while kill -0 "$PID" 2>/dev/null; do sleep 0.2; done
TMP="$(mktemp -d)"
/usr/bin/ditto -x -k "$ZIP" "$TMP" || exit 1
NEW="$(ls -d "$TMP"/*.app | head -1)"
[ -d "$NEW" ] || exit 1
/usr/bin/xattr -cr "$NEW"
rm -rf "$DEST.old"
mv "$DEST" "$DEST.old" || exit 1
if mv "$NEW" "$DEST"; then rm -rf "$DEST.old"; else mv "$DEST.old" "$DEST"; fi
rm -rf "$TMP" "$ZIP"
open "$DEST"
)SH");
        s.close();
        QFile::setPermissions(script, QFile::permissions(script) | QFileDevice::ExeOwner);
        QProcess::startDetached("/bin/bash", {script, QString::number(QCoreApplication::applicationPid()), zip, bundle});
        QCoreApplication::quit();
    });
}

void StudioController::dismissUpdate() { m_updateVersion.clear(); m_updateUrl.clear(); emit updateChanged(); }

// ── OS helpers ───────────────────────────────────────────────────────────
void StudioController::copyText(const QString &text) { QGuiApplication::clipboard()->setText(text); }
void StudioController::openUrl(const QUrl &url) { QDesktopServices::openUrl(url); }
void StudioController::openFolder(const QString &path) { QDesktopServices::openUrl(QUrl::fromLocalFile(path)); }

void StudioController::zoomWindow() {
    const auto windows = QGuiApplication::topLevelWindows();
    if (!windows.isEmpty()) Mac::zoomWindow(windows.first());
}

QString StudioController::abbreviate(const QString &path) const {
    const QString home = QDir::homePath();
    return path.startsWith(home) ? "~" + path.mid(home.size()) : path;
}

QString StudioController::toolPath(const QString &bin) const { return Shell::resolve(bin); }
QString StudioController::dbPath() const { return Paths::sorterDb(); }
QString StudioController::memoryPath() const { return Paths::bmpMemory(); }
QString StudioController::libraryPath() const { return Paths::sorterLibrary(); }
void StudioController::revealFile(const QString &path) { Mac::revealInFinder(path); }
void StudioController::clearThumbnails() {
    QDir(Paths::sorterThumbs()).removeRecursively();
    emit toast(QStringLiteral("Thumbnail cache cleared"), 1);
}
