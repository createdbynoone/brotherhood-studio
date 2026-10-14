#pragma once
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

// Blocking subprocess runner. Call from worker threads only (QtConcurrent / thread pool).
// GUI apps don't inherit the login-shell PATH, so `claude` / `higgsfield` are resolved
// against the usual install locations manually.
namespace Shell {

struct Result {
    QString out;
    QString err;
    int status = 0;
};

QStringList searchPath();
QString resolve(const QString &bin);                 // absolute path or empty
QProcessEnvironment environment();

/// Runs `bin args...`. Throws Err on missing binary, timeout or non-zero exit.
Result run(const QString &bin, const QStringList &args, int timeoutMs = -1);

}
