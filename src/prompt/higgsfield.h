#pragma once
#include <QList>
#include <QPair>
#include <QString>
#include <QVariant>
#include <functional>

// Image generation goes through the official `higgsfield` CLI: auth is a one-time OAuth
// browser login billed against the user's own plan. Every call appends --json and parses
// stdout. All functions block: call from worker threads.
namespace Higgsfield {

struct Job {
    QString id;
    QString status;
    QString resultUrl;
};

struct Credits { bool ok = false; double credits = 0; QString plan; };

using Params = QList<QPair<QString, QVariant>>;   // invalid QVariant = skipped; QStringList = repeated flag

bool isInstalled();
bool isAuthenticated();
void login();                                     // OAuth in the browser, 120 s cap
Credits credits();
QString upload(const QString &path);
/// Ordered params -> `--flag-name=value` args (the `=` form is required for cobra bool flags).
QStringList args(const Params &params);
/// `generate create` (no --wait), then poll `generate get` for up to 10 minutes. Each poll is a CLI
/// process + TLS handshake, so the interval backs off: 2 s, then 3 s for the first 45 s, then 5 s.
Job generate(const QString &jobType, const Params &params, const std::function<void(const QString &)> &progress);

}
