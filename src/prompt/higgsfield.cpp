#include "higgsfield.h"

#include "../core/err.h"
#include "../core/paths.h"
#include "shell.h"

#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>

namespace Higgsfield {

static const QString kBin = QStringLiteral("higgsfield");

static QJsonDocument json(const QStringList &args, int timeoutMs = -1) {
    const auto r = Shell::run(kBin, args + QStringList{"--json"}, timeoutMs);
    QJsonParseError pe;
    const auto doc = QJsonDocument::fromJson(r.out.toUtf8(), &pe);
    if (pe.error != QJsonParseError::NoError) throw Err(QStringLiteral("Higgsfield devolvió una respuesta inválida"));
    return doc;
}

bool isInstalled() { return !Shell::resolve(kBin).isEmpty(); }

// A fresh OAuth login has no workspace selected; solo accounts only have one.
static void ensureWorkspaceSelected() {
    try {
        if (json({"workspace", "status"}).object().contains("id")) return;
    } catch (const Err &) {}
    const auto list = json({"workspace", "list"}).array();
    if (!list.isEmpty()) {
        const QString first = list.first().toObject().value("id").toString();
        if (!first.isEmpty()) Shell::run(kBin, {"workspace", "set", first});
    }
}

bool isAuthenticated() {
    if (!QFileInfo::exists(Paths::higgsfieldCredentials())) return false;
    try { ensureWorkspaceSelected(); return true; } catch (const Err &) { return false; }
}

void login() {
    Shell::run(kBin, {"auth", "login"}, 120000);
    ensureWorkspaceSelected();
}

Credits credits() {
    Credits c;
    try {
        const auto o = json({"account", "status"}).object();
        // `credits` comes back as a decimal (e.g. 509.5)
        if (o.value("credits").isDouble()) { c.ok = true; c.credits = o.value("credits").toDouble(); }
        c.plan = o.value("subscription_plan_type").toString();
    } catch (const Err &) {}
    return c;
}

double cost(const QString &jobType, const Params &params) {
    try {
        const auto o = json(QStringList{"generate", "cost", jobType} + args(params), 20000).object();
        return o.value("credits").isDouble() ? o.value("credits").toDouble() : -1;
    } catch (const Err &) { return -1; }
}

QString upload(const QString &path) {
    const QString id = json({"upload", "create", path}).object().value("id").toString();
    if (id.isEmpty()) throw Err(QStringLiteral("No file in response"));
    return id;
}

QStringList args(const Params &params) {
    QStringList out;
    for (const auto &[key, value] : params) {
        if (!value.isValid()) continue;
        QString flag = "--" + key;
        flag.replace('_', '-');
        if (value.typeId() == QMetaType::QStringList) {
            for (const QString &v : value.toStringList()) out << flag + "=" + v;
        } else {
            out << flag + "=" + value.toString();
        }
    }
    return out;
}

Job generate(const QString &jobType, const Params &params, const std::function<void(const QString &)> &progress) {
    const auto ids = json(QStringList{"generate", "create", jobType} + args(params)).array();
    if (ids.isEmpty()) throw Err(QStringLiteral("Higgsfield: no job id returned"));
    const QString jobId = ids.first().toString();

    QString last;
    QElapsedTimer clock;
    clock.start();
    double elapsed = 0;
    while (elapsed < 600) {
        const double wait = elapsed == 0 ? 2 : elapsed < 45 ? 3 : 5;
        QThread::msleep(ulong(wait * 1000));
        const auto o = json({"generate", "get", jobId}).object();
        elapsed = clock.elapsed() / 1000.0;
        Job job{o.value("id").toString(), o.value("status").toString(), o.value("result_url").toString()};
        if (job.status != last) { progress(QStringLiteral("%1 · %2s").arg(job.status).arg(qRound(elapsed))); last = job.status; }
        if (job.status == "completed") return job;
        if (job.status == "failed") throw Err(QStringLiteral("Higgsfield: generation failed (credits refunded)"));
        if (job.status == "nsfw") throw Err(QStringLiteral("Higgsfield: rejected by content moderation (credits refunded)"));
    }
    throw Err(QStringLiteral("Timeout: job exceeded 10 minutes"));
}

}  // namespace Higgsfield
